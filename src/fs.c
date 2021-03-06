#include "fs.h"

#define OK 0
#define ERROR -1

//fs_syncs
static void __sync_log_buf();
static void __sync_cr(dptr address);
static void __write(void * data, int bytes, dptr address);
//loads
static void * __load(dptr addr, int bytes);
static checkpoint * __load_checkpoint(dptr addr);
static lnode * __load_lnode(dptr addr);
#define __LOAD(type, addr) ((type *)(__load_lnode(addr)->data))
#define __load_imap(addr) __LOAD(imap, addr)
#define __load_inode(addr) __LOAD(inode, addr)
#define __load_ddata(addr) __LOAD(ddata, addr)
#define __load_fdata(addr) __LOAD(fdata, addr)
//iteration
static lnode * __next_lnode(lnode * curr);
//constructors
static dptr __new_dptr(unsigned short sector, int offset);
static checkpoint * __new_checkpoint(int lsize);
static lnode * __new_lnode(lntype type, void * data, dptr next);
static inode * __new_inode(int inoden, ftype type, didata idata);
static imap * __new_imap(int inoden, dinode inode);
static ddata * __new_ddata();
static fdata * __new_fdata(void * data, int bytes);
//utils
#define __SET(dest, src, size) memcpy(&(dest), &(src), size)
#define __set_dptr(dest, src) __SET(dest, src, sizeof(dptr))
static bool __is_null_dptr(dptr addr);
static void __append_to_buf(void * data, int bytes);
static dptr __stage(lntype type, void * data);
static dptr __dptr_add(dptr address, int bytes);
static void __put_null(dptr * addr);
static bool __is_null_dptr(dptr addr);
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
//lookups
static void * __get_last_data(char * dir, ftype * type);
static int __get_inoden(char * filename);
static inode * __get_inode(imap * pimap, int inoden);
static imap * __get_imap(int inoden);
static void * __get_data(inode * inode, ftype * rettype);
static cr_entry * __get_cr_entry(int inoden);

//main
static void __mkfile(int inoden, char * filename, ftype type, void * data, int bytes);

static void __add_cr_entry(const char * filename, int inoden, dimap map);
static int __get_max_inoden();
static int __get_parent_filename(char * filename, char * pfname);
static void __add_ddata_entry(ddata * ddatap, int inoden, char * filename);

static int __dptr_to_int(dptr * addr);
static dptr __int_to_dptr(int bytes);
static int __sizeof_lndata(lntype type);

static bool __is_alive(lnode * lnptr);
static bool __is_inode_alive(inode * inode);
static bool __cmp_inodes(inode * inode1, inode * inode2);
static bool __is_imap_alive(imap * imap);
static bool __cmp_imaps(imap * imap1, imap * imap2);
static bool __cmp_dptr(dptr dptr1, dptr dptr2);


int fs_sync() {
	fs_sync_cr();
	fs_sync_lbuf();
	return 0;
}

int fs_sync_cr() {
	printk("Syncing CR...\n");
	__sync_cr(__new_dptr(0, 0));
	printk("...Done\n");
	return 0;
}

int fs_sync_lbuf() {
	printk("Syncing log...\n");
	__sync_log_buf();
	printk("...Done\n");
	return 0;
}

int fs_print_cr() {
	__print_checkpoint(__cp);
	return 0;
}

int fs_print_log(int len) {
	dptr addr = __cp->lstart;
	lnode * curr;
	do {
		curr = __load_lnode(addr);
		__print_dptr(&addr);
		printk("\n");
		__print_lnode(curr);
		printk("alive?: %s\n", __is_alive(curr)? "true":"false");
		printk("\n");
		__set_dptr(addr, curr->next);
		len--;
	} while(len>0);
	return 0;
}

int fs_print_imap(char * filename) {
	__print_imap(__get_imap(__get_inoden(filename)));
	return 0;
}

int fs_print_inoden(char * filename) {
	printk("inoden:%d\n", __get_inoden(filename));
	return 0;
}

//For SHELL use (cd command). Given a direction, searchs for it in the cr
bool fs_fexists(char * dir){
	int i;
	for(i=0;i<MAX_IMAP;i++){
		if(strcmp((__cp->map)[i].filename, dir)){
			return true;
		}	
	}
	return false;
}

