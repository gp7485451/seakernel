#ifndef __SEA_FS_INODE_H
#define __SEA_FS_INODE_H

#include <sea/types.h>
#include <sea/mutex.h>
#include <sea/rwlock.h>
#include <sea/ll.h>
#include <sea/fs/flock.h>
#include <sea/fs/stat.h>
#include <sea/lib/hash.h>

#define MAY_EXEC      0100
#define MAY_WRITE     0200
#define MAY_READ      0400

#define INAME_LEN 256

#define inode_has_children(i) (i->children.head && ll_is_active((&i->children)))

typedef struct {
	struct inode *root;
	struct inode *parent;
} mount_pt_t;

typedef struct pipe_struct pipe_t;

struct inode {
	/* Attributes */
	mode_t mode;
	uid_t uid;
	gid_t gid;
	unsigned short nlink;
	unsigned char dynamic, marked_for_deletion;
	unsigned int flags;
	off_t len;
	addr_t start;
	unsigned int nblocks;
	time_t ctime, atime, mtime;
	int count, f_count, newlocks;
	size_t blksize;
	/* Identification */
	char name[INAME_LEN];
	dev_t dev;
	unsigned long num;
	unsigned int sb_idx, fs_idx;
	char node_str[INAME_LEN];
	/* Pointers */
	struct inode_operations *i_ops;
	struct inode *parent;
	struct llist children;
	struct llistnode *node;
	struct inode *mount_parent;
	pipe_t *pipe;
	mount_pt_t *mount;
	/* Locking */
	rwlock_t rwl;
	struct flock *flocks;
	mutex_t *flm;

	/* shared mmappings */
	struct hash_table *physicals;
	mutex_t mappings_lock;
	int mapped_pages_count, mapped_entries_count;
};

struct inode_operations {
	int (*read) (struct inode *, off_t, size_t, char *);
	int (*write) (struct inode *, off_t, size_t, char *);
	int (*select) (struct inode *, unsigned int);
	struct inode *(*create) (struct inode *,char *, mode_t);
	struct inode *(*lookup) (struct inode *,char *);
	struct inode *(*readdir) (struct inode *, unsigned);
	int (*link) (struct inode *, char *);
	int (*unlink) (struct inode *);
	int (*rmdir) (struct inode *);
	int (*sync_inode) (struct inode *);
	int (*unmount)(struct inode *, unsigned int);
	int (*fsstat)(struct inode *, struct posix_statfs *);
	int (*fssync)(struct inode *);
	int (*update)(struct inode *);
};

#define iremove_recur(i)  vfs_do_iremove(i, 2, 0)
#define iremove(i)        vfs_do_iremove(i, 0, 0)
#define iremove_nofree(i) vfs_do_iremove(i, 3, 0)
#define iremove_force(i)  vfs_do_iremove(i, 1, 0)

#define vfs_get_idir(path,in_st)         vfs_do_get_idir(path, in_st, 0, 0, 0)
#define vfs_lget_idir(path,in_st)        vfs_do_get_idir(path, in_st, 1, 0, 0)
#define vfs_clget_idir(path,in_st,x)     vfs_do_get_idir(path, in_st, 1, x, 0)
#define vfs_cget_idir(path,in_st,x)      vfs_do_get_idir(path, in_st, 1, x, 0)
#define vfs_ctget_idir(path,in_st,x,res) vfs_do_get_idir(path, in_st, 1, x, res)

#define vfs_add_inode(a,b) vfs_do_add_inode(a, b, 0)

struct inode *vfs_do_get_idir(char *p_path, struct inode *b, int use_link, 
	int create, int *did_create);

int vfs_sync_inode_tofs(struct inode *i);
struct inode *sys_create(char *path);
int vfs_link(char *old, char *);
int vfs_unlink(char *f);
int vfs_do_unlink(struct inode *i);
int vfs_rmdir(char *f);

int vfs_inode_is_directory(struct inode *i);
int vfs_inode_get_ref_count(struct inode *i);
int vfs_inode_get_check_permissions(struct inode *i, mode_t flag, int use_real_id);
int vfs_do_add_inode(struct inode *b, struct inode *i, int locked);
int vfs_do_iremove(struct inode *i, int flag, int locked);
int vfs_free_inode(struct inode *i, int recur);

int vfs_read_inode(struct inode *i, off_t off, size_t  len, char *b);
int vfs_write_inode(struct inode *i, off_t off, size_t len, char *b);

int sys_getdepth(int fd);
int sys_getcwdlen();
int sys_fcntl(int filedes, int cmd, long attr1, long attr2, long attr3);
int sync_inode_tofs(struct inode *i);

#define FS_INODE_POPULATE 1
addr_t fs_inode_map_shared_physical_page(struct inode *node, addr_t virt, 
		size_t offset, int flags, int attrib);
addr_t fs_inode_map_private_physical_page(struct inode *node, addr_t virt,
		size_t offset, int attrib, size_t);
void fs_inode_map_region(struct inode *node, size_t offset, size_t length);
void fs_inode_sync_physical_page(struct inode *node, addr_t virt, size_t offset, size_t);
void fs_inode_unmap_region(struct inode *node, addr_t virt, size_t offset, size_t length);
void fs_inode_destroy_physicals(struct inode *node);

#endif
