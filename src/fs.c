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
//utils
#define __SET(dest, src, size) memcpy(&(dest), &(src), size)
#define __set_dptr(dest, src) __SET(dest, src, sizeof(dptr))
static bool __is_null(dptr addr);
static void __stage(void * data, int bytes);
static void __append(lntype type, void * data);
static int __sizeof_lntype(lntype type);
static dptr __dptr_add(dptr address, int bytes);
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
static int __get_fst_dir(char * filename, char * dir);
static int __get_cr_entry(char * filename, cr_entry * creptr);
static int __get_inode(dimap dimap, dinode * dinode, int inoden, imap * retimap);
//main
static dptr __mkdir(char * basename);

static void __add_cr_entry(const char * filename, int inoden, dimap map);

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

void create(int drive, int size) {
	dptr start, prev;

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

/*	char * dir;
	printk("%d, %s\n", __get_fst_dir("/d1/d2/d3/f1", dir), dir);
	printk("%d, %s\n", __get_fst_dir("d1/d2/d3/f1", dir), dir);
*/

	prev = __mkdir("/");
	__add_cr_entry("/", __inoden, prev);
	__inoden++;

	printk("__get_cr_entry...\n");
	cr_entry crep;
	printk("ret:%d\n", __get_cr_entry("/", &crep));
	__print_cr_entry(&crep);
}

void init() {
	printk("Starting FS...\n");
	__log_buf_size = 0;
	__cp = __load_checkpoint(__new_dptr(0, 0));
	__print_checkpoint(__cp);


	printk("\n");
	printk("...Done\n");
}

/*dptr mkdir*/


// /!\ NOT the filename!
dptr __mkdir(char * basename) {
	ddata data;
	inode inode;
	imap imap;

	dptr prev = __cp->lend;
	__append(FS_DDATA, &data);

	inode.num = __inoden;
	inode.type = FS_DIR;
	__set_dptr(inode.idata[0], prev);
	prev = __cp->lend;
	__append(FS_INODE, &inode);

	imap.map[0].inoden = __inoden;
	__set_dptr(imap.map[0].inode, prev);
	prev = __cp->lend;
	__append(FS_IMAP, &imap);

	__print_ddata(&data);
	printk("\n");
	__print_inode(&inode);
	printk("\n");
	__print_imap(&imap);
	printk("\n");
	return prev;
}

void __add_cr_entry(const char * dirname, int inoden, dimap map) {
	int i;
	cr_entry * creptr;
	for (i=0; i<MAX_IMAP; i++) {
		creptr = &__cp->map[i];
		if (__is_null(creptr->map)) {
			creptr->inoden = inoden;
			strcpy(creptr->dir_name, dirname);
			__set_dptr(creptr->map, map);
			return;
		}
	}
	//must check for the i >= MAX_IMAP elsewhere!
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
	int inodedir;

	//Agarro el inodo y le agrego otro segmento de datos
	if (__get_last(dir,mypimap, mypinode, myidata)==-1) {
		return -1;
	}

	//Recorro el inodo mypinode
	for(i=0;i<=MAX_INODES;i++){
		didata myddata = mypinode->idata[i];
		if(__is_null(myddata)){
			//EL PRIMERO QUE ES NULL, CREO UNA NUEVA FDATA CON EL TXT		
			fdata * mynewpfdata;
			mynewpfdata->data=txt;
			dptr prev = __cp->ledn; //Direccion en la que esta
			__lnode_append(FS_FDATA,mynewpfdata);
			__set_dptr(mypinode->idata[i], prev);
			//EN EL INODO TENGO QUE CAMBIAR EL DATO Y APENDEARLO DE NUEVO
			inodedir=prev+sizeof(*mynewpfdata);
			__lnode_append(FS_INODE,mypinode);
			//TENGO QUE CAMBIAR EL CACHO DE IMAPA QUE APUNTABA AL INODO QUE APPENDEE			
			//COMO LO TENGO SUBIDO A MEMORIA PUEDO RECORRERLO TRANQUILAMENTE
			for(j=0;j<=MAX_IMAP;j++){
				if((mydimap->map[j]).inoden==pinode->num){	
					//RECORRO EL CR
				}
			}
		}
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
	for (i=0;i<=MAX_IMAP;i++) { //RECORRO EL CR
		cr_entry entry=map[i];
		if (streq(entry.dir_name, dir)) {
			n=entry.inoden;
			imap * mypimap = __load_imap(entry.map)
			for (j=0; j<=MAX_INODES j++) { //RECORRO EL IMAP
				myimapentry = (myimap->map)[j];
				dinode mydinode=myimapentry.inode;
				if (mydinode.sector != 0 && mydinode.offset != 0) {
					count++; //Hay algo en el sector
				}
				if (myimapentry.inoden=n) {
					count--;
					pinode=__load_inode(mydinode); //CARGO EL INODO
					for (k=0;k<MAX_IDATA;k++) { //BORRO TODA LA DATA DEL INODO
						__put_null((pinode->idata)[k]);
					}
					__put_null(&mydinode);
				}
			}
			if (count==0) {
				__put_null(&entry.map);
			}
		}		
	}

	// Si el imapa queda vacío borro también el puntero del CR al imapa
	return 1;
}

bool __is_inode_dir(inode * myinode) {
	if (myinode->type)==FS_DIR) {
		return true; 
	} return false;
}

