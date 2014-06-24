#include "fs.h"

#define OK 0
#define ERROR -1

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
//iteration
static lnode * __next_lnode_buf(int * i);
static lnode * __next_lnode(lnode * curr);
//constructors
static dptr __new_dptr(unsigned short sector, int offset);
static checkpoint * __new_checkpoint(dptr lstart, dptr lend);
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
static int __sizeof_lntype(lntype type);
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
static int __get_fst_dirname(char * filename, char * dir);
static void * __get_last_data(char * dir, ftype * type);
static int __get_inoden(char * filename);
static inode * __get_inode(imap * pimap, int inoden);
static imap * __get_imap(int inoden);
static void * __get_data(inode * inode, ftype * rettype);
/*static int __get_cr_entry(char * filename, cr_entry * crep);*/

//main
static void __mkfile(int inoden, char * filename, ftype type, void * data, int bytes);
static int __list(char * dir);
static int __remove(char * dir);

static void __add_cr_entry(const char * filename, int inoden, dimap map);
static int __get_max_inoden();
static int __get_parent_filename(char * filename, char * pfname);
static void __add_ddata_entry(ddata * ddatap, int inoden, char * filename);

//vars
static int __drive;
static int __log_size;

static checkpoint * __cp;

static char __log_buf[BUFFER_SIZE];
static int __log_buf_size = 0;
static lnode * __log_buf_list[BUFFER_SIZE/sizeof(lnode)];

/*static char __path_buffer[MAX_PATH];*/

/*TODO: static char * pwd;*/

int __get_max_inoden() {
	int i, inoden = 0;
	for (i=0; i<MAX_IMAP; i++) {
		inoden = max(__cp->map[i].inoden, inoden);
	}
	return inoden;
}

int sync_cr() {
	printk("Syncing CR...\n");
	__sync_cr(__new_dptr(0, 0));
	printk("...Done\n");
	return 0;
}

int sync_lbuf() {
	printk("Syncing log buffer...\n");
	__sync_log_buf();
	printk("...Done\n");
	return 0;
}

int testfs() {
	create(ATA0, 1<<15);
	//init(); //TODO:remove!
	mkfile("/tere", FS_DIR, NULL, 0);
	mkfile("/tere/Downloads", FS_DIR, NULL, 0);
/*	char * text = "hola mundo!";
	mkfile("/tere/Downloads/pepe.txt", FS_FILE, text, strlen(text)+1);*/

	__print_checkpoint(__cp);


/*	sync_cr();
	sync_lbuf();*/
	
/*	printk("Loading CR...");
	__cp = __load_checkpoint(__new_dptr(0,0));
	__print_checkpoint(__cp);
	printk("...Done\n");
*/
/*	char * dir;
	printk("%d, %s\n", __get_fst_dirname("/d1/d2/d3/f1", dir), dir);
	printk("%d, %s\n", __get_fst_dirname("d1/d2/d3/f1", dir), dir);
*/

	/*printk("__get_cr_entry...\n");
	cr_entry crep;
	printk("ret:%d\n", __get_cr_entry("/", &crep));
	__print_cr_entry(&crep);*/

	/*__print_inode(__load_inode(__new_dptr(4, 400)));
	__print_imap(__load_imap(__new_dptr(4, 492)));*/
	
/*	dinode dinode;
	imap imap;
	__get_inode(&crep.map, 0, &dinode, &imap);
*/	//__print_inode(__load_inode(dinode));
	//__print_imap(&imap);
	
	/*char * ret;
	__get_fst_dirname("/Tere/Downloads", ret);
	printk(ret);
*/
	/*	sync_cr();
	sync_lbuf();*/

	printk("Chau!\n");
	return 0;
}

