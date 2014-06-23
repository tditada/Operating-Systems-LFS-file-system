#include "fs.h"

//syncs
static void __sync_log_buf();
static void __sync_cr(dptr address);
static void __write(void * data, int bytes, dptr address);
//loads
static void * __load(dptr addr, int bytes);
#define __load_checkpoint(addr) ((checkpoint *)__load(addr, sizeof(checkpoint)))
static lnode * __load_lnode(dptr addr);
#define __LOAD(type, addr) ((type *)(__load_lnode(addr)->data))
#define __load_imap(addr) __LOAD(imap, addr)
#define __load_inode(addr) __LOAD(inode, addr)
#define __load_ddata(addr) __LOAD(ddata, addr)
#define __load_fdata(addr) __LOAD(fdata, addr)

void __sync_cr(dptr address) {
	__write(__cp, sizeof(checkpoint), address);
}

void __sync_log_buf() {
	int i, bytes;
	dptr write_from = __dptr_add(__cp->lend, -__log_buf_size);
	for (i=0; __log_buf_size-(i*SECTOR_SIZE)>0; i++) {
		bytes = min(__log_buf_size-(i*SECTOR_SIZE), SECTOR_SIZE);
		__write(__log_buf+i*SECTOR_SIZE, bytes, __dptr_add(write_from, i*SECTOR_SIZE));
		//TODO: Ver que no se esta haciendo de manera circular el tema del lend!!!!!!
	}
	__set_dptr(__cp->lend, __cp->lstart);
	for (i=0; i<__log_buf_size; i++) {
		__log_buf_list[i] = NULL;
	}
	__log_buf_size = 0;
}

void __write(void * data, int bytes, dptr address) {
	ata_write(__drive, data, bytes, address.sector, address.offset);
}

dptr __dptr_add(dptr address, int bytes) {
	int _bytes = address.sector * SECTOR_SIZE + address.offset + bytes;
	return __new_dptr(_bytes/SECTOR_SIZE, _bytes%SECTOR_SIZE);
}

void * __load(dptr addr, int bytes) {
	void * data = malloc(bytes);
	ata_read(__drive, data, bytes, addr.sector, addr.offset);
	return data;
}

lnode * __load_lnode(dptr addr) {
	lnode * data = malloc(MAX_LNODE_SIZE);
	ata_read(__drive, (void *) data, MAX_LNODE_SIZE, addr.sector, addr.offset);

	int size = __sizeof_lntype(((int *)data)[0]);
	free(data + size);
	//or else you'll read garbage

	return data;
}

void __stage(lntype type, void * data) {
	dptr new_end = __dptr_add(__cp->lend, __sizeof_lntype(type));
	__append_to_buf(__new_lnode(type, data, new_end), __sizeof_lntype(type));
	__set_dptr(__cp->lend, new_end);
}

void __append_to_buf(void * data, int bytes) {
	memcpy(__log_buf+__log_buf_size, data, bytes);
	__log_buf_size += bytes;
}

lnode * __new_lnode(lntype type, void * data, dptr next) {
	int size = __sizeof_lntype(type);
	lnode * lnptr = malloc(sizeof(lnode)+size);
	lnptr->type = type;
	__set_dptr(lnptr->next, next);
	memcpy(lnptr->data, data, size);
	return lnptr;
}

int __sizeof_lntype(lntype type) {
	/*printk("\nlntype: %s\n", __lntype_to_str(type));*/
	switch(type) {
	case FS_IMAP:
		return sizeof(imap);
	case FS_INODE:
		return sizeof(inode);
	case FS_DDATA:
		return sizeof(ddata);
	case FS_FDATA:
		return sizeof(fdata);
	//TODO: check for more cases!
	}
	return -1;//CHECK!
}