int fs_cat(char * dir){
	ftype type;
	/*int i;*/
	void * data=__get_last_data(dir, &type);
	if(type==FS_DIR){
		return ERROR;
	}else{
		fdata * _fdata=(fdata *) data;
		printk(_fdata->data);
		printk("\n");
		return OK;
	}
}

int fs_list(char * dir){
	ftype type;
	int i;
	void * data=__get_last_data(dir, &type);
	if(type == FS_FILE){
		return ERROR;
	} else {
		ddata * _ddata=(ddata *) data;
		for(i=0; i<=MAX_DIR_FILES; i++){
			if (((_ddata->map)[i]).inoden > 0) {
				printk("%s\n", ((_ddata->map)[i]).name);
			}
		}
		return OK;
	}
}

int fs_data() {
	int size = __cp->lsize+sizeof(checkpoint);
	printk("fs size (total)=%d bytes (approx. %d sectors)\n", size, size/SECTOR_SIZE);
	printk("CR size=%d bytes (approx. %d sectors)\n", sizeof(checkpoint), sizeof(checkpoint)/SECTOR_SIZE);
	printk("log size=%d bytes (approx. %d sectors)\n", __cp->lsize, __cp->lsize/SECTOR_SIZE);
	printk("log buffer size(curr)=%d bytes (approx. %d sectors)\n", __log_buf_size, __log_buf_size/SECTOR_SIZE);
	printk("log buffer size(total)=%d (approx. %d sectors)\n", FS_BUFFER_SIZE, FS_BUFFER_SIZE/SECTOR_SIZE);
	printk("sizeof(log node)=%d (approx. %d sectors)\n", sizeof(lnode), sizeof(lnode)/SECTOR_SIZE);
	return 0;
}

int fs_creat(int size) {
	printk("Wiping old FS...\n");
	int i=0;
	char empty[SECTOR_SIZE];
	for (i=0; i<(1<<16)/SECTOR_SIZE; i+=SECTOR_SIZE) {
		__write(empty, SECTOR_SIZE, __int_to_dptr(i*SECTOR_SIZE));
	}
	printk("...Done\n");

	printk("Creating CR...\n");
	__cp = __new_checkpoint(((size-sizeof(checkpoint))/sizeof(lnode))*sizeof(lnode));
	__print_checkpoint(__cp);
	printk("\n");
	printk("...Done\n");

	printk("Creating /...\n");
	__mkfile(0, "/", FS_DIR, NULL, 0);
	printk("\n...Done\n");

	fs_sync();
	return 0;
}

int fs_init() {
	printk("Starting FS...\n");
	__log_buf_size = 0;
	__cp = __load_checkpoint(__new_dptr(0, 0));
	printk("\n");
	printk("...Done\n");
	return 0;
}

int fs_run_gc(int len) {
	printk("Starting garbage collection...\n");
	bool done;
	dptr addr = __cp->lstart;
	lnode * curr;

	imap * imp;
	inode * inp;
	fdata * fdp;
	ddata * ddp;
	dptr prev;
	int inoden;
	int moved = 0;

	do {
		curr = __load_lnode(addr);
		__print_dptr(&addr);
		printk("\n");
		__set_dptr(addr, curr->next);
		printk("alive?: %s\n", __is_alive(curr)? "true":"false");
		if (__is_alive(curr)) {
			done = false;
			while (!done) {
				switch(curr->type) {
				case FS_DDATA:
					ddp = (ddata *)curr->data;
					prev = __stage(FS_DDATA, ddp);
					break;
				case FS_FDATA:
					fdp = (fdata *)curr->data;
					prev = __stage(FS_FDATA, fdp);
					break;
				case FS_INODE:
					inp = (inode *)curr->data;
					__set_dptr(inp->idata, prev);
					inoden = inp->num;
					prev = __stage(FS_INODE, inp);
					break;
				case FS_IMAP:
					imp = (imap *)curr->data;
					__set_dptr(imp->map[0].inode, prev);
					prev = __stage(FS_IMAP, imp);
					__set_dptr(__get_cr_entry(inoden)->map, prev);
					done = true;
					break;
				}
				moved++;
				curr = __load_lnode(curr->next);
			}
		}
		len--;
	} while(len>0);


	fs_sync();
	printk("Relocated %d living blocks\n", moved);
	printk("...Done\n");
	return 0;
}

