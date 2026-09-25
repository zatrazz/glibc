# See Makeconfig regarding the use of default-abi.
default-abi := 64

sysdep-CFLAGS += -fcall-used-g6
LD += -melf64_sparc

ifeq ($(subdir),stdlib)
sysdep_routines += __start_context
# Its CFI, including the resume point, must be in .eh_frame.
CFLAGS-swapcontext.c += -fasynchronous-unwind-tables
endif

ifeq ($(subdir),conform)
# For bug 23821 (incorrect type of si_band).
conformtest-xfail-conds += sparc64-linux
endif