void create(int drive, int size) {
	dptr start;

	__drive = drive;
	__log_size = size - sizeof(checkpoint);

	printk("Creating CR...\n");
	printk("fs size (total)=%d bytes\nCR size=%d bytes\nlog size=%d bytes\n", size, sizeof(checkpoint), __log_size);
	/*FIXME: lstart esta en 0,0 y el CR se va a guardar ahi!! 
	(parece que la suma no anda!)*/
	start = __dptr_add(__new_dptr(0, 0), sizeof(checkpoint));
	__cp = __new_checkpoint(start, start);
	__print_checkpoint(__cp);
	printk("\n");
	printk("...Done\n");

	printk("Creating /...\n");
	__mkfile(0, "/", FS_DIR, NULL, 0);
	printk("\n...Done\n");
}

void init() {
	printk("Starting FS...\n");
	__log_buf_size = 0;
	__cp = __load_checkpoint(__new_dptr(0, 0));
	__print_checkpoint(__cp);


	printk("\n");
	printk("...Done\n");
}

int mkfile(char * filename, ftype type, void * data, int bytes) {
	char pfname[MAX_PATH];
	int pinoden;

	//TODO: mkdir deberia fijarse si el directorio es hijo de alguien y modificar a ese alguien!
	if (__get_inoden(filename) != -1) { //file exists
		printk("file exists: %s\n", filename);
		return -1;
	}

	/*__get_parent_filename(filename, pfname);
	printk("<filename:%s\n", filename);
	printk("parent:%s>\n", pfname);*/

	if ((pinoden=__get_inoden(pfname)) == -1) { //nonexistent parent
		printk("nonexistent parent: %s\n", pfname);
		return -1;
	}
	int inoden = __get_max_inoden()+1;

	imap * pimap = __get_imap(pinoden);
/*	__print_imap(pimap);*/
	inode * pinode = __get_inode(pimap, pinoden);
/*	__print_inode(pinode);*/
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
/*		printk("ddata at: ");*/
		break;
	case FS_FILE:
		fdptr = __new_fdata(data, bytes);
		prev = __stage(FS_FDATA, fdptr);
/*		printk("fdata at: ");*/
	}	
/*	__print_dptr(&prev);
	printk("\n");*/

	inptr = __new_inode(inoden, type, prev);
	prev = __stage(FS_INODE, inptr);

/*	printk("inode at: ");
	__print_dptr(&prev);
	printk("\n");*/

	imptr = __new_imap(inoden, prev);
	prev = __stage(FS_IMAP, imptr);

/*	printk("imap at: ");
	__print_dptr(&prev);
	printk("\n");
*/
	__add_cr_entry(filename, inoden, prev);
}

