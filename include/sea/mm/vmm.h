#ifndef __SEA_MM_VMM_H
#define __SEA_MM_VMM_H

#include <sea/mm/context.h>
#include <sea/tm/process.h>
#include <sea/mm/pmm.h>
#if CONFIG_ARCH == TYPE_ARCH_X86
#include <sea/mm/memory-x86.h>
#elif CONFIG_ARCH == TYPE_ARCH_X86_64
#include <sea/mm/memory-x86_64.h>
#endif

struct pd_data {
	unsigned count;
	mutex_t lock;
};
extern struct pd_data *pd_cur_data;
typedef addr_t page_dir_t, page_table_t, pml4_t, pdpt_t;
extern volatile page_dir_t *kernel_dir, *current_dir;
extern int id_tables;
extern addr_t i_stack;
page_dir_t *mm_vm_clone(page_dir_t *pd, char cow);
page_dir_t *mm_vm_copy(page_dir_t *pd);
void mm_free_thread_shared_directory();
void mm_destroy_task_page_directory(task_t *p);
void mm_free_thread_specific_directory();
void mm_vm_init(addr_t id_map_to);
void mm_vm_init_2();
void mm_vm_switch_context(page_dir_t *n/*VIRTUAL ADDRESS*/);
addr_t mm_vm_get_map(addr_t v, addr_t *p, unsigned locked);
void mm_vm_set_attrib(addr_t v, short attr);
unsigned int mm_vm_get_attrib(addr_t v, unsigned *p, unsigned locked);
int mm_vm_map(addr_t virt, addr_t phys, unsigned attr, unsigned opt);
int mm_vm_unmap_only(addr_t virt, unsigned locked);
int mm_vm_unmap(addr_t virt, unsigned locked);
int mm_is_valid_user_pointer(int num, void *p, char flags);

static void map_if_not_mapped(addr_t loc)
{
	if(!mm_vm_get_map(loc & PAGE_MASK, 0, 0))
		mm_vm_map(loc & PAGE_MASK, mm_alloc_physical_page(), 
		       PAGE_PRESENT | PAGE_WRITE, MAP_CRIT);
}

static void map_if_not_mapped_noclear(addr_t loc)
{
	if(!mm_vm_get_map(loc & PAGE_MASK, 0, 0))
		mm_vm_map(loc & PAGE_MASK, mm_alloc_physical_page(), 
		       PAGE_PRESENT | PAGE_WRITE, MAP_CRIT | MAP_NOCLEAR);
}

static void user_map_if_not_mapped(addr_t loc)
{
	if(!mm_vm_get_map(loc & PAGE_MASK, 0, 0))
		mm_vm_map(loc & PAGE_MASK, mm_alloc_physical_page(), 
		       PAGE_PRESENT | PAGE_WRITE | PAGE_USER, MAP_CRIT);
	else
		mm_vm_set_attrib(loc & PAGE_MASK, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
}

static void user_map_if_not_mapped_noclear(addr_t loc)
{
	if(!mm_vm_get_map(loc & PAGE_MASK, 0, 0))
		mm_vm_map(loc & PAGE_MASK, mm_alloc_physical_page(), 
		       PAGE_PRESENT | PAGE_WRITE | PAGE_USER, MAP_CRIT | MAP_NOCLEAR);
	else
		mm_vm_set_attrib(loc & PAGE_MASK, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
}
#endif
