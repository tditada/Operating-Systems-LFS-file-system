#include "fs.h"

#define __LOAD(type, addr) ((type *)__load(addr, sizeof(type)))

bool __is_null(disk_addr addr);
void __sync_log_buf();
void __sync_cr(disk_addr address);
void __write(void * data, int bytes, disk_addr address);
disk_addr __disk_addr_new(unsigned short sector, int offset);

void * __load(disk_addr addr, int bytes);
#define __load_imap(addr) __LOAD(imap, addr)
#define __load_inode(addr) __LOAD(inode, addr)
#define __load_ddata(addr) __LOAD(ddata, addr)
#define __load_fdata(addr) __LOAD(fdata, addr)

checkpoint __checkpoint_new(disk_addr lstart, disk_addr lend);

static int __drive;

// Hacemos el CR en RAM
static checkpoint * __cp;

// reservar buffer en RAM
static char __log_buf[BUFFER_SIZE];
static int __log_buf_size = 0;

static char __path_buffer[MAX_PATH];


/*TODO: static char * pwd;*/

int create(int drive, unsigned short sector, int offset){
	// mkdir de /
	// mkdir("/");
	__drive = drive;

	__cp = malloc(sizeof(checkpoint));
	__cp->l_start = ;
	__cp->l_end = ;
}




int init() {
	//aca cargas de disco el cr en cr 
}

int mkdir(char * filename){
	// aca hago un lookup, primero en el buffer
	int i;
	char * s;
	

}

//TODO: como escribir en el __log_buf (memcpy al final o castear el final al tipo que tengas, trabajar ahi y listo)
//TODO: sync: baja a disco

int touch() {
	// mismo que mkdir pero para archivos
}

int append() {
	// escribe al final del archivo
}

int remove() {
	// borra archivo o directorio (con todo lo que tenga adentro)
}

bool __is_inode_dir(pinode myinode) {
	if((*pinode).type)==FS_DIR){
		return true; 
	} return false;
}

bool __is_inode_file(pinode myinode) {
	if((*pinode).type)==FS_FILE){
		return true; 
	} return false;
}

//Returns -1 if it doesn't exist
//Gets the inode number searching in the directory data for a char * file
int __get_inode_from_directory(pinode myinode, char * name){
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

//aca se hace todo el "" hasta consumir todo el camino y llegar al inode que vos querias: 
// /home/pepe/sarasa/foo.txt te da el inode de foo.txt
int __get_last_inode(char * filename) {
	pimap myimap;
	pinode myinode;
	int read,fnsize;
	fnsize=strlen(filename);
	read=__get_fst_imap(filename, myimap);

	//Cutting the filename - strcut with sprintf
	char * newfilename=malloc((fnsize-i)+1);
	sprintf(newfilename, "%.*s", fnsize-i, filename[i]);

	if(newfilename[0]=="\0"){
		//THE END!! File or directory
	}
	//Recorrer el resto de los bloques (por dentro!)
	//Si no es el final, debe ser si o si una carpeta
	char * dir;
	read=__get_fst_dir(newfilename, dir);
	__get_inode_from_directory(myinode,dir);
}

//Having the inode number and the piece of the imap, searchs for the inode
//If there is an error, it returns -1.
int __get_inode_from_imap(pimap myimap, pinode myinode, int myinoden){
	for(i=0;i<MAX_INODES;i++){
		pimap[MAX_IMAP] actual = myimap->map[i];
		if(actual==NULL){
			return -1;	
		}else{
			if(actual.inoden==myinoden){
				myinode=actual.inode;
				return actual.inoden;
			}
		}
	}
	return -1;
}

//TODO: . y .. !!
// Busca el primer inode desde el CR (sea file o sea directory. Arreglate vos)
// La idea sería obtener el inode del primer directorio para ir mapeando desde ahí
int __get_fst_imap(char * filename, pinode inode) {
	char * dir;
	int read;

	read = __get_fst_dir(filename, &dir);
	if(strcmp(filename, "/")){
		//peola
	}else if(strcmp(filename,".")|| strcmp(filename, "..")){
		//We have to search the CR with pwd or the one before the pwd
		//TODO:string compare
	}else{
		//caso Tere/Downloads
		//agregar el pwd
	}
	cr_entry[MAX_IMAP] localmap =(cp->map);
	for (i=0;i<=MAX_IMAP && localmap[i]!=NULL;i++){
		cr_entry actualentry=localmap[i];
		if(strcmp(actualentry,dir)){
			pimap = localmap.myimap;
		}
	}
	//Arreglar bajada a disco
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

void __sync_log_buf() {
	int i, bytes;
	for (i=0; __log_buf_size-(i*SECTOR_SIZE)>0; i++) {
		bytes = min(__log_buf_size, SECTOR_SIZE);
		__write(__log_buf+i*SECTOR_SIZE, bytes, __cp->l_end);
		__cp->l_end.sector += bytes / SECTOR_SIZE;
		__cp->l_end.offset = (__cp->l_end.offset + bytes) % SECTOR_SIZE;
	}
	__log_buf_size = 0;
}

void __sync_cr(disk_addr address) {
	__write(*__cp, sizeof(checkpoint), address);
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

void * __load(disk_addr addr, int bytes) {
	void * data;
	ata_read(__drive, data, bytes, addr.sector, addr.offset);
	return data;
}

checkpoint __checkpoint_new(disk_addr lstart, disk_addr lend) {
	checkpoint c;
	c.lstart.sector = lstart.sector;
	c.lstart.offset = lstart.offset;
	c.lend.sector = lend.sector;
	c.lend.offset = lend.offset;
	return c;
}

