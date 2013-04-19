/* Defines functions to program the LAPIC and the IOAPIC (if present) */
#include <config.h>
#if CONFIG_SMP
#include <kernel.h>
#include <cpu.h>
#include <task.h>
#include <mutex.h>
#include <imps.h>

#define MAX_IOAPIC 8
#define write_ioapic(l,o,v) ioapic_rw(l, WRITE, o, v)
#define read_ioapic(l,o) ioapic_rw(l, READ, o, 0)
unsigned lapic_timer_start=0;
volatile unsigned num_ioapic=0;
struct imps_ioapic *ioapic_list[MAX_IOAPIC];
extern char imcr_present;
extern int imps_enabled;

void add_ioapic(struct imps_ioapic *ioapic)
{
	assert(ioapic);
	assert(ioapic->type == 2);
	assert(num_ioapic < MAX_IOAPIC);
	ioapic_list[num_ioapic]=ioapic;
	ioapic_list[++num_ioapic]=0;
}

unsigned ioapic_rw(struct imps_ioapic *l, int rw, unsigned char off, unsigned val)
{
	*(uint32_t*)(l->addr) = off;
	if(rw == WRITE)
		return (*(uint32_t*)(l->addr + 0x10) = val);
	else
		return *(uint32_t*)(l->addr + 0x10);
}

void write_ioapic_vector(struct imps_ioapic *l, unsigned irq, char masked, char trigger, char polarity, char mode, unsigned char vector)
{
	unsigned lower=0, higher=0;
	lower = (unsigned)vector & 0xFF;
	/* 8-10: delivery mode */
	lower |= (mode << 8) & 0x700;
	/* 11: destination mode */
	//lower |= (1 << 11);
	/* 13: polarity */
	if(polarity) lower |= (1 << 13);
	/* 15: trigger */
	if(trigger) lower |= (1 << 15);
	/* 16: mask */
	if(masked) lower |= (1 << 16);
	/* 56-63: destination. Currently, we just send this to the bootstrap cpu */
	higher |= (bootstrap << 24) & 0xF;
	write_ioapic(l, irq*2 + 0x10, 0x10000);
	write_ioapic(l, irq*2 + 0x10 + 1, higher);
	write_ioapic(l, irq*2 + 0x10, lower);
}

int program_ioapic(struct imps_ioapic *ia)
{
	int i;
	for(i=0;i<16;i++) {
		if(i != 2)
			write_ioapic_vector(ia, i, 0, 0, 0, 0, 32+i);
	}
	return 1;
}

void lapic_eoi()
{
	if(!imps_enabled)
		return;
	IMPS_LAPIC_WRITE(LAPIC_EOI, 0x0);
}

void set_lapic_timer(unsigned tmp)
{
	if(!imps_enabled)
		return;
	/* mask the old timer interrupt */
	mask_pic_int(0, 1);
	/* THE ORDER OF THESE IS IMPORTANT!
	 * If you write the initial count before you
	 * tell it to set periodic mode and set the vector
	 * it will not generate an interrupt! */
	IMPS_LAPIC_WRITE(LAPIC_LVTT, 32 | 0x20000);
	IMPS_LAPIC_WRITE(LAPIC_TDCR, 3);
	IMPS_LAPIC_WRITE(LAPIC_TICR, tmp);
}

void calibrate_lapic_timer(unsigned freq)
{
	if(!imps_enabled)
		return;
	kprintf("warning - not calibrating\n");
	lapic_timer_start = 0x10000;
	set_lapic_timer(lapic_timer_start);
}

void init_lapic(int extint)
{
	if(!imps_enabled)
		return;
	int i;
	/* we may be in a state where there are interrupts left
	 * in the registers that haven't been EOI'd. I'm pretending like
	 * I know why that may be. Linux does this, and that's their
	 * explination */
	for(i=0;i<=255;i++)
		lapic_eoi();
	/* these are not yet configured */
	IMPS_LAPIC_WRITE(LAPIC_DFR, 0xFFFFFFFF);
	IMPS_LAPIC_WRITE(LAPIC_LDR, (IMPS_LAPIC_READ(LAPIC_LDR)&0x00FFFFFF)|1);
	/* disable the timer while we set up */
	IMPS_LAPIC_WRITE(LAPIC_LVTT, LAPIC_DISABLE);
	/* if we accept the extint stuff (the boot processor) we need to not
	 * mask, and set the proper flags for these entries.
	 * LVT1: NMI
	 * LVT0: extINT, level triggered
	 */
	IMPS_LAPIC_WRITE(LAPIC_LVT1, 0x400 | (extint ? 0 : LAPIC_DISABLE)); //NMI
	IMPS_LAPIC_WRITE(LAPIC_LVT0, 0x8700 | (extint ? 0 : LAPIC_DISABLE)); //external interrupts
	/* disable errors (can trigger while messing with masking) and performance
	 * counter, but also set a non-zero vector */
	IMPS_LAPIC_WRITE(LAPIC_LVTE, 0xFF | LAPIC_DISABLE);
	IMPS_LAPIC_WRITE(LAPIC_LVTPC, 0xFF | LAPIC_DISABLE);
	
	/* accept all priority levels */
	IMPS_LAPIC_WRITE(LAPIC_TPR, 0);
	/* finally write to the spurious interrupt register to enable
	 * the interrupts */
	IMPS_LAPIC_WRITE(LAPIC_SPIV, 0x0100 | 0xFF);
}

void id_map_apic(page_dir_t *pd)
{
	int a = PAGE_DIR_IDX(imps_lapic_addr / 0x1000);
	int t = PAGE_TABLE_IDX(imps_lapic_addr / 0x1000);
	pd[a] = pm_alloc_page() | PAGE_PRESENT | PAGE_WRITE;
	unsigned int *pt = (unsigned int *)(pd[a] & PAGE_MASK);
	pt[t] = (imps_lapic_addr&PAGE_MASK) | PAGE_PRESENT | PAGE_WRITE | PAGE_NOCACHE;
}

void init_ioapic()
{
	if(!num_ioapic)
		return;
	unsigned i, num=0;
	set_int(0);
	/* enable all discovered ioapics */
	for(i=0;i<num_ioapic;i++) {
		struct imps_ioapic *l = ioapic_list[i];
		assert(l->type == 2);
		printk(1, "[apic]: found ioapic at %x: ID %d, version %x; flags=%x\n", l->addr, l->id, l->ver, l->flags);
		if(l->flags) num += program_ioapic(l);
	}
	if(!num)
	{
		/* We can still try to use the PIC... */
		kprintf("[apic]: WARNING: no IOAPIC controllers are enabled.\n[apic]: WARNING: this is a non-conforming system.\n");
		kprintf("[apic]: WARNING: using PIC only, advanced interrupt features disabled.\n");
		return;
	}
	else if(imcr_present) {
		outb(0x22, 0x70);
		outb(0x23, 0x01);
	}
	/* ok, disable the PIC */
	disable_pic();
	interrupt_controller = IOINT_APIC;
}
#endif
