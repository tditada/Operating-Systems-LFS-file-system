#include "fs.h"

//debugging
static void __print_checkpoint(checkpoint * cp);
static void __print_dptr(dptr * addr);
static void __print_imap(imap * imap);
static void __print_inode(inode * inode);
static void __print_ddata(ddata * ddata);
static void __print_fdata(fdata * fdata);
static void __print_lnode(lnode * lnode);
static void __print_cr_entry(cr_entry * entry);
static char * __lntype_to_str(lntype type);
static char * __ftype_to_str(ftype type);

void __print_checkpoint(checkpoint * cp) {
	printk("checkpoint:{\n\tlstart: ");
	__print_dptr(&(__cp->lstart));
	printk("\n\tlend: ");
	__print_dptr(&(__cp->lend));
	printk("\n}");
}

void __print_dptr(dptr * addr) {
	printk("dptr:{ sector:%d, offset:%d } (#%d d)", addr->sector, addr->offset, addr->sector*SECTOR_SIZE+addr->offset);
}

void __print_imap(imap * imptr) {
	int i;
	printk("imap:{\n");
	for (i=0; i<MAX_IMAP && !__is_null(imptr->map[i].inode); i++) {
		printk("\t{ inoden:%d, inode: ", imptr->map[i].inoden);
		__print_dptr(&imptr->map[i].inode);
		printk("}\n");
	}
	printk("\n}");
}

void __print_cr_entry(cr_entry * entry) {
	printk("cr_entry:{\n\tinoden: %d\n\tdir_name: %s\n\tmap: ",
	 entry->inoden, entry->dir_name);
	__print_dptr(&entry->map);
	printk("\n}");
}

void __print_inode(inode * inptr) {
	int i;
	printk("inode:{\n");
	printk("\tnum: %d\n", inptr->num);
	printk("\ttype: %s\n", __ftype_to_str(inptr->type));
	printk("\tfsize: %d\n", inptr->fsize);
	printk("\tidata:{\n");
	for (i=0; i<MAX_IDATA && !__is_null(inptr->idata[i]); i++) {
		printk("\t\t");
		__print_dptr(&(inptr->idata[i]));
		printk("\n");
	}
	printk("\t}\n}");
}

char * __lntype_to_str(lntype type) {
	switch (type) {
	case FS_FDATA:
		return "FS_FDATA";
	case FS_DDATA:
		return "FS_DDATA";
	case FS_INODE:
		return "FS_INODE";
	case FS_IMAP:
		return "FS_IMAP";
	}
	return "error";
}

char * __ftype_to_str(ftype type) {
	switch (type) {
	case FS_FILE:
		return "FS_FILE";
	case FS_DIR:
		return "FS_DIR";
	}
	return "error";
}

void __print_ddata(ddata * ddptr) {
	int i;
	printk("ddata:{\n");
	for (i=0; i<MAX_DIR_FILES && strlen(ddptr->map[i].name) != 0; i++) {
		printk("\t{ inoden:%d, name: %s}\n",
		 ddptr->map[i].inoden, ddptr->map[i].name);
	}
	printk("\n}");
}

void __print_fdata(fdata * fdptr) {
	printk("fdata:{\n\t%s\n}", fdptr->data);
}

void __print_lnode(lnode * lnptr) {
	printk("lnode:{\n");
	printk("\ttype: %s\n", __lntype_to_str(lnptr->type));
	printk("\tnext: ");
	__print_dptr(&lnptr->next);
	printk("\n\tdata:{\n\t\t");
	switch(lnptr->type) {
	case FS_IMAP:
		__print_imap((imap *)lnptr->data);
		break;
	case FS_INODE:
		__print_inode((inode *)lnptr->data);
		break;
	case FS_DDATA:
		__print_ddata((ddata *)lnptr->data);
		break;
	case FS_FDATA:
		__print_fdata((fdata *)lnptr->data);
		break;
	}
	printk("\n\t}\n}\n");
}