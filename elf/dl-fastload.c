/* Fastload: position-skip hash table for symbol lookup.
   Copyright (C) 2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/* When a process has many directly-linked DSOs, do_lookup_x walks the
   main search list once per undefined symbol.  This is O(N) per lookup,
   and total cost is O(N * relocs) — quadratic when relocs scales with N
   (BZ#16709).

   At startup, after _dl_map_object_deps has produced the final initial
   r_searchlist, we walk each DSO's .gnu.hash (or DT_HASH) and insert
   every exported symbol into a hash table keyed by (hash, name) whose
   value is the earliest position in the search list at which that
   symbol is defined.

   do_lookup_x then consults this table whenever it would otherwise
   start a scope walk at index 0 of the main search list, and skips
   directly to the recorded position.  Symbols not in the table are not
   defined anywhere in the initial set; we return num_objects, which
   sends do_lookup_x past every initial DSO (it will still consult any
   dlopen'd objects that come after the initial set, which are at
   positions >= num_objects).

   The table is allocated once via the loader's minimal allocator; it
   is read-only after _dl_fill_position_hash returns.  dlopen does not
   modify the table — this is safe because:

   * If the new object defines a symbol that already lived in the
     initial set, the recorded position still wins (first-definition
     semantics).

   * If the new object defines a new symbol, lookup returns
     num_objects and the scope walk reaches it.

   Build of the table reuses the precomputed hashes from .gnu.hash —
   we do NOT call _dl_new_hash per symbol during fill.  */

#include <ldsodefs.h>
#include <dl-hash.h>
#include <dl-new-hash.h>
#include <dl-tunables.h>
#include <stdint.h>
#include <link.h>
#include <string.h>
#include <atomic.h>


/* Hash table entry.  The hash field stores the Bernstein hash with the
   low bit cleared (the same value found in DT_GNU_HASH chains), which
   lets us copy values straight out of the chains without recomputing
   anything during fill.  */
struct position_hash_entry
{
  uint32_t hash;
  uint32_t pos;
  const char *name;
};

/* The table itself.  Read-only after _dl_fill_position_hash returns;
   keep state-tracking variables file-local so the lookup path doesn't
   pay an extra indirection through rtld_global_ro.  */
static struct position_hash_entry *position_hash_table;
static unsigned int position_hash_slot_mask;     /* slots - 1 */
static unsigned int position_hash_lookup_default; /* num_objects */


/* Insert (HASH, NAME, POS) into TABLE.  If NAME is already present,
   keep the smaller of the existing and new POS — symbol interposition
   means the earliest definition wins, and we record that one.

   Open addressing with triangular probing.  The table is sized in
   advance so this never overflows.  */
static void
position_hash_put (struct position_hash_entry *table, unsigned int slot_mask,
                   uint32_t hash, const char *name, uint32_t pos)
{
  unsigned int i = hash & slot_mask;
  unsigned int stride = 0;

  while (1)
    {
      struct position_hash_entry *slot = &table[i];

      if (slot->name == NULL)
        {
          slot->hash = hash;
          slot->pos = pos;
          slot->name = name;
          return;
        }

      if (slot->hash == hash
          && (slot->name == name || strcmp (slot->name, name) == 0))
        {
          if (pos < slot->pos)
            slot->pos = pos;
          return;
        }

      stride++;
      i = (i + stride) & slot_mask;
    }
}


/* Lookup new_hash + NAME in the table.  Returns the earliest position
   in the main search list at which NAME is defined, or
   position_hash_lookup_default if NAME is not in the table.

   When the table has not been built (small process, fastload disabled,
   or allocation failed) returns 0, meaning "start search at the
   beginning" — equivalent to the unmodified do_lookup_x behavior.  */
int
_dl_position_hash_lookup (uint32_t new_hash, const char *name)
{
  struct position_hash_entry *table
    = atomic_load_acquire (&position_hash_table);
  if (table == NULL)
    return 0;

  uint32_t hash = new_hash >> 1;
  unsigned int slot_mask = position_hash_slot_mask;
  unsigned int i = hash & slot_mask;
  unsigned int stride = 0;

  while (1)
    {
      struct position_hash_entry *slot = &table[i];

      if (slot->name == NULL)
        return position_hash_lookup_default;

      if (slot->hash == hash
          && (slot->name == name || strcmp (slot->name, name) == 0))
        return slot->pos;

      stride++;
      i = (i + stride) & slot_mask;
    }
}


/* Decide whether SYM (in the dynamic symbol table of some DSO) should
   be inserted into the fastload table.  The filter mirrors check_match
   in dl-lookup.c — we only want symbols that could ever satisfy a
   lookup.  */
