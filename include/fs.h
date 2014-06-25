#ifndef FS_H
#define FS_H

#include "kernel.h"
#include "disk.h"

#define FS_DRIVE ATA0
#define DATA_BLOCK_SIZE 64
#define MAX_INODES 2
#define MAX_IMAP 64
#define FS_BUFFER_SIZE 12*sizeof(lnode)
#define MAX_DIR_FILES 8
#define MAX_PATH 256
#define MAX_FILENAME 64
#define MAX_LNODE_DATA_SIZE max(sizeof(inode),\
							max(sizeof(imap),\
							max(sizeof(ddata),\
							sizeof(fdata))))

typedef struct {
	unsigned short sector;
	int offset;
} dptr;

typedef struct {
	char data[DATA_BLOCK_SIZE];
} fdata;

typedef struct {
	char name[MAX_PATH];
	int inoden;
} ddata_entry;

typedef struct {
	ddata_entry map[MAX_DIR_FILES];
} ddata;

typedef enum {
	FS_FILE, FS_DIR
} ftype;

typedef dptr didata;

typedef struct {
	int num;
	ftype type;
	didata idata;
	int fsize;
} inode;

typedef dptr dinode;

typedef struct {
	int inoden;
	dinode inode;
} imap_entry;

typedef struct {
	imap_entry map[MAX_INODES];
} imap;

typedef dptr dimap;

typedef struct {
	char filename[MAX_FILENAME];
	int inoden;
	dimap map;
} cr_entry;

typedef struct {
	cr_entry map[MAX_IMAP];
	dptr lstart;
	dptr lend;
	dptr lfst;
	int lsize;
} checkpoint;

typedef enum {
	FS_IMAP, FS_INODE, FS_DDATA, FS_FDATA
} lntype;

typedef struct {
	lntype type;
	dptr next;
	char data[MAX_LNODE_DATA_SIZE];
} lnode;


// Elimina informacion vieja en el disco y crea un nuevo filesystem de tamaño 'size'
// desde el comienzo del disco.
int fs_creat(int size);
// Carga un filesystem preexistente
int fs_init();
// Muestra informacion sobre el filesystem
int fs_data();
// Verifica si un archivo existe
bool fs_fexists(char * dir);
// Baja tanto el checkpoint registry como el log buffer a disco
int fs_sync();
// Baja el checkpoint registry a disco
int fs_sync_cr();
// Baja el buffer del log a disco
int fs_sync_lbuf();
// cat de UNIX
int fs_cat(char * dir);
// ls de UNIX, solo funciona con paths absolutos
int fs_list(char * dir);
// Crea un archivo (de texto), o un directorio
int fs_mkfile(char * filename, ftype type, void * data, int bytes);
// Muestra el estado del checkpoint registry
int fs_print_cr();
// Muestra el fragmento de imapa de 'filename'
int fs_print_imap(char * filename);
// Muestra el numero de inodo de 'filename'
int fs_print_inoden(char * filename);
// Muestra el estado de los primeros 'len' bloques del log 
int fs_print_log(int len);
// Ejecuta el recolector de basura, analiza 'len' bloques. 
// De encontrar uno vivo, lo reposiciona.
int fs_run_gc(int len);
// Borra archivo o directorio (con todo lo que tenga adentro)
int fs_remove(char * dir);

extern checkpoint * __cp;
extern char __log_buf[FS_BUFFER_SIZE];
extern int __log_buf_size;

#endif