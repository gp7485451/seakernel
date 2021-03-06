#include <sea/asm/system.h>
#include <sea/mutex.h>
#include <sea/tty/terminal.h>

extern struct console_driver crtc_drv;
#define VIDEO_MEMORY 0xb8000

void arch_console_init_stage1()
{
	kernel_console->vmem=kernel_console->cur_mem
						=kernel_console->video=(char *)VIDEO_MEMORY;
	console_initialize_vterm(kernel_console, &crtc_drv);
}