static int
position_hash_include_symbol (const ElfW(Sym) *sym)
{
  unsigned int stt = ELFW(ST_TYPE) (sym->st_info);

  /* Undefined symbols are not definitions.  Reject them.  */
  if (sym->st_shndx == SHN_UNDEF)
    return 0;

  /* "Empty" symbols (st_value == 0) cannot satisfy a normal lookup
     unless they live in SHN_ABS or are TLS.  */
  if (sym->st_value == 0
      && sym->st_shndx != SHN_ABS
      && stt != STT_TLS)
    return 0;

  /* The only symbol types check_match accepts.  */
  switch (stt)
    {
    case STT_NOTYPE:
    case STT_OBJECT:
    case STT_FUNC:
    case STT_COMMON:
    case STT_TLS:
    case STT_GNU_IFUNC:
      break;
    default:
      return 0;
    }

  /* Local symbols never satisfy a global lookup.  .gnu.hash doesn't
     emit them, but DT_HASH does.  */
  if (ELFW(ST_BIND) (sym->st_info) == STB_LOCAL)
    return 0;

  return 1;
}


/* Return one past the highest dynamic symbol index that .gnu.hash
   references, or 0 if the section is empty / malformed.  Used to find
   the iteration end in position_hash_fill_from_gnu_hash.  */
static uint32_t
last_gnu_hash_bucket (const struct link_map *map)
{
  if (__glibc_unlikely (map->l_nbuckets < 1))
    return 0;

  /* Scan buckets in reverse for the first non-empty one.  Its value is
     the symidx that starts the last chain.  */
  Elf32_Word last_idx = map->l_nbuckets - 1;
  while (last_idx > 0 && map->l_gnu_buckets[last_idx] == 0)
    --last_idx;

  return map->l_gnu_buckets[last_idx];
}


/* Count the number of exported symbols in MAP that we'd insert into
   the table.  Used to pre-size the table so we never need to rehash.
   Caller must have already checked that DT_GNU_HASH or DT_HASH is
   present.  */
static uint32_t
count_exported_symbols (const struct link_map *map)
{
  if (map->l_info[ADDRIDX (DT_GNU_HASH)] != NULL)
    {
      uint32_t last_bucket = last_gnu_hash_bucket (map);
      if (last_bucket == 0)
        return 0;

      /* From .gnu.hash: the highest symidx that the section references
         is the last one in the last chain.  Find it by following the
         chain starting at last_bucket until a chain-end marker.  */
      const Elf32_Word *chain_zero = map->l_gnu_chain_zero;
      uint32_t k = last_bucket;
      while ((chain_zero[k] & 1u) == 0)
        ++k;
      /* k is the index of the last symbol.  Number of indexed symbols
         is (k - symbias + 1).  symbias = chains_start - chain_zero. */
      const Elf32_Word *chains_start
        = &map->l_gnu_buckets[map->l_nbuckets];
      uint32_t symbias = chains_start - chain_zero;
      return k - symbias + 1;
    }
  else if (map->l_info[DT_HASH] != NULL)
    {
      /* DT_HASH header: nbucket, nchain, ... — nchain is the number of
         entries in .dynsym this hash references.  */
      const Elf_Symndx *hash
        = (const void *) D_PTR (map, l_info[DT_HASH]);
      if (hash == NULL)
        return 0;
      return hash[1];
    }

  return 0;
}


/* Fill the table with all qualifying symbols from MAP, recording POS
   as the earliest position.  Uses .gnu.hash chain hashes directly
   (zero hashing overhead).  */
static void
position_hash_fill_from_gnu_hash (const struct link_map *map, uint32_t pos,
                                  struct position_hash_entry *table,
                                  unsigned int slot_mask)
{
  uint32_t last_bucket = last_gnu_hash_bucket (map);
  if (last_bucket == 0)
    return;

  const Elf32_Word *chain_zero = map->l_gnu_chain_zero;
  const Elf32_Word *chains_start = &map->l_gnu_buckets[map->l_nbuckets];
  uint32_t symbias = chains_start - chain_zero;
  const ElfW(Sym) *symtab
    = (const void *) D_PTR (map, l_info[DT_SYMTAB]);
  const char *strtab
    = (const void *) D_PTR (map, l_info[DT_STRTAB]);

  for (uint32_t k = symbias; ; ++k)
    {
      uint32_t hash = chain_zero[k];
      const ElfW(Sym) *sym = &symtab[k];
      if (position_hash_include_symbol (sym))
        position_hash_put (table, slot_mask,
                           hash >> 1, strtab + sym->st_name, pos);

      /* Stop after the chain-end marker of the highest chain.  */
      if (k >= last_bucket && (hash & 1u))
        break;
    }
}


/* DT_HASH fallback for DSOs without .gnu.hash.  We have to compute
   hashes ourselves since DT_HASH stores SysV hashes, not DJB.  */
