#include "fs.h"

//TODO TERE: Testing LFS structure, FREES.

//syncs
static void __sync_log_buf();
static void __sync_cr(dptr address);
static void __write(void * data, int bytes, dptr address);
//loads
static void * __load(dptr addr, int bytes);
#define __load_checkpoint(addr) ((checkpoint *)__load(addr, sizeof(checkpoint)))
static lnode * __load_lnode(dptr addr);
#define __LOAD(type, addr) ((type *)(__load_lnode(addr, sizeof(type))->data))
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
//utils
#define __SET(dest, src, size) memcpy(&(dest), &(src), size)
#define __set_dptr(dest, src) __SET(dest, src, sizeof(dptr))
static bool __is_null(dptr addr);
static void __log_buf_append(void * data, int bytes);
static void __lnode_append(lntype type, void * data);
static int __sizeof_lntype(lntype type);
static dptr __dptr_add(dptr address, int bytes);
//debugging
static void __print_checkpoint(checkpoint * cp);
static void __print_dptr(dptr * addr);
static void __print_imap(imap * imap);
static void __print_inode(inode * inode);
static void __print_ddata(ddata * ddata);
static void __print_fdata(fdata * fdata);
static char * __lntype_to_str(lntype type);
static char * __ftype_to_str(ftype type);
static void __lnode_print(lnode * lnode);
//main
static void __mkdir(char * basename);


//vars
static int __drive;
static int __log_size;
static int __inoden;

static checkpoint * __cp;

static char __log_buf[BUFFER_SIZE];
static int __log_buf_size = 0;
static lnode * __log_buf_list[BUFFER_SIZE/sizeof(lnode)];

/*static char __path_buffer[MAX_PATH];*/

/*TODO: static char * pwd;*/

int testfs() {
	__inoden = 0; // REMOVE!
	create(ATA0, 1<<15);
	//init(); //TODO:remove!
	printk("Chau!\n");
	return 0;
}

void create(int drive, int size){
	dptr start;

	__drive = drive;
	__log_size = size - sizeof(checkpoint);

	printk("Creating CR...\n");
	printk("fs size (total)=%d bytes\nCR size=%d bytes\nlog size=%d bytes\n", size, sizeof(checkpoint), __log_size);
	start = __dptr_add(__new_dptr(0, 0), sizeof(checkpoint));
	__cp = __new_checkpoint(start, start);
	__print_checkpoint(__cp);
	printk("\n");
	printk("...Done\n");

	printk("Creating /...\n");
	__mkdir("/");
	printk("...Done\n");

	printk("Syncing CR...\n");
	__sync_cr(__new_dptr(0, 0));
	printk("...Done\n");

	printk("Syncing log buffer...\n");
	__sync_log_buf();
	printk("...Done\n");

	printk("Loading CR...");
	__cp = __load_checkpoint(__new_dptr(0,0));
	__print_checkpoint(__cp);
	printk("...Done\n");
}

void init() {
	printk("Starting FS...\n");
	__log_buf_size = 0;
	__cp = __load_checkpoint(__new_dptr(0, 0));
	__print_checkpoint(__cp);
	printk("\n");
	printk("...Done\n");
}

// /!\ NOT the filename!
void __mkdir(char * basename) {
	ddata data;
	inode inode;
	imap imap;

	dptr prev = __cp->lend;
	__lnode_append(FS_DDATA, &data);

	inode.num = __inoden++;
	inode.type = FS_DIR;
	__set_dptr(inode.idata[0], prev);
	prev = __cp->lend;
	__lnode_append(FS_INODE, &inode);

	imap.map[0].inoden = inode.num;
	__set_dptr(imap.map[0].inode, prev);
	__lnode_append(FS_IMAP, &imap);

	__print_ddata(&data);
	printk("\n");
	__print_inode(&inode);
	printk("\n");
	__print_imap(&imap);
	printk("\n");
}

//TODO: como escribir en el __log_buf (memcpy al final o castear el final al tipo que tengas, trabajar ahi y listo)
//TODO: sync: baja a disco

/*int touch() { //JP
	// mismo que mkdir pero para archivos
}*/

