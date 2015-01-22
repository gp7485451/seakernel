#include <sea/kernel.h>
#include <sea/syscall.h>
#include <sea/cpu/interrupt.h>
#include <sea/tm/process.h>
#include <sea/dm/dev.h>
#include <sea/fs/inode.h>
#include <sea/sys/stat.h>
#include <sea/loader/module.h>
#include <sea/sys/sysconf.h>
#include <sea/mm/swap.h>
#include <sea/cpu/processor.h>
#include <sea/syscall.h>
#include <sea/uname.h>
#include <sea/fs/mount.h>
#include <sea/tty/terminal.h>
#include <sea/fs/stat.h>
#include <sea/fs/dir.h>
#include <sea/tm/schedule.h>
#include <sea/cpu/interrupt.h>
#include <sea/dm/pipe.h>
#include <sea/loader/exec.h>
#include <sea/cpu/atomic.h>
#include <sea/mm/map.h>
#include <sea/errno.h>
#include <sea/vsprintf.h>
#include <sea/fs/socket.h>

static unsigned int num_syscalls=0;
//#define SC_DEBUG 1
int sys_null(long a, long b, long c, long d, long e)
{
	#if CONFIG_DEBUG
	kprintf("[kernel]: Null system call (%d) called in task %d\n%x %x %x %x %x",
			current_task->system, current_task->pid, a, b, c, d, e);
	#endif
	return -ENOSYS;
}

int sys_gethost(int a, int b, int c, int d, int e)
{
	/* Remember to add systemcall buffer checks... */
	return -ENOSYS;
}

int sys_getserv(int a, int b, int c, int d, int e)
{
	return -ENOSYS;
}

int sys_setserv(int a, int b, int c, int d, int e)
{
	return -ENOSYS;
}

int sys_syslog(int level, char *buf, int len, int ctl)
{
	if(ctl)
		return 0;
	printk(level, "%s", buf);
	return 0;
}

