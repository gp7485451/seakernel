LDFLAGS_ARCH =-Wl,-Tarch/x86/link.ld -Wl,-mseaos_i386
ASFLAGS_ARCH =-felf32
GASFLAGS_ARCH=--32
CFLAGS_ARCH  =-m32 -I arch/x86_common/include
include arch/x86/kernel/make.inc
include arch/x86_common/make.inc