int fs_mkfile(char * filename, ftype type, void * data, int bytes) {
	char * pfname = malloc(sizeof(char)*MAX_PATH);
	int pinoden;

	printk("filename: %s\n", filename);
	if (__get_inoden(filename) != -1) { //file exists
		printk("file exists: %s\n", filename);
		return -1;
	}

	__get_parent_filename(filename, pfname);
	/*printk("<filename:%s\n", filename);
	printk("parent:%s>\n", pfname);*/

	if ((pinoden=__get_inoden(pfname)) == -1) { //nonexistent parent
		printk("nonexistent parent: %s\n", pfname);
		return -1;
	}

	int inoden = __get_max_inoden()+1;
	imap * pimap = __get_imap(pinoden);
	inode * pinode = __get_inode(pimap, pinoden);

	if (pinode->type != FS_DIR) {
		printk("parent is not a directory!\n");
		return -1;
	}
	
	__mkfile(inoden, filename, type, data, bytes);

	ddata * pddata = __load_ddata(pinode->idata);
	__add_ddata_entry(pddata, inoden, filename);
	dptr prev = __stage(FS_DDATA, pddata);

	__set_dptr(pinode->idata, prev);
	prev = __stage(FS_INODE, pinode);

	__set_dptr(pimap->map[0].inode, prev);
	prev = __stage(FS_IMAP, pimap);

	__set_dptr(__get_cr_entry(pinoden)->map, prev);
	return 0;
}

void __mkfile(int inoden, char * filename, ftype type, void * data, int bytes) {
	fdata * fdptr;
	ddata * ddptr;
	inode * inptr;
	imap * imptr;
	dptr prev;

	switch(type) {
	case FS_DIR:
		ddptr = __new_ddata();
		prev = __stage(FS_DDATA, ddptr);
		printk("ddata at: ");
		break;
	case FS_FILE:
		fdptr = __new_fdata(data, bytes);
		prev = __stage(FS_FDATA, fdptr);
		printk("fdata at: ");
	}
	__print_dptr(&prev);
	printk("\n");

	inptr = __new_inode(inoden, type, prev);
	prev = __stage(FS_INODE, inptr);

	printk("inode at: ");
	__print_dptr(&prev);
	printk("\n");

	imptr = __new_imap(inoden, prev);
	prev = __stage(FS_IMAP, imptr);

	printk("imap at: ");
	__print_dptr(&prev);
	printk("\n");

	__add_cr_entry(filename, inoden, prev);
}

int __get_max_inoden() {
	int i, inoden = 0;
	for (i=0; i<MAX_IMAP; i++) {
		inoden = max(__cp->map[i].inoden, inoden);
	}
	return inoden;
}

void __add_ddata_entry(ddata * ddatap, int inoden, char * filename) {
	int i;
	for (i=0; i<MAX_DIR_FILES; i++) {
		if (ddatap->map[i].inoden <= 0) { // isn't set
			ddatap->map[i].inoden = inoden;
			strcpy(ddatap->map[i].name, filename);
			return;
		}
	}
}

int __get_parent_filename(char * filename, char * pfname) {
	int i, last = -1;
	for (i=0; filename[i]!='\0'; i++) {
		if (filename[i] == '/') {
			last = i;
		}
	}
	printk("i=%d/%d, last=%d\n", i, strlen(filename), last);

	if (last == -1) {
		return -1;
	} else if (last == 0) {
		pfname[0]=filename[0];
		pfname[1]='\0';
		
		printk("test=%s\n", pfname);
		return 1;
	} else {
		strncpy(pfname, filename, last);
		pfname[i] = '\0';
		printk("test=%s\n", pfname);
		return last;
	}
}

cr_entry * __get_cr_entry(int inoden) {
	int i;
	for (i=0; i<MAX_IMAP && ((__cp->map)[i]).inoden!=-1; i++) {
		if ((__cp->map[i]).inoden == inoden) {
			return &(__cp->map[i]);
		}
	}
	return NULL;
}

void __add_cr_entry(const char * dirname, int inoden, dimap dimap) {
	int i;
	cr_entry * crep;
	for (i=0; i<MAX_IMAP; i++) {
		crep = &__cp->map[i];
		if (__is_null_dptr(crep->map)) {
			crep->inoden = inoden;
			strcpy(crep->filename, dirname);
			__set_dptr(crep->map, dimap);
			return;
		}
	}
	//must check for the i >= MAX_IMAP elsewhere!
}

