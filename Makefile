# Make all projects in trusty.

NEBULA_VERSION_STR := $(shell git describe --always --tags)

PRJS=log storage libtrusty ta-loader-proxy libnbl_fwk_ree nbl_secmgr_agent \
	libnbl_loader_ree nblruncmd nbl_fwk_test libnbl_secmgr_ree libnbl_secmgr_ree/tests nbl_daemon \
	libnbl_license_ree libnbl_license_ree/tests libseccam-ipc nbl_tui libnbl_attest_ree openssl \
	nbl_attestd ion_test nblmsg benchmark nbl_benchmarks fr_test_ca

.PHONY: all $(PRJS)

all: $(PRJS)

libtrusty: log
storage: libtrusty log libnbl_fwk_ree
ta-loader-proxy: libtrusty log
libnbl_fwk_ree: libtrusty log
nbl_secmgr_agent: libnbl_fwk_ree
libnbl_loader_ree: libtrusty log libnbl_fwk_ree
nblruncmd: libtrusty
nbl_fwk_test: libnbl_fwk_ree libnbl_loader_ree
libnbl_secmgr_ree: libnbl_fwk_ree
nbl_daemon: libnbl_secmgr_ree libnbl_loader_ree libnbl_license_ree
libnbl_license_ree: libnbl_fwk_ree
libnbl_license_ree/tests: libnbl_license_ree
libnbl_secmgr_ree/tests: libnbl_secmgr_ree
libseccam-ipc: log libtrusty
libnbl_attest_ree: libnbl_secmgr_ree libnbl_fwk_ree libnbl_loader_ree openssl
nbl_attestd: libnbl_attest_ree ion_test
ion_test: libtrusty log libseccam-ipc
nblmsg: libnbl_fwk_ree
nbl_benchmarks: benchmark
fr_test_ca: libnbl_fwk_ree

ifeq ($(USE_CLANG),true)
  CC=$(CLANG_CROSS_COMPILE)clang
  CPP=$(CLANG_CROSS_COMPILE)clang++
  AR=$(CLANG_CROSS_COMPILE)llvm-ar
  GCC_TOOLCHAIN_PATH="$(dir $(GCC_CROSS_COMPILE))../"
  CFLAGS += --target=aarch64-linux-gnu
  CFLAGS += --gcc-toolchain="$(GCC_TOOLCHAIN_PATH)"
  CFLAGS += --sysroot=$(GCC_TOOLCHAIN_PATH)/aarch64-linux-gnu/libc
else
  CC=$(GCC_CROSS_COMPILE)gcc
  CPP=$(GCC_CROSS_COMPILE)g++
  AR=$(GCC_CROSS_COMPILE)ar
endif

export CC
export CPP
export AR
export CFLAGS

$(PRJS):
	$(MAKE) NEBULA_VERSION_STR="$(NEBULA_VERSION_STR)" -C $@ all
