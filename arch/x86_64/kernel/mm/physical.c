/* Defines functions for physical memory */
#include <sea/kernel.h>
#include <sea/mm/vmm.h>
#include <sea/boot/multiboot.h>
#include <sea/tm/process.h>
#include <sea/mm/swap.h>
#include <sea/tm/schedule.h>

addr_t arch_mm_alloc_physical_page_zero()
{
	addr_t ret = mm_alloc_physical_page();
	if(kernel_state_flags & KSF_PAGING)
		memset((void *)(ret + PHYS_PAGE_MAP), 0, 0x1000);
	else
		memset((void *)ret, 0, 0x1000);
	return ret;
}