void * __get_last_data(char * dir, ftype * type){
	int inoden=__get_inoden(dir);
	imap * imap=__get_imap(inoden);
	inode * inode=__get_inode(imap,inoden);
	return __get_data(inode,type);
}


int fs_remove(char * dir) {
	int i;
	cr_entry * _map=__cp->map;
	cr_entry * current;
	bool found = false;
	for(i=0; i<=MAX_IMAP; i++){
		current = &(_map[i]);
		if (strstr(current->filename, dir) != NULL){
			printk("removing... %s\n", current->filename);
			__put_null(&(current->map));
			found = true;
		}
	}
	return found? 0:ERROR;
}

imap * __get_imap_inoden(int inoden){
	int i;
	for(i=0;i<MAX_IMAP;i++){
				if((__cp->map)[i].inoden==inoden){
			return __load_imap((__cp->map)[i].map);
		}	
	}
	return NULL;
}

int __get_inoden(char * filename){
	int i;
	cr_entry * curr;
	for(i=0; i<=MAX_IMAP; i++){
		curr = &(__cp->map[i]);
		if (curr->inoden != -1 && streq(curr->filename, filename)) {
			/*__print_cr_entry(curr);*/
			return curr->inoden;
		}
	}
	return -1;
}

imap * __get_imap(int inoden) {
	int i;
	cr_entry * curr;
	for(i=0; i<=MAX_IMAP; i++){
		curr = &(__cp->map[i]);
		if (inoden == curr->inoden) {
			return __load_imap(curr->map);
		}
	}
	return NULL;
}

inode * __get_inode(imap * pimap, int inoden) {
	int i;
	imap_entry * curr;
	for (i=0; i<MAX_INODES; i++) {
		curr = &(pimap->map[i]);
		if (curr->inoden == inoden) {
			return __load_inode(curr->inode);
		}
	}
	return NULL;
}

//Uso: hay que volver a castear data con el type de retorno al recibir.
void * __get_data(inode * inode, ftype * rettype){
	void * data;
	if(inode->type==FS_FILE){
		data=__load_fdata(inode->idata);
	}else{
		data=__load_ddata(inode->idata);
	}
	*rettype=inode->type;
	return data;
}

bool __is_alive(lnode * lnptr){
	imap * _imap;
	inode * _inode;
	// ddata * _ddata;
	// fdata * _fdata;
	lnode * nextlnode;
	switch(lnptr->type){
		case FS_IMAP:
			_imap=(imap*)lnptr->data;
			return __is_imap_alive(_imap);
			break;
		case FS_INODE:
			_inode=(inode*)lnptr->data;
			return __is_inode_alive(_inode);
			break;
		case FS_DDATA:
			//Al tener un solo segmento de datos, revisamos el inodo que está si o si al lado
			// _ddata=(ddata*)lnptr->data;
			nextlnode=__next_lnode(lnptr);
			_inode=(inode*)nextlnode->data;
			return __is_inode_alive(_inode);
			break;
		case FS_FDATA:
			// _fdata=(fdata*)lnptr->data;
			nextlnode=__next_lnode(lnptr);
			_inode=(inode*)nextlnode->data;
			return __is_inode_alive(_inode);
			break;
		default:
			return false;
			break;
	}
}

bool __is_inode_alive(inode * _inode){
	imap * imap=__get_imap (_inode->num);
	if(imap==NULL){
		return false;
	}else{
		inode * inode2=__get_inode(imap,_inode->num);
		if(inode2==NULL){
			return false;
		}else{
			return __cmp_inodes(inode2, _inode);
		}
	}
}

bool __cmp_inodes(inode * inode1, inode * inode2){
	return inode1->num==inode2->num && inode1->fsize==inode2->fsize && inode1->type==inode2->type && __cmp_dptr(inode1->idata, inode2->idata);
}


bool __is_imap_alive(imap * _imap){
	int i;
	cr_entry curr;
	imap * auximap;
	for(i=0;i<=MAX_IMAP;i++){
		curr=(__cp->map)[i];
		auximap=__load_imap(curr.map);
		if(__cmp_imaps(auximap,_imap)){
			return true;
		}
	}
	return false;
}

bool __cmp_imaps(imap * imap1, imap * imap2){
	int i;
	imap_entry curr1;
	imap_entry curr2;
	for(i=0;i<=MAX_IMAP;i++){
		curr1=(imap1->map)[i];
		curr2=(imap2->map)[i];
		if(curr1.inoden!=curr2.inoden
			|| !__cmp_dptr(curr1.inode, curr2.inode)){
			return false;
		}
	}
	return true;
}