// COSAS PARA HACER EN EL APPEND
// Appendeo un cacho de data a un archivo que ya existe (al final)!
// La idea sería: traer el inodo que referencia el archivo.
// Lo tengo que modificar y volver al buffer
// Lo lleno con la dirección de disco como en mkdir
// Ahora modifico el imap que estaba apuntando a ese inodo
// Deberia quedar en BUFFER: fdata (nuevo), inodo modificado e imapa modificado.
// Despues se ocupara de bajar a disco otra parte 
/*
int append(char * dir, void * txt) { //TERE
	imap * mypimap;
	inode * mypinode;
	void * myidata;

	//Agarro el inodo y le agrego otro segmento de datos
	if(__get_last(dir,mypimap, mypinode, myidata)==-1){
		return -1;
	}
	
}

// borra archivo o directorio (con todo lo que tenga adentro)
int remove(char * dir) {
	// Cambia de acuerdo a si es un archivo o un directorio cambia el comportamiento
	int n,flag=0,count=0;
	imap_entry myimapentry;
	cr_entry * map=(__cp->map);
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
						__put_null((pinode->idata)[k]);
					}
					__put_null(&mydinode);
				}
			}
			if(count==0){
				__put_null(&actualentry.map);
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
		ddata_entry[MAX_DIR_FILES] dmap =((myinode->idata).ddata).map;
		for(i=0; i<MAX_DIR_FILES && dmap[i]!=NULL;i++){
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
	void * mydidata;
	imap * mypimap;
	inode * mypinode;
	int read,fnsize,myinoden;
	char * dir;

	//COMO LLENO LAS ULTIMAS COSAS CUANDO NO ENTRA EN EL CASO FILE?!?!?!?
	while(filename[0]!='\0'){
		fnsize=strlen(filename);
		read=__get_cr_imap_n_inoden(filename, mydimap,myinoden); 
		// ^^^ BUSCA DESDE EL CR, NO SUBE NADA A DISCO PORQUE USA CR_ENTRY
		
		//Cutting the filename - strcut with sprintf
		char * newfilename=malloc((fnsize-i)+1);
		sprintf(newfilename, "%.*s", fnsize-read, filename[read]);
		filename=newfilename;
		free(newfilename);

		read=__get_fst_dir(filename, dir);

		//ATAJO ERRORES
		if(__get_inode_from_imap(mydimap,mydinode,myinoden,mypimap)==-1){
			return -1;
		}else if(__get_data_from_inode(mydinode,mypinode,mytype,mydidata)){
			return -1;
		}; //devuelvo idata y type
		// ^^^ no me tiene que dar el numero de inodo???!?!?!??!

		//TODO: SI ES UN ARCHIVO LO PROXIMO A BUSCAR NO TIENE QUE VOLVER AL WHILE
		//Los archivos no están en el CR!! estan en el imap del directorio al que pertenecen
		if(__is_last_dir(filename)){ //PREGUNTO SI LO QUE QUEDA ES EL ULTIMO
			//LEVANTO LA DATA DE DISCO DEL ULTIMO DIRECTORIO (EL ANTERIOR)
			bool found=false;
			ddata * myddata = __load_ddata(mydidata);
			for(i=0;i<MAX_INODES;i++){ 
				//BUSCO DENTRO DE LOS DATOS DEL DIRECTORIO EL INODO PARA IR AL MAPA
				ddata_entry actual_ddata_entry = (myddata->map)[i];
				if(strcmp(actual_ddata_entry.name,filename)){
					myinoden=actual_ddata_entry.inoden;
					//ENCONTRE EL INODO: BUSCA EN EL IMAPA QUE YA TENGO.
					for(j=0;j<MAX_INODES;j++){
						imap_entry actual_imap_entry = (mypimap->map)[j];
						if(actual_imap_entry.inoden == myinoden){
							//LO ENCONTREEEEEEEEEEEE
							//SUBO LA DATA Y EL INODO
							lastimap=mypimap; //GUARDO EL imapa 
							__get_inode_from_imap(mydimap,mydinode,myinoden,mypimap);
							__get_data_from_inode(mydinode,mypinode,mytype,mydidata);
							lastinode=mypinode;
							myidata = __load_fdata(mydidata);
							return 0;
						}
					}
					found=true; //ERA UN ARCHIVO!!! :)
				}
			}
			if(!found){
				//ERA UN DIRECTORIO Y NO UN ARCHIVO, LO BUSCO DESDE EL CR NORMALMENTE (SIGO EN EL WHILE SIN HACER NADA)
			}
		}

	} 
	lastimap=mypimap;
	lastinode=mypinode;
	myidata=__load_ddata(mydidata); //SI HUBIERA SIDO UN FILE ENTRABA EN EL CASO ANTES
	return 0; //CUANDO FALLA?????
}

bool __is_last_dir(char * dir){
	if(dir[0]=='\0'){ //YA ESTA VACIO!
			return false;
	}
	for(i=0;i<strlen(dir);i++){
		if(dir[i]=='/'){ 
			return false;
		}
	}
	return true;
}

int __get_data_from_inode(dinode mydinode, inode * actualinode, ftype mytype, void * mydidata){
	actualinode = __load_inode(mydinode);
	mytype = actualinode.type;
	mydidata = actualinode.idata;
	return 0;
}

//Having the inode number and the piece of the imap, searchs for the inode
//If there is an error, it returns -1.
int __get_inode_from_imap(dimap mydimap, dinode mydinode, int myinoden, imap * retimap){
	int i;
	for(i=0;i<MAX_INODES;i++){
		imap * pimap=__load_imap(mydimap):
		imap_entry actual = (pimap->map)[i];
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
			myinoden = actualentry.inoden;
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
	if(i==1 && strlen(filename)!=0){
		filename='/';
	}

	dir = malloc(sizeof(char) * (i+1));
	sprintf(dir, "%.*s", i, filename);

	return i;
}

*/

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

void __sync_cr(dptr address) {
	__write(__cp, sizeof(checkpoint), address);
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

bool __is_null(dptr addr) {
	return addr.sector == 0 && addr.offset == 0;
}

void __put_null(dptr * addr){
	addr->sector = 0;
	addr->offset = 0;
}

checkpoint * __new_checkpoint(dptr lstart, dptr lend) {
	checkpoint * cp = malloc(sizeof(checkpoint));
	cp->lstart.sector = lstart.sector;
	cp->lstart.offset = lstart.offset;
	cp->lend.sector = lend.sector;
	cp->lend.offset = lend.offset;
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

void __lnode_append(lntype type, void * data) {
	dptr new_end = __dptr_add(__cp->lend, __sizeof_lntype(type));
	__log_buf_append(__new_lnode(type, data, new_end), __sizeof_lntype(type));
	__set_dptr(__cp->lend, new_end);
}

void __log_buf_append(void * data, int bytes) {
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
	printk("checkpoint:{\n\tlstart: ");
	__print_dptr(&(cp->lstart));
	printk("\n\tlend: ");
	__print_dptr(&(cp->lend));
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

void __lnode_print(lnode * lnptr) {
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