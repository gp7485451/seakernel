/* Functions for mapping of addresses */
#include <sea/kernel.h>
#include <sea/mm/vmm.h>
#include <sea/cpu/interrupt.h>
#include <sea/tm/process.h>
#include <sea/cpu/processor.h>
#include <sea/mm/pmm.h>
#include <sea/cpu/interrupt.h>
#include <sea/cpu/cpu-x86.h>
#include <sea/asm/system.h>
int arch_mm_vm_map(addr_t virt, addr_t phys, unsigned attr, unsigned opt)
{
	unsigned vpage = (virt&PAGE_MASK)/0x1000;
	unsigned vdir = PAGE_DIR_IDX(vpage);
	addr_t p;
	unsigned *pd = page_directory;
	if(kernel_task && !(opt & MAP_PDLOCKED))
		mutex_acquire(&pd_cur_data->lock);
	if(!pd[vdir])
	{
		p = mm_alloc_physical_page();
		mm_zero_page_physical(p);
		pd[vdir] = p | PAGE_WRITE | PAGE_PRESENT | (attr & PAGE_USER);
		flush_pd();
	}
	if(current_task->pid > 5 && page_tables[vpage]) {
		panic(0, "WARNING - task %d in sys %d overwriting page table entry %x: was %x, now: %x %x\n", current_task->pid, current_task->system, virt, page_tables[vpage], phys, attr);
	}
	page_tables[vpage] = (phys & PAGE_MASK) | attr;
	asm("invlpg (%0)"::"r" (virt));
	if(!(opt & MAP_NOCLEAR))
		memset((void *)(virt&PAGE_MASK), 0, 0x1000);
#if CONFIG_SMP
	if(kernel_task) {
		if(IS_KERN_MEM(virt))
			x86_cpu_send_ipi(LAPIC_ICR_SHORT_OTHERS, 0, LAPIC_ICR_LEVELASSERT | LAPIC_ICR_TM_LEVEL | IPI_TLB);
		else if((IS_THREAD_SHARED_MEM(virt) && pd_cur_data->count > 1))
			x86_cpu_send_ipi(LAPIC_ICR_SHORT_OTHERS, 0, LAPIC_ICR_LEVELASSERT | LAPIC_ICR_TM_LEVEL | IPI_TLB);
	}
#endif
	if(kernel_task && !(opt & MAP_PDLOCKED))
		mutex_release(&pd_cur_data->lock);
	return 0;
}
