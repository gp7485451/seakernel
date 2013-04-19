/* Functions for unmapping addresses */
#include <kernel.h>
#include <memory.h>
#include <isr.h>
#include <task.h>
#include <swap.h>
#include <cpu.h>
int vm_do_unmap_only(unsigned virt, unsigned locked)
{
#if CONFIG_SWAP
	if(current_task && num_swapdev && current_task->num_swapped)
		swap_in_page((task_t *)current_task, virt & PAGE_MASK);
#endif
	if(kernel_task && (virt&PAGE_MASK) != PDIR_DATA && !locked)
		mutex_acquire(&pd_cur_data->lock);
	page_tables[(virt&PAGE_MASK)/0x1000] = 0;
	__asm__ volatile ("invlpg (%0)" : : "a" (virt));
#if CONFIG_SMP
	if(kernel_task && IS_KERN_MEM(virt))
		send_ipi(LAPIC_ICR_SHORT_OTHERS, 0, LAPIC_ICR_LEVELASSERT | LAPIC_ICR_TM_LEVEL | IPI_TLB);
#endif
	if(kernel_task && (virt&PAGE_MASK) != PDIR_DATA && !locked)
		mutex_release(&pd_cur_data->lock);
	return 0;
}

int vm_do_unmap(unsigned virt, unsigned locked)
{
	/* This gives the virtual address of the table needed, and sets
	 * the correct place as zero */
#if CONFIG_SWAP
	if(current_task && num_swapdev && current_task->num_swapped)
		swap_in_page((task_t *)current_task, virt & PAGE_MASK);
#endif
	if(kernel_task && (virt&PAGE_MASK) != PDIR_DATA && !locked)
		mutex_acquire(&pd_cur_data->lock);
	unsigned p = page_tables[(virt&PAGE_MASK)/0x1000];
	page_tables[(virt&PAGE_MASK)/0x1000] = 0;
	__asm__ volatile ("invlpg (%0)" : : "a" (virt));
#if CONFIG_SMP
	if(kernel_task && IS_KERN_MEM(virt))
		send_ipi(LAPIC_ICR_SHORT_OTHERS, 0, LAPIC_ICR_LEVELASSERT | LAPIC_ICR_TM_LEVEL | IPI_TLB);
#endif
	if(kernel_task && (virt&PAGE_MASK) != PDIR_DATA && !locked)
		mutex_release(&pd_cur_data->lock);
	if(p && !(p & PAGE_COW))
		pm_free_page(p & PAGE_MASK);
	return 0;
}