void __add_ddata_entry(ddata * ddatap, int inoden, char * filename) {
	int i;
	for (i=0; i<MAX_DIR_FILES; i++) {
		if (ddatap->map[i].inoden <= 0) { // isn't set
			ddatap->map[i].inoden = inoden;
			strcpy(ddatap->map[i].name, filename);
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
	if (last == -1) {
		return -1;
	} else if (last == 0) {
		strncpy(pfname, filename, 1);
		return 1;
	} else {
		strncpy(pfname, filename, last);
		return last;
	}
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

fdata * __new_fdata(void * data, int bytes) {
	fdata * fdptr = malloc(sizeof(fdata));
	memcpy(fdptr->data, data, bytes);
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
	/*TODO:
	switch(type) {
	case FS_DIR:
		inptr->fsize = sizeof(ddata);
	or maybe fsize should be a parameter?
	}*/
	__set_dptr(inptr->idata, idata);
	return inptr;
}

imap * __new_imap(int inoden, dinode inode) {
	imap * imptr = malloc(sizeof(imap));
	imptr->map[0].inoden = inoden;
	__set_dptr(imptr->map[0].inode, inode);
	return imptr;
}


//TODO: como escribir en el __log_buf (memcpy al final o castear el final al tipo que tengas, trabajar ahi y listo)
//TODO: sync: baja a disco

/*int touch() { //JP
	// mismo que mkdir pero para archivos
}*/

int fs_cat(char * dir){
	ftype type;
	int i;
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
	if(type==FS_FILE){
		return ERROR;
	}else{
		ddata * _ddata=(ddata *)data;
		for(i=0;i<=MAX_DIR_FILES;i++){
			printk(((_ddata->map)[i]).name);
			printk("\n");
		}
		return OK;
	}
}

void * __get_last_data(char * dir, ftype * type){
	int inoden=__get_inoden(dir);
	imap * imap=__get_imap(inoden);
	inode * inode=__get_inode(imap,inoden);
	return __get_data(inode,type);
}


// borra archivo o directorio (con todo lo que tenga adentro)
int __remove(char * dir) {
	int i;
	cr_entry * _map=__cp->map;
	for(i=0;i<=MAX_IMAP;i++){
		cr_entry current=_map[i];
		if(strcmp(current.filename,dir)){
			__put_null(&current.map);
			return OK;
		}		
	}
	return ERROR;
}

// bool __is_inode_dir(inode * myinode) {
// 	if (myinode->type)==FS_DIR) {
// 		return true; 
// 	} return false;
// }

// bool __is_inode_file(inode * myinode) {
// 	if (myinode->type)==FS_FILE) {
// 		return true; 
// 	} return false;
// }

//For SHELL use (cd command). Given a direction, searchs for it in the cr
bool file_existence(char * dir){
	int i;
	for(i=0;i<MAX_IMAP;i++){
		if(strcmp((__cp->map)[i].filename, dir)){
			return true;
		}	
	}
	return false;
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
			/*__print_cr_entry(curr);*/
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

//Pasamos el dato del directorio padre
int __get_inoden_child(ddata * data, char * name) {
	int i;
	ddata_entry * map=data->map;
	ddata_entry current;
	for(i=0; i<=MAX_DIR_FILES;i++){
		current=map[i];
		if(strcmp(current.name,name)){
			return current.inoden;
		}
	}
	return -1;
}

/*int __get_cr_entry(char * filename, cr_entry * crep) {
	int i;
	cr_entry * map = __cp->map;
	for (i=0; i<=MAX_IMAP && !__is_null_dptr(map[i].map);i++) {
		if (streq(map[i].filename, filename)) {
			*crep = map[i];
			return i;
		}
	}
	return -1;
}
*/
// Gets the first director copying everything before /
// Returns number of chars read
int __get_fst_dirname(char * filename, char * dir) {
	int i=1;
	while (filename[i-1] != '\0' && filename[i-1] != '/') {
		i++;
	}
	memcpy(dir, filename, i);
	dir[i]='\0';
	return i;
}

lnode * __next_lnode_buf(int * i) {
	if (*i >= __log_buf_size) {
		return NULL;
	}
	*i=(*i)+1;
	return __log_buf_list[*i];
}

lnode * __next_lnode(lnode * curr) {
	return __load_lnode(curr->next);
}

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

dptr __new_dptr(unsigned short sector, int offset) {
	dptr addr;
	addr.sector = sector;
	addr.offset = offset;
	return addr;
}

bool __is_null_dptr(dptr addr) {
	return addr.sector == 0 && addr.offset == 0;
}

void __put_null(dptr * addr) {
	addr->sector = 0;
	addr->offset = 0;
}

checkpoint * __new_checkpoint(dptr lstart, dptr lend) {
	checkpoint * cp = malloc(sizeof(checkpoint));
	cp->lstart.sector = lstart.sector;
	cp->lstart.offset = lstart.offset;
	cp->lend.sector = lend.sector;
	cp->lend.offset = lend.offset;
	int i;
	for (i=0; i<MAX_DIR_FILES; i++) {
		cp->map[i].inoden=-1;
	}
	return cp;
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

dptr __stage(lntype type, void * data) {
	dptr prev_end = __cp->lend;
	dptr new_end = __dptr_add(__cp->lend, __sizeof_lntype(type));
	__append_to_buf(__new_lnode(type, data, new_end), __sizeof_lntype(type));
	__set_dptr(__cp->lend, new_end);
	return prev_end;
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
		printk("\t{ inoden:%d, inode: ", imptr->map[i].inoden);
		__print_dptr(&imptr->map[i].inode);
		printk("}\n");
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