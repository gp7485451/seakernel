ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),config)
ifneq ($(MAKECMDGOALS),defconfig)
include ../sea_defines.inc
endif
endif
include ../make.inc
endif

MODULES-$(CONFIG_MODULE_LOOP)       += block/loop.m
MODULES-$(CONFIG_MODULE_CRYPTO_PART) += block/crypto-part.m
MODULES-$(CONFIG_MODULE_RAND)	    += char/rand.m
MODULES-$(CONFIG_MODULE_CRC32)      += library/crc32.m
MODULES-$(CONFIG_MODULE_ETHERNET)   += net/ethernet.m
MODULES-$(CONFIG_MODULE_I825XX)     += net/cards/i825xx.m
MODULES-$(CONFIG_MODULE_RTL8139)    += net/cards/rtl8139.m
MODULES-$(CONFIG_MODULE_I350)       += net/cards/i350.m
MODULES-$(CONFIG_MODULE_PARTITIONS) += partitions/partitions.m
MODULES-$(CONFIG_MODULE_NETLOOP)    += net/lo.m
MODULES-$(CONFIG_MODULE_UDP)        += net/udp.m

SUBDIRS-$(CONFIG_MODULE_AES)	    += aes
SUBDIRS-$(CONFIG_MODULE_AHCI)	    += ahci
SUBDIRS-$(CONFIG_MODULE_EXT2)	    += ext2
SUBDIRS-$(CONFIG_MODULE_ISO9660)    += iso9660
SUBDIRS-$(CONFIG_MODULE_FAT)	    += fat
SUBDIRS-$(CONFIG_MODULE_PSM)	    += psm
SUBDIRS-$(CONFIG_MODULE_SHIV)	    += shiv
SUBDIRS-$(CONFIG_MODULE_IPV4)	    += net/ipv4

ALLSUBDIRS = aes ext2 iso9660 fat ahci psm shiv net/ipv4

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
include ../arch/$(ARCH)/drivers/make.inc
endif
endif

MODULES += $(MODULES-y)
SUBDIRS = "$(SUBDIRS-y)"
VERSION=${KERNEL_VERSION}

# note the use of mcmodel=large
# I believe, but don't quote me on this, that
# this flag is needed if we want to access relocations
# that require 64bit fields...
MCFLAGS:=        -std=gnu11 \
		-D__KERNEL__ -mno-red-zone -ffreestanding -include `realpath ../sea_defines.h`\
		-nostdlib \
		-I`realpath ../arch/${ARCH}/include` \
		-I`realpath ../include` \
		-I`realpath ../arch/include`\
		-Wall -Wextra -Wformat-security -Wformat-nonliteral -Wshadow \
		-Wpointer-arith -Wcast-align -Wno-unused -Wnested-externs \
		-Winline -Wno-long-long -Wno-unused-parameter \
		-Wno-unused-but-set-parameter $(CFLAGS_ARCH) \
		-mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow \
		-fno-omit-frame-pointer \
		-fno-tree-loop-distribute-patterns \
		-fno-common 

ifeq ($(ARCH), x86_64)
MCFLAGS += -mcmodel=large
endif

MCFLAGS += -O$(CONFIG_OPTIMIZATION_LEVEL)

ifeq ($(CONFIG_DEBUG),y)
	MCFLAGS += -g -D__DEBUG__
endif

ifeq ($(CONFIG_WERROR),y)
	MCFLAGS += -Werror
endif

export MCFLAGS
export CC
export LD

all:
	@$(MAKE) -s all_s

all_s: make.deps $(MODULES)
	echo "Building modules, pass 2..."
	
	for i in "$(SUBDIRS)"; do \
		if [[ $$i != "" ]]; then \
			export MCFLAGS="$(MCFLAGS)" ;\
			$(MAKE) -s -C $$i ;\
		fi ;\
	done
	@-mkdir -p built
	@-rm -rf built/*
	@-for i in $(MODULES); do \
		MOD=`basename $$i | sed -e "s@^\(.*\)\.m@\1@"` ; \
		cp -f $$i built/$$MOD ;\
	done
	@-for i in "$(SUBDIRS)"; do \
		if [[ $$i != "" ]]; then \
		cp -f $$i/$$(basename $$i).m built/$$(basename $$i) ;\
		fi ;\
	done
	@cp modules.deps built/

install: $(MODULES)
	@-for i in $(MODULES); do \
		MOD=`basename $$i | sed -e "s@^\(.*\)\.m@\1@"` ; \
		cp -vf built/$$MOD /sys/modules-${VERSION}/$$MOD ;\
	done
	@-for i in "$(SUBDIRS)"; do \
		cp -f built/$$i /sys/modules-${VERSION}/$$i ;\
	done
	@-cp modules.deps /sys/modules-${VERSION}/modules.deps

%.m: %.c
	echo -n -e "[CC] M \t$<                 \n"
	$(CC) $(MCFLAGS) $< -c -o $@.o
	echo -n -e "[LD] M \t$@                 \n"
	$(CC) $(MCFLAGS) $(LDFLAGS_ARCH) $@.o -o $@ -Wl,-r,-Tlink.ld ../library/klib.a -lgcc -static

clean:
	@-find -name "*.m" | xargs rm -f
	@-find -name "*.o" | xargs rm -f
	@-find -name "*.m.1" | xargs rm -f
	@-find -name "make.deps" | xargs rm -f
	@-find $$(find ../arch -name drivers | grep -v kernel) -name "*.m" | xargs rm -f
	@-find $$(find ../arch -name drivers | grep -v kernel) -name "*.o" | xargs rm -f
	@-find $$(find ../arch -name drivers | grep -v kernel) -name "make.deps" | xargs rm -f
	@-for i in $$(find $$(find ../arch -name drivers | grep -v kernel) -name "Makefile"); do \
		-$(MAKE) -s -C $$(dirname $$i) clean 2>/dev/null ;\
	done
	@-rm -rf built/* $(MODULES) $(MODULES:.m=.m.o)
	@-find ../arch -name "*.m" | xargs rm -f
	@-find ../arch -name "*.m.o" | xargs rm -f
	@-for i in $(ALLSUBDIRS); do \
		-$(MAKE) -s -C $$i clean 2>/dev/null ;\
	done

export OBJ_EXT=m
export DOBJS=${MODULES}
export DCFLAGS=${MCFLAGS}
include ../tools/make/deps.inc
