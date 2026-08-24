ifeq ($(subdir),stdlib)
gen-as-const-headers += ucontext_i.sym
endif

ifeq ($(subdir),signal)
sysdep_routines += sigrestorer
endif

ifeq ($(subdir),misc)
# MIPS/Tile-style cacheflush routine
sysdep_headers += sys/cachectl.h
sysdep_routines += cacheflush
endif

ifeq ($(subdir),elf)
# Only MMUv4 supports transparent huge pages.  The huge page size is a
# kernel configuration choice (ARC_HUGEPAGE_2M or ARC_HUGEPAGE_16M) which
# pegs the page table layout to a 2MB or 16MB super page.  Only the
# default 2MB is supported: a 16MB PT_LOAD alignment can not be linked
# on arc because the R_ARC_S25W_PCREL branch from __start to
# __libc_start_main in static tests has a +-16MB range.
THP-PAGE-SIZE = 0x200000
endif