bool __cmp_dptr(dptr dptr1, dptr dptr2){
	return dptr1.offset==dptr2.offset && dptr1.sector==dptr2.sector;
}

lnode * __next_lnode(lnode * curr) {
	return __load_lnode(curr->next);
}

dptr __stage(lntype type, void * data) {
	dptr prev_end = __cp->lend;

	//TODO: deberia fijarse si no se lleno el fs antes de hacer esto! (lease, que no este live el nuevo puntero)

	int bytes = (__dptr_to_int(&(__cp->lend))-__dptr_to_int(&(__cp->lstart))
		+sizeof(lnode))%__cp->lsize;

	dptr new_end = __dptr_add(__cp->lstart, bytes);
	__append_to_buf(__new_lnode(type, data, new_end), sizeof(lnode));
	__set_dptr(__cp->lend, new_end);

	return prev_end;
}

void __append_to_buf(void * data, int bytes) {
	if (__log_buf_size+bytes >= __cp->lsize) {
		printk("Syncing...\n");
		__sync_log_buf();
		printk("...Done\n");
	}
	memcpy(__log_buf+__log_buf_size, data, bytes);
	__log_buf_size += bytes;
}

void __sync_log_buf() {
	printk("Syncing...\n");
	int fst_offset = __dptr_to_int(&(__cp->lfst));
	int last_offset = __dptr_to_int(&(__cp->lend));
	int total = __dptr_to_int(&(__cp->lstart))+__cp->lsize;

	if (fst_offset == last_offset) {
		return; //you shouldn't be syncing in the first place... Show off
	}
	if (fst_offset < last_offset) { // there was no overflow
		printk("\nfrom: ");
		__print_dptr(&(__cp->lfst));
		printk("\nto: ");
		dptr aux = __dptr_add(__cp->lfst, __log_buf_size);
		__print_dptr(&aux);
		printk("\n");
		__write(__log_buf, __log_buf_size, __cp->lfst);
	} else {
		__write(__log_buf, total-last_offset, __cp->lfst);
		__write(__log_buf+(total-last_offset), last_offset, __cp->lstart);	
	}
	__log_buf_size = 0;
	__set_dptr(__cp->lfst, __cp->lend);
	printk("...Synced\n");
}

void __sync_cr(dptr address) {
	__write(__cp, sizeof(checkpoint), address);
}

void __write(void * data, int bytes, dptr address) {
	ata_write(FS_DRIVE, data, bytes, address.sector, address.offset);
}


bool __is_null_dptr(dptr addr) {
	return addr.sector == 0 && addr.offset == 0;
}

void __put_null(dptr * addr) {
	addr->sector = 0;
	addr->offset = 0;
}


dptr __dptr_add(dptr address, int bytes) {
	int _bytes = __dptr_to_int(&address) + bytes;
	return __new_dptr(_bytes/SECTOR_SIZE, _bytes%SECTOR_SIZE);
}

void * __load(dptr addr, int bytes) {
	void * data = malloc(bytes);
	ata_read(FS_DRIVE, data, bytes, addr.sector, addr.offset);
	return data;
}

lnode * __load_lnode(dptr addr) {
	lnode * data = malloc(sizeof(lnode));
	ata_read(FS_DRIVE, (void *)data, sizeof(lnode), addr.sector, addr.offset);
	/*__print_lnode(data);*/
	return data;
}

checkpoint * __load_checkpoint(dptr addr) {
	checkpoint * cp = (checkpoint *)__load(addr, sizeof(checkpoint));
	__set_dptr(cp->lfst, cp->lend);
	return cp;
}


checkpoint * __new_checkpoint(int lsize) {
	int i;
	printk("sizeof(lnode):%d\n", sizeof(lnode));
	printk("lsize: %d (%d lnodes)\n", lsize, lsize/sizeof(lnode));
	dptr start = __int_to_dptr(sizeof(checkpoint));
	checkpoint * cp = malloc(sizeof(checkpoint));
	__set_dptr(cp->lstart, start);
	__set_dptr(cp->lend, cp->lstart);
	__set_dptr(cp->lfst, cp->lend);
	cp->lsize = lsize;
	for (i=0; i<MAX_DIR_FILES; i++) {
		cp->map[i].inoden=-1;
	}
	return cp;
}

