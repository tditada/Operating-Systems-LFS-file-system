#include "fs.h"

//TODO TERE: Testing LFS structure, FREES.

//constructors
static dptr __new_dptr(unsigned short sector, int offset);
static checkpoint * __new_checkpoint(dptr lstart, dptr lend);
static lnode * __new_lnode(lntype type, void * data, dptr next);
//utils
#define __SET(dest, src, size) memcpy(&(dest), &(src), size)
#define __set_dptr(dest, src) __SET(dest, src, sizeof(dptr))
static bool __is_null(dptr addr);
static void __append_to_buf(void * data, int bytes);
static void __stage(lntype type, void * data);
static int __sizeof_lntype(lntype type);
static dptr __dptr_add(dptr address, int bytes);

//main
static dptr __mkdir(char * basename);

static void __add_cr_entry(const char * filename, int inoden, dimap map);
static bool __is_last_dir(char * dir);

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

	printk("Syncing CR...\n");
	__sync_cr(__new_dptr(0, 0));
	printk("...Done\n");

	printk("Syncing log buffer...\n");
	__sync_log_buf();
	printk("...Done\n");

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

	__print_inode(__load_inode(__new_dptr(4, 400)));
	__print_imap(__load_imap(__new_dptr(4, 492)));
	
/*	dinode dinode;
	imap imap;
	__get_inode(&crep.map, 0, &dinode, &imap);
*/	//__print_inode(__load_inode(dinode));
	//__print_imap(&imap);

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
	prev = __mkdir("/");
	__add_cr_entry("/", __inoden, prev);
	__inoden++;
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

/*dptr mkdir*/

// /!\ NOT the filename!
dptr __mkdir(char * basename) {
	ddata data;
	inode inode;
	imap imap;

	dptr prev = __cp->lend;
	__stage(FS_DDATA, &data);

	inode.num = __inoden;
	inode.type = FS_DIR;
	__set_dptr(inode.idata[0], prev);
	prev = __cp->lend;
	__stage(FS_INODE, &inode);

	imap.map[0].inoden = __inoden;
	__set_dptr(imap.map[0].inode, prev);
	prev = __cp->lend;
	__stage(FS_IMAP, &imap);

	__print_ddata(&data);
	printk("\n");
	__print_inode(&inode);
	printk("\n");
	__print_imap(&imap);
	printk("\n");
	__print_dptr(&prev);
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
	imap * _pimap;
	inode * _pinode;
	void * _idata;
	int inodedir, imapdir;

	//Agarro el inodo y le agrego otro segmento de datos
	if (__get_last(dir,_pimap, _pinode, _idata)==-1) {
		return -1;
	}
	int _inoden=_pinode->num;
	//Recorro el inodo _pinode
	for(i=0;i<=MAX_INODES;i++){
		didata _ddata = _pinode->idata[i];
		if(__is_null(_ddata)){
			//EL PRIMERO QUE ES NULL, CREO UNA NUEVA FDATA CON EL TXT		
			fdata * _newpfdata;
			_newpfdata->data=txt;
			dptr prev = __cp->ledn; //Direccion en la que esta
			__lnode_append(FS_FDATA,_newpfdata);
			__set_dptr(_pinode->idata[i], prev);
			//EN EL INODO TENGO QUE CAMBIAR EL DATO Y APENDEARLO DE NUEVO
			inodedir=prev+sizeof(*_newpfdata);
			imapdir=inode+sizeof(*_pinode);
			__lnode_append(FS_INODE,_pinode);
			//TENGO QUE CAMBIAR EL CACHO DE IMAPA QUE APUNTABA AL INODO QUE APPENDEE			
			//COMO LO TENGO SUBIDO A MEMORIA PUEDO RECORRERLO TRANQUILAMENTE
			for(j=0;j<=MAX_IMAP;j++){
				if((_pimap->map[j]).inoden==_inoden){	
					__set_dptr((_pimap->map[j]).inode,inodedir);
					__lnode_append(FS_IMAP,_pimap);
				}
			}
			for(j=0;j<MAX_IMAP;j++){ 	//RECORRO EL CR
				cr_entry actual = __cp->map[j];
				if(actual.inoden==_inoden){
					__set_dptr(actual.dimap,imapdir);
				}
			}
		}
	}
	
}

// borra archivo o directorio (con todo lo que tenga adentro)
int remove(char * dir) {
	// Cambia de acuerdo a si es un archivo o un directorio cambia el comportamiento
	int n,flag=0,count=0;
	imap_entry _imapentry;
	cr_entry * map=(__cp->map);
	inode * pinode;
	// Borra el puntero al inodo del imapa.. 
	for (i=0;i<=MAX_IMAP;i++) { //RECORRO EL CR
		cr_entry entry=map[i];
		if (streq(entry.dir_name, dir)) {
			n=entry.inoden;
			imap * _pimap = __load_imap(entry.map)
			for (j=0; j<=MAX_INODES j++) { //RECORRO EL IMAP
				_imapentry = (_imap->map)[j];
				dinode _dinode=_imapentry.inode;
				if (_dinode.sector != 0 && _dinode.offset != 0) {
					count++; //Hay algo en el sector
				}
				if (_imapentry.inoden=n) {
					count--;
					pinode=__load_inode(_dinode); //CARGO EL INODO
					for (k=0;k<MAX_IDATA;k++) { //BORRO TODA LA DATA DEL INODO
						__put_null((pinode->idata)[k]);
					}
					__put_null(&_dinode);
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


//Returns -1 if it doesn't exist
//Gets the inode number searching in the directory data for a char * file
int __get_inode_from_directory(dinode _inode, char * name) {
	if (!__is_inode_dir(_inode)) {
		return -1; 
	} else {
		ddata_entry[MAX_DIR_FILES] dmap =((_inode->idata).ddata).map;
		for (i=0; i<MAX_DIR_FILES && dmap[i]!=NULL;i++) {
			if (streq(dmap[i],name)) {
				return dmap[i].inoden;
			}
		}
		return -1;
	}
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

