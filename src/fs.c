#include "fs.h"

//TODO TERE: Testing LFS structure, FREES.

//syncs
static void __sync_log_buf();
static void __sync_cr(disk_addr address);
static void __write(void * data, int bytes, disk_addr address);
//loads
#define __LOAD(type, addr) ((type *)__load(addr, sizeof(type)))
static void * __load(disk_addr addr, int bytes);
static lnode * __load_lnode(disk_addr addr);
#define __load_checkpoint(addr) __LOAD(checkpoint, addr)
#define __load_imap(addr) __LOAD(imap, addr)
#define __load_inode(addr) __LOAD(inode, addr)
#define __load_ddata(addr) __LOAD(ddata, addr)
#define __load_fdata(addr) __LOAD(fdata, addr)
//iteration
static lnode * __next_lnode_buf(int * i);
static lnode * __next_lnode(lnode * curr);
//constructors
static disk_addr __disk_addr_new(unsigned short sector, int offset);
static checkpoint * __checkpoint_new(disk_addr lstart, disk_addr lend);
//utils
static bool __is_null(disk_addr addr);
//debugging
static void __checkpoint_print(checkpoint * cp);
static void __disk_addr_print(disk_addr * addr);

//vars
static int __drive;

static checkpoint * __cp;

static char __log_buf[BUFFER_SIZE];
static int __log_buf_size = 0;
static (lnode *) __log_buf_list[BUFFER_SIZE/sizeof(lnode)];

/*static char __path_buffer[MAX_PATH];*/

/*TODO: static char * pwd;*/

int testfs() {
	printk("Hola mundo!\n");
	create(ATA0, 1<<15);

	printk("Syncing CR...\n");
	__sync_cr(__disk_addr_new(0, 0)); //TODO:remove!
	printk("...Done\n");

	init(); //TODO:remove!
	return 0;
}

void create(int drive, int size){
	__drive = drive;

	/* TODO:
	mkdir de /
	mkdir("/");
	*/

	printk("Creating CR...\n");
	__cp = __checkpoint_new(__disk_addr_new(MAX_IMAP/SECTOR_SIZE, MAX_IMAP%SECTOR_SIZE),
		__disk_addr_new(size/SECTOR_SIZE, size%SECTOR_SIZE));
	__checkpoint_print(__cp);
	printk("\n");
	printk("...Done\n");
}

void init() {
	printk("Starting FS...\n");
	__cp = __load_checkpoint(__disk_addr_new(0, 0));
	__checkpoint_print(__cp);
	printk("\n");
	printk("...Done\n");
}

/*int __mkdir(char * filename) {
	
}*/

/*int mkdir(char * filename){
	// TODO: aca hago un lookup, primero en el buffer


}*/

//TODO: como escribir en el __log_buf (memcpy al final o castear el final al tipo que tengas, trabajar ahi y listo)
//TODO: sync: baja a disco

int touch() { //JP
	// mismo que mkdir pero para archivos
}

int append() {
	//Agarro el inodo
	dinode mydinode=__load_inode(__get_last_inode());
	if(strlen((mydinode->idata)[i])) //?? Revisar punteros
}

// borra archivo o directorio (con todo lo que tenga adentro)
int remove(char * dir) {
	// Cambia de acuerdo a si es un archivo o un directorio cambia el comportamiento
	int n,flag=0,count=0;
	imap_entry myimapentry;
	cr_entry * map=(crp->map);
	inode * pinode;
	// Borra el puntero al inodo del imapa.. 
	for(i=0;i<=MAX_IMAP;i++){ //RECORRO EL CR
		cr_entry actualentry=map[i];
		if(strcmp(actualentry.dir_name, dir)){
			n=actualentry.inoden;
			imap * mypimap = __load_imap(actualentry.map)
			for(j=0; j<=MAX_INODES j++){ //RECORRO EL IMAP
				myimapentry = (myimap->map)[j];
				dinode mydinode=myimapentry.inode;
				if(mydinode.sector != 0 && mydinode.offset != 0){
					count++; //Hay algo en el sector
				}
				if(myimapentry.inoden=n){
					count--;
					pinode=__load_inode(mydinode); //CARGO EL INODO
					for(k=0;k<MAX_IDATA;k++){ //BORRO TODA LA DATA DEL INODO
						__put__null((pinode->idata)[k]);
					}
					__put__null(&mydinode);
				}
			}
			if(count==0){
				__put__null(&actualentry.map);
			}
		}		
	}

	// Si el imapa queda vacío borro también el puntero del CR al imapa
	return 1;
}

bool __is_inode_dir(inode * myinode) {
	if(myinode->type)==FS_DIR){
		return true; 
	} return false;
}

bool __is_inode_file(inode * myinode) {
	if(myinode->type)==FS_FILE){
		return true; 
	} return false;
}

//Returns -1 if it doesn't exist
//Gets the inode number searching in the directory data for a char * file
int __get_inode_from_directory(dinode myinode, char * name){
	if(!__is_inode_dir(myinode)){
		return -1; 
	}else{
		dir_data_pair[MAX_INODES] dmap =((myinode->idata).ddata).mdata;
		for(i=0; i<MAX_INODES && dmap[i]!=NULL;i++){
			if(strcmp(dmap[i],name)){
				return dmap[i].inoden;
			}
		}
		return -1;
	}
}