bool __is_inode_file(inode * myinode) {
	if (myinode->type)==FS_FILE) {
		return true; 
	} return false;
}

//Returns -1 if it doesn't exist
//Gets the inode number searching in the directory data for a char * file
int __get_inode_from_directory(dinode myinode, char * name) {
	if (!__is_inode_dir(myinode)) {
		return -1; 
	} else {
		ddata_entry[MAX_DIR_FILES] dmap =((myinode->idata).ddata).map;
		for (i=0; i<MAX_DIR_FILES && dmap[i]!=NULL;i++) {
			if (streq(dmap[i],name)) {
				return dmap[i].inoden;
			}
		}
		return -1;
	}
}


//CUANDO CONSIGO EL INODO EN EL CR PARA USAR EL IMAP
int __get_last(char * filename, imap * lastpimap, inode * lastpinode, void * mydidata) {

	cr_entry mycrentry
	dimap mydimap;
	dinode mydinode;
	ftype mytype;
	void * mydidata;
	imap * mypimap;
	inode * mypinode;
	int read,fnsize,myinoden;
	char * dir;

	//COMO LLENO LAS ULTIMAS COSAS CUANDO NO ENTRA EN EL CASO FILE?!?!?!?
	while(filename[0]!='\0') {
		fnsize=strlen(filename);
		read=__get_cr_entry(filename, ); 
		// ^^^ BUSCA DESDE EL CR, NO SUBE NADA A DISCO PORQUE USA CR_ENTRY
		
		//Cutting the filename - strcut with sprintf
		char * newfilename=malloc((fnsize-i)+1);
		sprintf(newfilename, "%.*s", fnsize-read, filename[read]);
		filename=newfilename;
		free(newfilename);

		read=__get_fst_dir(filename, dir);

		//ATAJO ERRORES
		if (__get_inode(mydimap,mydinode,myinoden,mypimap)==-1) {
			return -1;
		}else if (__get_data_from_inode(mydinode,mypinode,mytype,mydidata)) {
			return -1;
		}; //devuelvo idata y type
		// ^^^ no me tiene que dar el numero de inodo???!?!?!??!

		//TODO: SI ES UN ARCHIVO LO PROXIMO A BUSCAR NO TIENE QUE VOLVER AL WHILE
		//Los archivos no están en el CR!! estan en el imap del directorio al que pertenecen
		if (__is_last_dir(filename)) { //PREGUNTO SI LO QUE QUEDA ES EL ULTIMO
			//LEVANTO LA DATA DE DISCO DEL ULTIMO DIRECTORIO (EL ANTERIOR)
			bool found=false;
			ddata * myddata = __load_ddata(mydidata);
			for (i=0;i<MAX_INODES;i++) { 
				//BUSCO DENTRO DE LOS DATOS DEL DIRECTORIO EL INODO PARA IR AL MAPA
				ddata_entry actual_ddata_entry = (myddata->map)[i];
				if (streq(actual_ddata_entry.name,filename)) {
					myinoden=actual_ddata_entry.inoden;
					//ENCONTRE EL INODO: BUSCA EN EL IMAPA QUE YA TENGO.
					for (j=0;j<MAX_INODES;j++) {
						imap_entry actual_imap_entry = (mypimap->map)[j];
						if (actual_imap_entry.inoden == myinoden) {
							//LO ENCONTREEEEEEEEEEEE
							//SUBO LA DATA Y EL INODO
							lastimap=mypimap; //GUARDO EL imapa 
							__get_inode(mydimap,mydinode,myinoden,mypimap);
							__get_data_from_inode(mydinode,mypinode,mytype,mydidata);
							lastpinode=mypinode;
							mypidata = __load_fdata(mydidata);
							return 0;
						}
					}
					found=true; //ERA UN ARCHIVO!!! :)
				}
			}
			if (!found) {
				//ERA UN DIRECTORIO Y NO UN ARCHIVO, LO BUSCO DESDE EL CR NORMALMENTE (SIGO EN EL WHILE SIN HACER NADA)
			}
		}

	} 
	lastpimap=mypimap;
	lastpinode=mypinode;
	mypidata=__load_ddata(mydidata); //SI HUBIERA SIDO UN FILE ENTRABA EN EL CASO ANTES
	return 0; //CUANDO FALLA?????
}

