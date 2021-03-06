#ifndef __SEA_FS_DIR_H
#define __SEA_FS_DIR_H

#include <sea/fs/inode.h>

int sys_get_pwd(char *buf, int sz);
int vfs_get_path_string(struct inode *p, char *buf, int len);
int vfs_chroot(char *n);
int vfs_chdir(char *path);
struct inode *vfs_read_dir(char *n, int num);
int vfs_directory_is_empty(struct inode *i);
int vfs_ichdir(struct inode *i);
struct inode *vfs_read_idir(struct inode *i, int num);

int sys_chdir(char *n, int fd);
int sys_dirstat(char *dir, unsigned num, char *namebuf, struct stat *statbuf);
int sys_getpath(int f, char *b, int);
int sys_dirstat_fd(int fd, unsigned num, char *namebuf, struct stat *statbuf);

#endif
