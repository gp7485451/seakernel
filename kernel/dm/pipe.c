#include <sea/dm/dev.h>
#include <sea/fs/inode.h>
#include <sea/dm/pipe.h>
#include <sea/tm/process.h>
#include <sea/fs/file.h>
#include <sea/cpu/interrupt.h>
#include <sea/tm/schedule.h>
#include <sea/sys/fcntl.h>
#include <sea/errno.h>
#include <sea/mm/kmalloc.h>
#include <sea/string.h>

pipe_t *dm_create_pipe()
{
	pipe_t *pipe = (pipe_t *)kmalloc(sizeof(pipe_t));
	pipe->length = PIPE_SIZE;
	pipe->buffer = (char *)kmalloc(PIPE_SIZE+1);
	pipe->lock = mutex_create(0, 0);
	pipe->read_blocked = ll_create(0);
	pipe->write_blocked = ll_create(0);
	return pipe;
}

static struct inode *create_anon_pipe()
{
	struct inode *node;
	/* create a 'fake' inode */
	node = vfs_inode_create();
	node->uid = current_task->thread->effective_uid;
	node->gid = current_task->thread->effective_gid;
	node->mode = S_IFIFO | 0x1FF;
	node->count=2;
	
	pipe_t *pipe = dm_create_pipe();
	pipe->count=2;
	pipe->wrcount=1;
	node->pipe = pipe;
	/* TODO: DYNAMIC? */
	return node;
}

int sys_pipe(int *files)
{
	if(!files) return -EINVAL;
	struct file *f;
	struct inode *inode = create_anon_pipe();
	/* this is the reading descriptor */
	f = (struct file *)kmalloc(sizeof(struct file));
	f->inode = inode;
	f->flags = _FREAD;
	f->pos=0;
	f->count=1;
	int read = fs_add_file_pointer((task_t *)current_task, f);
	/* this is the writing descriptor */
	f = (struct file *)kmalloc(sizeof(struct file));
	f->inode = inode;
	f->flags = _FREAD | _FWRITE;
	f->count=1;
	f->pos=0;
	int write = fs_add_file_pointer((task_t *)current_task, f);
	files[0]=read;
	files[1]=write;
	fs_fput((task_t *)current_task, read, 0);
	fs_fput((task_t *)current_task, write, 0);
	return 0;
}

void dm_free_pipe(struct inode *i)
{
	if(!i || !i->pipe) return;
	kfree((void *)i->pipe->buffer);
	mutex_destroy(i->pipe->lock);
	ll_destroy(i->pipe->read_blocked);
	ll_destroy(i->pipe->write_blocked);
	kfree(i->pipe);
	i->pipe=0;
}

int dm_read_pipe(struct inode *ino, int flags, char *buffer, size_t length)
{
	if(!ino || !buffer)
		return -EINVAL;
	pipe_t *pipe = ino->pipe;
	if(!pipe)
		return -EINVAL;
	size_t len = length;
	int ret=0;
	size_t count=0;
	if((flags & _FNONBLOCK) && !pipe->pending)
		return -EAGAIN;
	/* should we even try reading? (empty pipe with no writing processes=no) */
	if(!pipe->pending && pipe->count <= 1 && pipe->type != PIPE_NAMED)
		return count;
	/* block until we have stuff to read */
	mutex_acquire(pipe->lock);
	while(!pipe->pending && (pipe->count > 1 && pipe->type != PIPE_NAMED 
			&& pipe->wrcount>0)) {
		/* we need to block, but also release the lock. Disable interrupts
		 * so we don't schedule before we want to */
		int old = cpu_interrupt_set(0);
		tm_add_to_blocklist(pipe->read_blocked, (task_t *)current_task);
		mutex_release(pipe->lock);
		while(!tm_schedule());
		cpu_interrupt_set(old);
		if(tm_process_got_signal(current_task))
			return -EINTR;
		mutex_acquire(pipe->lock);
	}
	ret = pipe->pending > len ? len : pipe->pending;
	/* note: this is a quick implementation of line-buffering that should
	 * work for most cases. There is currently no way to disable line
	 * buffering in pipes, but I don't care, because there shouldn't be a
	 * reason to. */
	char *nl = strchr((char *)pipe->buffer+pipe->read_pos, '\n');
	if(nl && (nl-(pipe->buffer+pipe->read_pos)) < ret)
		ret = (nl-(pipe->buffer+pipe->read_pos))+1;
	memcpy((void *)(buffer + count), (void *)(pipe->buffer + pipe->read_pos), ret);
	memcpy((void *)pipe->buffer, (void *)(pipe->buffer + pipe->read_pos + ret), 
		PIPE_SIZE - (pipe->read_pos + ret));
	if(ret > 0) {
		pipe->pending -= ret;
		pipe->write_pos -= ret;
		len -= ret;
		count+=ret;
	}
	tm_remove_all_from_blocklist(pipe->write_blocked);
	tm_remove_all_from_blocklist(pipe->read_blocked);
	mutex_release(pipe->lock);
	return count;
}

int dm_write_pipe(struct inode *ino, int flags, char *initialbuffer, size_t totallength)
{
	if(!ino || !initialbuffer)
		return -EINVAL;
	pipe_t *pipe = ino->pipe;
	if(!pipe)
		return -EINVAL;
	/* allow for partial writes of the system page size. Thus, we wont
	 * have a process freeze because it tries to fill up the pipe in one
	 * shot. */
	char *buffer = initialbuffer;
	size_t length;
	size_t remain = totallength;
	mutex_acquire(pipe->lock);
	if((flags & _FNONBLOCK) && pipe->write_pos + totallength > PIPE_SIZE) {
		mutex_release(pipe->lock);
		return -EAGAIN;
	}
	while(remain) {
		length = PAGE_SIZE;
		if(length > remain)
			length = remain;
		/* we're writing to a pipe with no reading process! */
		if((pipe->count - pipe->wrcount) == 0 && pipe->type != PIPE_NAMED) {
			mutex_release(pipe->lock);
			current_task->sigd = SIGPIPE;
			return -EPIPE;
		}
		/* IO block until we can write to it */
		while((pipe->write_pos+length)>=PIPE_SIZE) {
			int old = cpu_interrupt_set(0);
			tm_remove_all_from_blocklist(pipe->read_blocked);
			tm_add_to_blocklist(pipe->write_blocked, (task_t *)current_task);
			mutex_release(pipe->lock);
			while(!tm_schedule());
			cpu_interrupt_set(old);
			if(tm_process_got_signal(current_task))
				return -EINTR;
			mutex_acquire(pipe->lock);
		}
		
		memcpy((void *)(pipe->buffer + pipe->write_pos), buffer, length);
		pipe->length = ino->length;
		pipe->write_pos += length;
		pipe->pending += length;
		/* now, unblock the tasks */
		tm_remove_all_from_blocklist(pipe->read_blocked);
		tm_remove_all_from_blocklist(pipe->write_blocked);

		remain -= length;
		buffer += length;
	}
	mutex_release(pipe->lock);
	return totallength;
}

int dm_pipedev_select(struct inode *in, int rw)
{
	if(rw != READ)
		return 1;
	pipe_t *pipe = in->pipe;
	if(!pipe) return 1;
	if(!pipe->pending && (pipe->count > 1 && pipe->type != PIPE_NAMED 
			&& pipe->wrcount>0))
		return 0;
	return 1;
}