static void *syscall_table[129] = {
	[SYS_SETUP]           = SC sys_setup,
	[SYS_EXIT]            = SC tm_exit,
	[SYS_FORK]            = SC tm_do_fork,
	[SYS_WAIT]            = SC tm_process_wait,
	[SYS_READ]            = SC sys_readpos,
	[SYS_WRITE]           = SC sys_writepos,
	[SYS_OPEN]            = SC sys_open_posix,
	[SYS_CLOSE]           = SC sys_close,
	[SYS_FSTAT]           = SC sys_fstat,
	[SYS_STAT]            = SC sys_stat,
	[SYS_ISATTY]          = SC sys_isatty,
	[SYS_SEEK]            = SC sys_seek,
	[SYS_SIGNAL]          = SC tm_send_signal,
	[SYS_SBRK]            = SC sys_sbrk,
	[SYS_TIMES]           = SC sys_times,
	[SYS_DUP]             = SC sys_dup,
	[SYS_DUP2]            = SC sys_dup2,
	[SYS_IOCTL]           = SC sys_ioctl,
	[SYS_VFORK]           = SC sys_vfork,
	[SYS_RECV]            = SC sys_recv,
	[SYS_SEND]            = SC sys_send,
	[SYS_SOCKET]          = SC sys_socket,
	[SYS_ACCEPT]          = SC sys_accept,
	[SYS_CONNECT]         = SC sys_connect,
	[SYS_LISTEN]          = SC sys_listen,
	[SYS_BIND]            = SC sys_bind,
	[SYS_EXECVE]          = SC execve,
	#if CONFIG_MODULES
	[SYS_LMOD]            = SC sys_load_module,
	[SYS_ULMOD]           = SC sys_unload_module,

	[SYS_ULALLMODS]       = SC loader_unload_all_modules,
	#else
	[SYS_LMOD]            = SC sys_null,
	[SYS_ULMOD]           = SC sys_null,

	[SYS_ULALLMODS]       = SC sys_null,
	#endif
	[SYS_GETPID]          = SC sys_get_pid,
	[SYS_GETPPID]         = SC sys_getppid,
	[SYS_LINK]            = SC sys_link,
	[SYS_UNLINK]          = SC sys_unlink,
	//[SYS_GETREFCOUNT]     = SC vfs_inode_get_ref_count,
	//[SYS_GETPWD]          = SC sys_get_pwd,
	//[SYS_GETPATH]         = SC sys_getpath,
	[SYS_GETSOCKNAME]     = SC sys_getsockname,
	[SYS_CHROOT]          = SC sys_chroot,
	[SYS_CHDIR]           = SC sys_chdir,
	[SYS_MOUNT]           = SC sys_mount,
	[SYS_UMOUNT]          = SC sys_umount,
	//[SYS_READDIR]         = SC vfs_read_dir,
	[SYS_MKDIR]           = SC sys_mkdir,

	[SYS_CREATE_CONSOLE]  = SC console_create,
	[SYS_SWITCH_CONSOLE]  = SC console_switch,
	[SYS_SOCKSHUTDOWN]    = SC sys_sockshutdown,
	[SYS_GETSOCKOPT]      = SC sys_getsockopt,
	[SYS_SETSOCKOPT]      = SC sys_setsockopt,
	[SYS_RECVFROM]        = SC sys_recvfrom,
	[SYS_SENDTO]          = SC sys_sendto,
	[SYS_SYNC]            = SC sys_sync,
	[SYS_RMDIR]           = SC sys_rmdir,
	[SYS_FSYNC]           = SC sys_fsync,
	[SYS_ALARM]           = SC sys_alarm,
	[SYS_SELECT]          = SC sys_select,
	[SYS_GETDENTS]        = SC sys_getdents,

	[SYS_SYSCONF]         = SC sys_sysconf,
	[SYS_SETSID]          = SC sys_setsid,
	[SYS_SETPGID]         = SC sys_setpgid,
	#if CONFIG_SWAP
	[SYS_SWAPON]          = SC sys_swapon,
	[SYS_SWAPOFF]         = SC sys_swapoff,
	#else
	[SYS_SWAPON]          = SC sys_null,
	[SYS_SWAPOFF]         = SC sys_null,
	#endif
	[SYS_NICE]            = SC sys_nice,
	[SYS_MMAP]            = SC sys_mmap,
	[SYS_MUNMAP]          = SC sys_munmap,
	[SYS_MSYNC]           = SC sys_msync,
	[SYS_TSTAT]           = SC sys_task_stat,

	[SYS_DELAY]           = SC tm_delay,
	[SYS_KRESET]          = SC kernel_reset,
	[SYS_KPOWOFF]         = SC kernel_poweroff,
	[SYS_GETUID]          = SC tm_get_uid,
	[SYS_GETGID]          = SC tm_get_gid,
	[SYS_SETUID]          = SC tm_set_uid,
	[SYS_SETGID]          = SC tm_set_gid,
	[SYS_MEMSTAT]         = SC sys_null,
	[SYS_TPSTAT]          = SC sys_task_pstat,
	[SYS_MOUNT2]          = SC sys_mount2,
	[SYS_SETEUID]         = SC tm_set_euid,
	[SYS_SETEGID]         = SC tm_set_egid,
	[SYS_PIPE]            = SC sys_pipe,
	[SYS_SETSIG]          = SC tm_set_signal,
	[SYS_GETEUID]         = SC tm_get_euid,
	[SYS_GETEGID]         = SC tm_get_egid,
	[SYS_GETTIMEEPOCH]    = SC time_get_epoch,

	[SYS_GETTIME]         = SC time_get,
	[SYS_TIMERTH]         = SC sys_get_timer_th,
	[SYS_ISSTATE]         = SC sys_isstate,
	[SYS_WAIT3]           = SC sys_wait3,


	//[SYS_GETCWDLEN]       = SC sys_getcwdlen,
	#if CONFIG_SWAP
	[SYS_SWAPTASK]        = SC /**96*/sys_swaptask,
	#else
	[SYS_SWAPTASK]        = SC /**96*/sys_null,
	#endif
	//[SYS_DIRSTAT]         = SC sys_dirstat,
	[SYS_SIGACT]          = SC sys_sigact,
	[SYS_ACCESS]          = SC sys_access,
	[SYS_CHMOD]           = SC sys_chmod,
	[SYS_FCNTL]           = SC sys_fcntl,
	//[SYS_DIRSTATFD]       = SC sys_dirstat_fd,
	//[SYS_GETDEPTH]        = SC sys_getdepth,
	[SYS_WAITPID]         = SC sys_waitpid,
	[SYS_MKNOD]           = SC sys_mknod,
	[SYS_SYMLINK]         = SC sys_symlink,
	[SYS_READLINK]        = SC sys_readlink,
	[SYS_UMASK]           = SC sys_umask,
	[SYS_SIGPROCMASK]     = SC sys_sigprocmask,
	[SYS_FTRUNCATE]       = SC sys_ftruncate,
	//[SYS_GETNODESTR]      = SC sys_getnodestr,
	[SYS_CHOWN]           = SC sys_chown,
	[SYS_UTIME]           = SC sys_utime,
	[SYS_GETHOSTNAME]     = SC sys_gethostname,
	[SYS_GSPRI]           = SC sys_gsetpriority,
	[SYS_UNAME]           = SC sys_uname,
	[SYS_GETHOST]         = SC sys_gethost,
	[SYS_GETSERV]         = SC sys_getserv,
	[SYS_SETSERV]         = SC sys_setserv,
	[SYS_SYSLOG]          = SC sys_syslog,
	[SYS_POSFSSTAT]       = sys_posix_fsstat,





	[SYS_WAITAGAIN] = SC sys_waitagain,
	[SYS_RET_FROM_SIG] = SC sys_null,
};

