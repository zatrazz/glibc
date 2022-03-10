ifeq ($(subdir),math)
# sqrtf128 requires soft-fp.
CPPFLAGS += -I../soft-fp

libm-support += powl_helper
tests += \
  test-fenv-clear-sse \
  test-fenv-sse \
  test-fenv-sse-2 \
  test-fenv-x87 \
  test-flt-eval-method-sse \
# tests
ifeq ($(have-test-cc-cflags-mfpmath-387),yes)
tests += test-flt-eval-method-387
endif
CFLAGS-test-fenv-sse.c += -msse2 -mfpmath=sse
CFLAGS-test-fenv-clear-sse.c += -msse2 -mfpmath=sse
CFLAGS-test-fenv-sse-2.c += -msse2 -mfpmath=sse
CFLAGS-test-flt-eval-method-387.c += $(config-cflags-fexcess-precision-standard) -mfpmath=387
CFLAGS-test-flt-eval-method-sse.c += $(config-cflags-fexcess-precision-standard) -msse2 \
				     -mfpmath=sse
endif
