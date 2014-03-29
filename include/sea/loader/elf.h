#ifndef __ELF_H
#define __ELF_H
#include <config.h>
#if CONFIG_ARCH == TYPE_ARCH_X86
#include <elf-x86.h>
#elif CONFIG_ARCH == TYPE_ARCH_X86_64
#include <elf-x86_64.h>
#endif

void *loader_parse_elf_module(module_t *mod, void * buf);
int arch_loader_parse_elf_executable(void *mem, int fp, addr_t *start, addr_t *end);
int loader_parse_elf_executable(void *mem, int fp, addr_t *start, addr_t *end);

void *arch_loader_parse_elf_module(module_t *mod, uint8_t * buf);
size_t arch_loader_calculate_allocation_size(void *buf);
int arch_loader_relocate_elf_module(void * buf, addr_t *entry, addr_t *tm_exiter, void *load_address);

#endif