void syscall_init()
{
	num_syscalls = sizeof(syscall_table)/sizeof(void *);
}

int mm_is_valid_user_pointer(int num, void *p, char flags)
{
	addr_t addr = (addr_t)p;
	if(!addr && !flags) return 0;
	if(!(kernel_state_flags & KSF_HAVEEXECED))
		return 1;
	if(addr < TOP_LOWER_KERNEL && addr) {
		#if CONFIG_DEBUG
		printk(0, "[kernel]: warning - task %d passed ptr %x to syscall %d (invalid)\n",
			   current_task->pid, addr, num);
		#endif
		return 0;
	}
	if(addr >= TOP_TASK_MEM) {
		#if CONFIG_DEBUG
		printk(0, "[kernel]: warning - task %d passed ptr %x to syscall %d (invalid)\n",
			   current_task->pid, addr, num);
		#endif
		return 0;
	}
	return 1;
}

/* here we test to make sure that the task passed in valid pointers
 * to syscalls that have pointers are arguments, so that we make sure
 * we only ever modify user-space data when we think we're modifying
 * user-space data. */
int check_pointers(volatile registers_t *regs)
{
	switch(SYSCALL_NUM_AND_RET) {
		case SYS_READ: case SYS_FSTAT: case SYS_STAT: /*case SYS_GETPATH:*/
		case SYS_READLINK: /*case SYS_GETNODESTR:*/
		case SYS_POSFSSTAT: case SYS_WRITE:
			return mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_B_, 0);

		case SYS_TIMES: /*case SYS_GETPWD:*/ case SYS_PIPE:
		case SYS_MEMSTAT: case SYS_GETTIME: case SYS_GETHOSTNAME:
		case SYS_UNAME: case SYS_MSYNC: case SYS_MUNMAP:
			return mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_A_, 0);

		case SYS_SETSIG: case SYS_WAITPID:
			return mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_B_, 1);

		case SYS_SELECT:
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_B_, 1))
				return 0;
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_C_, 1))
				return 0;
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_D_, 1))
				return 0;
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_E_, 1))
				return 0;
			break;

		//case SYS_DIRSTAT:
		//	if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_A_, 0))
		//		return 0;
		/* fall through *
		case SYS_DIRSTATFD:
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_C_, 0))
				return 0;
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_D_, 0))
				return 0;
			break;
			*/

		case SYS_SIGACT: case SYS_SIGPROCMASK:
			return mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_C_, 1);

		case SYS_CHOWN: case SYS_CHMOD: case SYS_TIMERTH: case SYS_CHDIR: case SYS_CHROOT:
			return mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_A_, 1);

		case SYS_LMOD:
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_B_, 1))
				return 0;
		/* fall through */
		case SYS_ULMOD: case SYS_OPEN:
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_A_, 0))
				return 0;
			break;
		case SYS_MMAP:
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_A_, 1))
				return 0;
			if(!mm_is_valid_user_pointer(SYSCALL_NUM_AND_RET, (void *)_B_, 0))
				return 0;
			break;
		//default:
		//	printk(0, ":: UNTESTED SYSCALL: %d\n", SYSCALL_NUM_AND_RET);
	}
	return 1;
}

