#include <sea/fs/inode.h>
#include <sea/errno.h>
#include <sea/lib/hash.h>
#include <sea/cpu/atomic.h>
#include <sea/mm/kmalloc.h>

struct hash_table *icache;
struct llist *ic_inuse;
struct queue *ic_lru;

void vfs_icache_init()
{
	icache = hash_table_create(0, 0, HASH_TYPE_CHAIN);
	hash_table_resize(icache, HASH_RESIZE_MODE_IGNORE,100000);
	hash_table_specify_function(icache, HASH_FUNCTION_BYTE_SUM);

	ic_inuse = ll_create(0);
	ic_lru = queue_create(0, 0);
}

struct dirent *vfs_inode_get_dirent(struct inode *node, const char *name, int namelen)
{
	struct dirent *dir;
	int r = hash_table_get_entry(node->dirents, (void *)name, 1, namelen, (void**)&dir);
	return r == -ENOENT ? 0 : dir;
}

void vfs_inode_add_dirent(struct inode *node, struct dirent *dir)
{
	int r = hash_table_set_entry(node->dirents, dir->name, 1, dir->namelen, dir);
	assert(!r);
}

void vfs_inode_del_dirent(struct inode *node, struct dirent *dir)
{
	int r = hash_table_delete_entry(node->dirents, dir->name, 1, dir->namelen);
	assert(!r);
}

int vfs_inode_check_permissions(struct inode *node, int perm, int real)
{
	return 1;
}

#warning "fix calls to this from other locations"
struct inode *vfs_inode_create()
{
	struct inode *node = kmalloc(sizeof(struct inode));
	rwlock_create(&node->lock);
	
	node->dirents = hash_table_create(0, 0, HASH_TYPE_CHAIN);
	hash_table_resize(node->dirents, HASH_RESIZE_MODE_IGNORE,1000);
	hash_table_specify_function(node->dirents, HASH_FUNCTION_BYTE_SUM);
#warning "TODO: other inits"
	node->flags = INODE_INUSE;
	ll_do_insert(ic_inuse, &node->inuse_item, node);

	return node;
}

void vfs_inode_destroy(struct inode *node)
{
	rwlock_destroy(&node->lock);
	assert(!node->count);
	assert(!node->dirents->count);
	hash_table_destroy(node->dirents);
	assert(!(node->flags & INODE_INUSE));
	/* TODO: test queue */

	kfree(node);
}

void vfs_inode_get(struct inode *node)
{
	assert(add_atomic(&node->count, 1) > 1);
	assert((node->flags & INODE_INUSE));
}

struct inode *vfs_icache_get(struct filesystem *fs, uint32_t num)
{
	/* create if it doesn't exist */
	struct inode *node;
	int newly_created = 0;
	uint32_t key[2] = {fs->id, num};
	if(hash_table_get_entry(icache, key, sizeof(uint32_t), 2, (void**)&node) == -ENOENT) {
	//	kprintf("create new inode\n");
		/* didn't find it. Okay, create one */
		node = vfs_inode_create();
		node->filesystem = fs;
		node->flags = INODE_NEEDREAD;
		node->id = num;
		struct inode *ret;
		hash_table_set_or_get_entry(icache, key, sizeof(uint32_t), 2, node, (void**)&ret);
		if(ret != node) {
			node->flags = node->count = 0;
			vfs_inode_destroy(node);
		} else {
			newly_created = 1;
		}
		/* someone else created this node before us. Okay. */
		node = ret;
	}
#warning "need to lock here? (for reclaiming)"
	add_atomic(&node->count, 1);
#warning "remove these lines"
	/* this was added in because a free'd inode that was allocated again had
	 * old data present. In theory, once the work is compelete, this shouldn't
	 * ever happen. But we need to fix this */
	vfs_inode_set_needread(node);
	fs_inode_pull(node);
	
	/* move to in-use */
	if(!(ff_or_atomic(&node->flags, INODE_INUSE) & INODE_INUSE)) {
	//	kprintf("move to inuse\n");
		if(!newly_created) {
			queue_remove(ic_lru, &node->lru_item);
			ll_do_insert(ic_inuse, &node->inuse_item, node);
		}
	}

	return node;
}

void vfs_inode_set_needread(struct inode *node)
{
	assert(!(node->flags & INODE_DIRTY));
	or_atomic(&node->flags, INODE_NEEDREAD);
	fs_inode_pull(node);
#warning "NO"
}

void vfs_inode_set_dirty(struct inode *node)
{
	assert(!(node->flags & INODE_NEEDREAD));
	or_atomic(&node->flags, INODE_DIRTY);
#warning "NO"
	fs_inode_push(node);
}

void vfs_icache_put(struct inode *node)
{
	assert(node->count > 0);
	fs_inode_push(node); //TODO: Not sure if we want to do this here
	if(!sub_atomic(&node->count, 1)) {
//		kprintf("moving to lru\n");
		assert(node->flags & INODE_INUSE);
		and_atomic(&node->flags, ~INODE_INUSE);
		ll_do_remove(ic_inuse, &node->inuse_item, 0);
		queue_enqueue_item(ic_lru, &node->lru_item, node);
	}
}

void vfs_inode_mount(struct inode *node, struct filesystem *fs)
{
	assert(!node->mount);
	node->mount = fs;
	vfs_inode_get(node);
	fs->point = node;
}

void vfs_inode_umount(struct inode *node)
{
	assert(node->mount);
	node->mount->point = 0;
	node->mount = 0;
	vfs_icache_put(node);
}

int fs_inode_write(struct inode *node, size_t off, size_t count, const char *buf)
{
	//TODO: check not directory, and update mtime.
	return fs_callback_inode_write(node, off, count, buf);
}

int fs_inode_read(struct inode *node, size_t off, size_t count, char *buf)
{
	return fs_callback_inode_read(node, off, count, buf);
}

int vfs_inode_chdir(struct inode *node)
{
	struct inode *old = current_task->thread->pwd;
	current_task->thread->pwd = node;
	vfs_inode_get(node);
	vfs_icache_put(old);
	return 0;
}

int vfs_inode_chroot(struct inode *node)
{
	struct inode *old = current_task->thread->root;
	current_task->thread->root = node;
	vfs_inode_get(node);
	vfs_icache_put(old);
	return 0;
}

int fs_icache_sync()
{
	printk(0, "[fs]: syncing inode cache...");
	rwlock_acquire(&ic_inuse->rwl, RWL_READER);
	struct llistnode *ln;
	struct inode *node;
	ll_for_each_entry(ic_inuse, ln, struct inode *, node) {
		fs_inode_push(node);
	}
	rwlock_release(&ic_inuse->rwl, RWL_READER);
	printk(0, " done\n");
	return 0;
}