fdata * __new_fdata(void * data, int bytes) {
	fdata * fdptr = malloc(sizeof(fdata));
	if (bytes == 0) {
		fdptr->data[0] = '\0';
	} else {
		memcpy(fdptr->data, data, bytes);
	}
	return fdptr;
}

ddata * __new_ddata() {
	ddata * ddptr = malloc(sizeof(ddata));
	return ddptr;
}

inode * __new_inode(int inoden, ftype type, didata idata) {
	inode * inptr = malloc(sizeof(inode));
	inptr->num = inoden;
	inptr->type = type;
	__set_dptr(inptr->idata, idata);
	return inptr;
}

imap * __new_imap(int inoden, dinode inode) {
	int i;
	imap * imptr = malloc(sizeof(imap));
	imptr->map[0].inoden = inoden;
	for (i=1; i<MAX_INODES; i++) {
		imptr->map[i].inoden = -1;
	}
	__set_dptr(imptr->map[0].inode, inode);
	return imptr;
}


dptr __new_dptr(unsigned short sector, int offset) {
	dptr addr;
	addr.sector = sector;
	addr.offset = offset;
	return addr;
}

lnode * __new_lnode(lntype type, void * data, dptr next) {
	lnode * lnptr = malloc(sizeof(lnode));
	lnptr->type = type;
	__set_dptr(lnptr->next, next);
	memcpy(lnptr->data, data, __sizeof_lndata(type));
	return lnptr;
}


int __sizeof_lndata(lntype type) {
	switch(type) {
	case FS_IMAP:
		return sizeof(imap);
	case FS_INODE:
		return sizeof(inode);
	case FS_DDATA:
		return sizeof(ddata);
	case FS_FDATA:
		return sizeof(fdata);
	}
	return -1;//CHECK!
}


dptr __int_to_dptr(int bytes) {
	return __new_dptr(bytes/SECTOR_SIZE, bytes%SECTOR_SIZE);
}

int __dptr_to_int(dptr * addr) {
	return (addr->sector)*SECTOR_SIZE + addr->offset;
}

void __print_checkpoint(checkpoint * cp) {
	int i;
	printk("checkpoint:{\n\tlstart: ");
	__print_dptr(&(cp->lstart));
	printk("\n\tlend: ");
	__print_dptr(&(cp->lend));
	printk("\n\tmap:{\n");
	for (i=0; i<MAX_IMAP && ((cp->map)[i]).inoden!=-1; i++) {
		__print_cr_entry(&(cp->map[i]));
		printk(",\n");
	}
	printk("\n}\n}");
}

void __print_dptr(dptr * addr) {
	printk("dptr:{ sector:%d, offset:%d } (#%d d)", addr->sector, addr->offset, addr->sector*SECTOR_SIZE+addr->offset);
}

void __print_imap(imap * imptr) {
	int i;
	printk("imap:{\n");
	for (i=0; i<MAX_IMAP && !__is_null_dptr(imptr->map[i].inode); i++) {
		if ((imptr->map[i]).inoden >= 0) {
			printk("\t{ inoden:%d, inode: ", imptr->map[i].inoden);
			__print_dptr(&((imptr->map[i]).inode));
			printk("}\n");
		}
	}
	printk("\n}");
}

void __print_cr_entry(cr_entry * entry) {
	printk("cr_entry:{\n\tinoden: %d\n\tfilename: %s\n\tmap: ",
	 entry->inoden, entry->filename);
	__print_dptr(&entry->map);
	printk("\n}");
}

void __print_inode(inode * inptr) {
	printk("inode:{\n");
	printk("\tnum: %d\n", inptr->num);
	printk("\ttype: %s\n", __ftype_to_str(inptr->type));
	printk("\tfsize: %d\n", inptr->fsize);
	printk("\tidata:{\n");
	if (!__is_null_dptr(inptr->idata)) {
		printk("\t\t");
		__print_dptr(&(inptr->idata));
		printk("\n");
	}
	printk("\t}\n}");
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
	__print_dptr(&(lnptr->next));
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
	return "ERROR";
}

char * __ftype_to_str(ftype type) {
	switch (type) {
	case FS_FILE:
		return "FS_FILE";
	case FS_DIR:
		return "FS_DIR";
	}
	return "ERROR";
}