int syscall_handler(volatile registers_t *regs)
{
	assert(current_task->magic == TASK_MAGIC);
	assert(current_task->thread->magic == THREAD_MAGIC);
	/* SYSCALL_NUM_AND_RET is defined to be the correct register in the syscall regs struct. */
	if(unlikely(SYSCALL_NUM_AND_RET >= num_syscalls))
		return -ENOSYS;
	if(unlikely(!syscall_table[SYSCALL_NUM_AND_RET]))
		return -ENOSYS;
	volatile long ret;
	if(!check_pointers(regs))
		return -EINVAL;
	if(kernel_state_flags & KSF_SHUTDOWN)
		for(;;);
	tm_process_enter_system(SYSCALL_NUM_AND_RET);
	/* most syscalls are pre-emptible, so we enable interrupts and
	 * expect handlers to disable them if needed */
	cpu_interrupt_set(1);
	/* start accounting information! */
	current_task->freed = current_task->allocated=0;

	#ifdef SC_DEBUG__
	if(current_task->tty == current_console->tty && SYSCALL_NUM_AND_RET != 5)
		printk(SC_DEBUG, "tty %d: syscall %d (from: %x): enter %d\n",
				current_task->tty, current_task->pid,
				current_task->sysregs->eip, SYSCALL_NUM_AND_RET);
	int or_t = tm_get_ticks();
	#endif
	__do_syscall_jump(ret, syscall_table[SYSCALL_NUM_AND_RET], _E_, _D_,
					  _C_, _B_, _A_);
	#ifdef SC_DEBUG
	if((current_task->tty == current_console->tty || 1)
		&& (ret < 0 || 1) && (ret == -EINTR || 1)
		&& ((current_task->allocated != 0 || current_task->freed != 0 || 1))
		&& SYSCALL_NUM_AND_RET != 4)
		printk(SC_DEBUG, "syscall %d: %d ret %d, took  ticks (%d al, %d fr)\n",
			   current_task->pid, current_task->system, ret, 
			   current_task->allocated, current_task->freed);
	#endif

	cpu_interrupt_set(0);
	tm_process_exit_system();
	tm_engage_idle();
	/* if we need to reschedule, or we have overused our timeslice
	 * then we need to reschedule. this prevents tasks that do a continuous call
	 * to write() from starving the resources of other tasks. syscall_count resets
	 * on each call to tm_tm_schedule() */
	if(current_task->flags & TF_SCHED
		|| (unsigned)(tm_get_ticks() -current_task->slice) > (unsigned)current_task->cur_ts
		|| ++current_task->syscall_count > 2)
	{
		/* clear out the flag. Either way in the if statement, we've rescheduled. */
		tm_lower_flag(TF_SCHED);
		tm_schedule();
	}
	/* store the return value in the regs */
	SYSCALL_NUM_AND_RET = ret;
	/* if we're going to jump to a signal here, we need to back up our
	 * return value so that when we return to the system we have the
	 * original systemcall returning the proper value.
	 */
	if((current_task->flags & TF_INSIG) && (current_task->flags & TF_JUMPIN)) {
#if CONFIG_ARCH == TYPE_ARCH_X86
		current_task->reg_b.eax=ret;
#elif CONFIG_ARCH == TYPE_ARCH_X86_64
		current_task->reg_b.rax=ret;
#endif
		tm_lower_flag(TF_JUMPIN);
	}
	return ret;
}

