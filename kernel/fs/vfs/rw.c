#include <sea/mm/vmm.h>
#include <sea/tm/process.h>
#include <sea/asm/system.h>
#include <sea/dm/dev.h>
#include <sea/fs/inode.h>
#include <sea/fs/inode.h>
#include <sea/fs/callback.h>
#include <sea/errno.h>
int fs_inode_write(struct inode *i, off_t off, size_t len, char *b)
{
	if(!i || !b)
		return -EINVAL;
	if(vfs_inode_is_directory(i))
		return -EISDIR;
	if(!vfs_inode_check_permissions(i, MAY_WRITE, 0))
		return -EACCES;
	int r = vfs_callback_write(i, off, len, b);
	i->mtime = time_get_epoch();
	sync_inode_tofs(i);
	return r;
}

int fs_inode_read(struct inode *i, off_t off, size_t  len, char *b)
{
	if(!i || !b)
		return -EINVAL;
	if(!vfs_inode_check_permissions(i, MAY_READ, 0))
		return -EACCES;
	return vfs_callback_read(i, off, len, b);
}
