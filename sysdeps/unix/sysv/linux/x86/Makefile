# We don't need any header files.
abi-includes :=

abi-variants := 32 64 x32

abi-32-condition := !defined __x86_64__
abi-64-condition := defined __x86_64__ && defined __LP64__
abi-x32-condition := defined __x86_64__ && defined __ILP32__

ifeq ($(subdir),misc)
sysdep_headers += sys/elf.h sys/perm.h sys/reg.h sys/vm86.h sys/debugreg.h sys/io.h
endif

ifeq ($(subdir),setjmp)
tests += tst-saved_mask-1
endif

ifeq ($(subdir),stdlib)
tests += \
  tst-ucontext-unwind-fault \
  # tests

CFLAGS-tst-ucontext-unwind-fault.c += -fasynchronous-unwind-tables \
  -fno-optimize-sibling-calls
LDFLAGS-tst-ucontext-unwind-fault = $(LDFLAGS-rdynamic)
endif