bool __is_last_dir(char * dir) {
	if (dir[0]=='\0') { //YA ESTA VACIO!
			return false;
	}
	for (i=0;i<strlen(dir);i++) {
		if (dir[i]=='/') { 
			return false;
		}
	}
	return true;
}

int __get_data_from_inode(dinode mydinode, inode * actualinode, ftype mytype, void * mydidata) {
	actualinode = __load_inode(mydinode);
	mytype = actualinode.type;
	mydidata = actualinode.idata;
	return 0;
}
*/
//Having the inode number and the piece of the imap, searchs for the inode
//If there is an error, it returns -1.
int __get_inode(dimap dimap, dinode * dinode, int inoden, imap * retimap) {
	int i;
	imap * pimap;
	imap_entry * curr;

	for (i=0; i<MAX_INODES; i++) {
		pimap = __load_imap(dimap);
		curr = &(pimap->map[i]);
		if (curr == NULL) {
			return -1;
		} else if (curr->inoden == inoden) {
			//No sube el inode a disco porque todavía no lo uso
			*dinode = curr->inode;
			*retimap = *pimap;
			return curr->inoden;
		}
	}
	return -1;
}

//TODO: . y .. !!
// Busca el primer imap desde el CR (sea file o sea directory. Arreglate vos)
// La idea sería obtener el imap del primer directorio para ir mapeando desde ahí
int __get_cr_entry(char * filename, cr_entry * crep) {
	char dir[MAX_FILENAME];
	int i, read = __get_fst_dir(filename, dir);
	/*if (streq(filename, "/")) {
		//peola
	} else if (streq(filename,".")
		|| streq(filename, "..")) {
		//We have to search the CR with pwd or the one before the pwd
	} else {
		//caso Tere/Downloads
		//agregar el pwd
	}*/
	cr_entry * map = __cp->map;
	for (i=0; i<=MAX_IMAP && !__is_null(map[i].map);i++) {
		if (streq(map[i].dir_name, dir)) {
			*crep = map[i];
			return read;
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

void __put_null(dptr * addr) {
	addr->sector = 0;
	addr->offset = 0;
}

checkpoint * __new_checkpoint(dptr lstart, dptr lend) {
	checkpoint * cp = malloc(sizeof(checkpoint));
	__cp->lstart.sector = lstart.sector;
	__cp->lstart.offset = lstart.offset;
	__cp->lend.sector = lend.sector;
	__cp->lend.offset = lend.offset;
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

void __append(lntype type, void * data) {
	dptr new_end = __dptr_add(__cp->lend, __sizeof_lntype(type));
	__stage(__new_lnode(type, data, new_end), __sizeof_lntype(type));
	__set_dptr(__cp->lend, new_end);
}

void __stage(void * data, int bytes) {
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