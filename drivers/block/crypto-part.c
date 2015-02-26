#include <sea/dm/block.h>
#include <sea/loader/module.h>
#include <sea/fs/inode.h>
#include <modules/psm.h>
#include <sea/ll.h>
#include <modules/aes.h>
#include <sea/errno.h>
#include <sea/vsprintf.h>
#include <sea/string.h>

#define MAX_CRYPTO 32
int cp_major;
struct cp_ioctl_arg {
	char *devname;
	int keylength;
	unsigned char *key;
};
struct crypt_part {
	int psm_minor;
	int used;
	dev_t dev;
	struct cp_ioctl_arg kd;
};

struct crypt_part list[MAX_CRYPTO];

struct crypt_part *crypto_create_dev(dev_t dev, struct cp_ioctl_arg *ia)
{
	int i;
	for(i=0;i<MAX_CRYPTO;i++)
	{
		if(!list[i].used) {
			list[i].used = 1;
			break;
		}
	}
	if(i == MAX_CRYPTO)
		return 0;
	
	struct disk_info di;
	di.length=0;
	di.num_sectors=0;
	di.sector_size=512;
	di.flags = PSM_DISK_INFO_NOPART;
	int psm_minor = psm_register_disk_device(PSM_CRYPTO_PART_ID, GETDEV(cp_major, i), &di);
	list[i].psm_minor = psm_minor;
	list[i].dev = dev;
	if(ia) memcpy(&(list[i].kd), ia, sizeof(struct cp_ioctl_arg));
	return &list[i];
}

int cp_rw_multiple(int rw, int min, u64 blk, char *buf, int count)
{
	int i, total=0;
	unsigned char tmp[512];
	if(!min) return 0;
	for(i=0;i<count;i++) {
		/* BUG/TODO: Handle non 512 sector sizes */
		if(rw == WRITE) {
			unsigned char rk[176];
			int n = aes_setup_encrypt(list[min].kd.key, rk, list[min].kd.keylength*8);
			int j;
			for(j=0;j<512;j+=16)
				aes_encrypt_block(rk, n, (unsigned char*)buf+i*512 + j, tmp+j);
		}
		int r = dm_do_block_rw(rw, list[min].dev, blk+i, (char *)tmp, 0);
		if(rw == READ) {
			unsigned char rk[176];
			int n = aes_setup_decrypt(list[min].kd.key, rk, list[min].kd.keylength*8);
			int j;
			for(j=0;j<512;j+=16)
				aes_decrypt_block(rk, n, tmp+j, (unsigned char*)buf+i*512 + j);
		}
		if(r <= 0) return r;
		total+=r;
	}
	return total;
}

int cp_rw_single(int rw, int min, u64 blk, char *buf)
{
	return cp_rw_multiple(rw, min, blk, buf, 1);
}

int cp_ioctl(int min, int cmd, long arg)
{
	struct cp_ioctl_arg *ia = (struct cp_ioctl_arg *)arg;
	if(cmd == 1) {
		int err;
		struct inode *node = fs_path_resolve_inode(ia->devname, 0, &err);
		if(!node) return err;
		printk(1, "[crypt-part]: creating crypto device for %s (dev=%x)\n", ia->devname, node->phys_dev);
		if(crypto_create_dev(node->phys_dev, ia))
			return 0;
	}
	return -EINVAL;
}

int module_install()
{
	memset(list, 0, sizeof(list));
	cp_major = dm_set_available_block_device(cp_rw_single, 512, cp_ioctl, cp_rw_multiple, 0);
	crypto_create_dev(0, 0);
	return 0;
}

int module_exit()
{
	
	
	return 0;
}