static void
position_hash_fill_from_symtab (const struct link_map *map, uint32_t pos,
                                struct position_hash_entry *table,
                                unsigned int slot_mask)
{
  const Elf_Symndx *hash
    = (const void *) D_PTR (map, l_info[DT_HASH]);
  if (hash == NULL)
    return;

  uint32_t nchain = hash[1];
  const ElfW(Sym) *symtab
    = (const void *) D_PTR (map, l_info[DT_SYMTAB]);
  const char *strtab
    = (const void *) D_PTR (map, l_info[DT_STRTAB]);

  for (uint32_t k = 0; k < nchain; ++k)
    {
      const ElfW(Sym) *sym = &symtab[k];
      if (position_hash_include_symbol (sym))
        {
          const char *name = strtab + sym->st_name;
          uint32_t djb = _dl_new_hash (name);
          position_hash_put (table, slot_mask, djb >> 1, name, pos);
        }
    }
}


/* Round X up to the next power of two.  X must be > 0.  */
static unsigned int
next_pow2 (unsigned int x)
{
  unsigned int r = 1;
  while (r < x)
    r <<= 1;
  return r;
}


/* Allocate and populate the position hash table from MAIN_MAP's
   search list.  Called once, from rtld after _dl_map_object_deps has
   produced the final initial search list, and only when the number of
   objects exceeds glibc.rtld.lookup_hash_cutoff.  Safe to call again
   (it tears down any prior state) but in practice it is called once
   per process.  */
void
_dl_fill_position_hash (struct link_map *main_map)
{
  const unsigned int num_objects = main_map->l_searchlist.r_nlist;
  const int debug_fastload = GLRO(dl_debug_mask) & DL_DEBUG_FASTLOAD;

  /* Read the cutoff tunable.  -1 disables fastload entirely; otherwise
     it's the minimum DSO count to enable the table.  */
  int32_t cutoff = TUNABLE_GET (glibc, rtld, lookup_hash_cutoff,
                                int32_t, NULL);
  GLRO(dl_lookup_hash_cutoff) = cutoff;

  if (cutoff < 0)
    {
      if (__glibc_unlikely (debug_fastload))
        _dl_debug_printf ("fastload: disabled (cutoff < 0)\n");
      return;
    }

  if (num_objects <= (unsigned int) cutoff)
    {
      if (__glibc_unlikely (debug_fastload))
        _dl_debug_printf ("fastload: disabled (%u objects <= cutoff %d)\n",
                          num_objects, cutoff);
      return;
    }

  /* Sum exported symbols across all initial DSOs.  Used to pre-size
     the table.  This estimate is an upper bound: many symbols collide
     (e.g. weak duplicates of __gmon_start__) and a single slot covers
     all duplicates.  */
  uint64_t total_symbols = 0;
  for (unsigned int k = 0; k < num_objects; ++k)
    {
      const struct link_map *map = main_map->l_searchlist.r_list[k];
      total_symbols += count_exported_symbols (map);
    }
  if (total_symbols == 0)
    {
      if (__glibc_unlikely (debug_fastload))
        _dl_debug_printf ("fastload: disabled (no symbols)\n");
      return;
    }

  /* Aim for a load factor of about 25% to keep probe chains short.
     The table is power-of-two sized.  Guard against pathological
     overflow (the SIZE_MAX/16 limit makes total_symbols * 4 fit in
     a unsigned int on 32-bit even with billions of "symbols"; in
     practice 1M total symbols caps the table at 16M entries / 128
     MiB which is fine).  */
  if (total_symbols > UINT32_MAX / 4)
    total_symbols = UINT32_MAX / 4;
  unsigned int slots = next_pow2 ((unsigned int) total_symbols * 4);
  if (slots < 16)
    slots = 16;

  size_t bytes = slots * sizeof (struct position_hash_entry);
  struct position_hash_entry *table = calloc (1, bytes);
  if (table == NULL)
    {
      /* Out of memory.  Fall back to the linear scan — fastload is an
         optimization, never a correctness requirement.  */
      if (__glibc_unlikely (debug_fastload))
        _dl_debug_printf ("fastload: cannot allocate %zu bytes; "
                          "falling back to linear scan\n", bytes);
      return;
    }

  unsigned int slot_mask = slots - 1;

  for (unsigned int k = 0; k < num_objects; ++k)
    {
      const struct link_map *map = main_map->l_searchlist.r_list[k];

      if (map->l_info[ADDRIDX (DT_GNU_HASH)] != NULL)
        position_hash_fill_from_gnu_hash (map, k, table, slot_mask);
      else if (map->l_info[DT_HASH] != NULL)
        position_hash_fill_from_symtab (map, k, table, slot_mask);
    }

  /* Publish the table.  After this point _dl_position_hash_lookup may
     observe the new state from any thread.  Pair the publication with
     a release barrier so the table's contents are visible to readers
     using an acquire barrier (in practice readers run on the same
     thread that just stored, so this is belt-and-suspenders).  */
  position_hash_slot_mask = slot_mask;
  position_hash_lookup_default = num_objects;
  atomic_store_release (&position_hash_table, table);

  if (__glibc_unlikely (debug_fastload))
    _dl_debug_printf ("fastload: enabled, %u slots, %u objects, "
                      "%lu symbols inserted\n",
                      slots, num_objects,
                      (unsigned long) total_symbols);
}
