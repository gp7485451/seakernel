#ifndef __ARCH_SEA_CPU_REGISTERS_X86_H
#define __ARCH_SEA_CPU_REGISTERS_X86_H

#include <sea/types.h>

typedef volatile struct __attribute__((packed))
{
	volatile   uint32_t ds;
	volatile   uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	volatile   uint32_t int_no, err_code;
	volatile   uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

void arch_cpu_print_reg_state(registers_t *);
#endif