int __get_last(char * filename, dimap lastdimap, dinode lastdinode, void * mydidata) {
	dimap mydimap;
	dinode mydinode;
	ftype mytype;
	int read,fnsize,myinoden;
	char * dir;

	while(filename[0]!='\0'){
		fnsize=strlen(filename);
		read=__get_cr_imap_n_inoden(filename, mydimap,myinoden);
		
		//Cutting the filename - strcut with sprintf
		char * newfilename=malloc((fnsize-i)+1);
		sprintf(newfilename, "%.*s", fnsize-i, filename[i]);
		filename=newfilename;
		free(newfilename);

		read=__get_fst_dir(filename, dir);
		__get_inode_from_imap(mydimap,mydinode,myinoden);
		__get_data_from_inode(mydinode,mytype,mydidata); //devuelvo idata y type
	
	}
	return -1;
}

int __get_data_from_inode(dinode mydinode, ftype mytype, void * mydidata){
	actualinode = __load_inode(mydinode);
	mytype = actualinode.type;
	mydidata = actualinode.idata;
	return 0;
}

//Having the inode number and the piece of the imap, searchs for the inode
//If there is an error, it returns -1.
int __get_inode_from_imap(dimap mydimap, dinode mydinode, int myinoden){
	for(i=0;i<MAX_INODES;i++){
		imap_entry actual = (__load_imap(mydimap)->map)[i];
		if(actual==NULL){
			return -1;	
		}else{
			if(actual.inoden==myinoden){
				//No sube el inode a disco porque todavía no lo uso
				mydinode=actual.inode; 
				return actual.inoden;
			}
		}
	}
	return -1;
}

//TODO: . y .. !!
// Busca el primer imap desde el CR (sea file o sea directory. Arreglate vos)
// La idea sería obtener el imap del primer directorio para ir mapeando desde ahí
int __get_cr_imap_n_inoden(char * filename, dimap mydimap, int myinoden) {
	char * dir;
	int read;

	read = __get_fst_dir(filename, &dir);
	if(strcmp(filename, "/")){
		//peola
	}else if(strcmp(filename,".")|| strcmp(filename, "..")){
		//We have to search the CR with pwd or the one before the pwd
	}else{
		//caso Tere/Downloads
		//agregar el pwd
	}
	cr_entry[MAX_IMAP] localmap =(cp->map);
	for (i=0;i<=MAX_IMAP && !__is_null(localmap[i].map);i++){
		cr_entry actualentry=localmap[i];
		if(strcmp(actualentry.dir_name,dir)){
			mydimap = actualentry.map;
			myinoden=actualentry.inoden;
		}
	}
	return read;
}

// Gets the first director copying everything before /
// Returns number of chars read
int __get_fst_dir(char * filename, char * dir) {
	int i=1;
	while (filename[i-1] != '\0' && filename[i-1] != '/') {
		i++;
	}

	dir = malloc(sizeof(char) * (i+1));
	sprintf(dir, "%.*s", i, filename);

	return i;
}

lnode * __next_lnode_buf(int * i) {
	if (*i >= __log_buf_size) {
		return NULL;
	}
	*i=(*i)++;
	return __log_buf_list_i[*i];
}

lnode * __next_lnode(lnode * curr) {
	return __load_lnode(curr->next);
}

void __sync_log_buf() {
	int i, bytes;
	for (i=0; __log_buf_size-(i*SECTOR_SIZE)>0; i++) {
		bytes = min(__log_buf_size, SECTOR_SIZE);
		__write(__log_buf+i*SECTOR_SIZE, bytes, __cp->lend);
		__cp->lend.sector += bytes / SECTOR_SIZE;
		__cp->lend.offset = (__cp->lend.offset + bytes) % SECTOR_SIZE;
	}
	for (i=0; i<__log_buf_size; i++) {
		__log_buf_list[i] = NULL;
	}
	__log_buf_size = 0;
}

void __sync_cr(disk_addr address) {
	__write(__cp, sizeof(checkpoint), address);
}

void __write(void * data, int bytes, disk_addr address) {
	ata_write(__drive, data, bytes, address.sector, address.offset);
}

disk_addr __disk_addr_new(unsigned short sector, int offset) {
	disk_addr addr;
	addr.sector = sector;
	addr.offset = offset;
	return addr;
}

bool __is_null(disk_addr addr) {
	return addr.sector == 0 && addr.offset == 0;
}

void __put__null(disk_addr * addr){
	disk_addr->sector=0;
	disk_addr->offset=0;
}

void * __load(disk_addr addr, int bytes) {
	void * data = malloc(bytes);
	ata_read(__drive, data, bytes, addr.sector, addr.offset);
	return data;
}

lnode * __load_lnode(disk_addr addr) {
	void * data = load(addr, MAX_LNODE_SIZE);
	int actual_size;
	switch(((int *)data)[0]) {
	case FS_IMAP:
		actual_size = sizeof(imap);
		break;
	case FS_INODE:
		actual_size = sizeof(inode);
		break;
	case FS_DDATA:
		actual_size = sizeof(ddata);
		break;
	case FS_FDATA:
		actual_size = sizeof(fdata);
		break;
	//TODO: check for more cases!
	}
	free(data + actual_size, MAX_LNODE_SIZE - actual_size);
	//or else you'll read garbage
	return (lnode) data;
}

checkpoint * __checkpoint_new(disk_addr lstart, disk_addr lend) {
	checkpoint * cp = malloc(sizeof(checkpoint));
	cp->lstart.sector = lstart.sector;
	cp->lstart.offset = lstart.offset;
	cp->lend.sector = lend.sector;
	cp->lend.offset = lend.offset;
	return cp;
}

void __checkpoint_print(checkpoint * cp) {
	printk("checkpoint:{\n\tlstart: ");
	__disk_addr_print(&(cp->lstart));
	printk("\n\tlend: ");
	__disk_addr_print(&(cp->lend));
	printk("\n}");
}

void __disk_addr_print(disk_addr * addr) {
	printk("disk_addr:{ sector:%d, offset:%d }", addr->sector, addr->offset);
}