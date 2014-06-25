#ifndef FS_H
#define FS_H

#include "kernel.h"
#include "disk.h"

#define FS_DRIVE ATA0
#define DATA_BLOCK_SIZE 128
#define MAX_INODES 2
#define MAX_IMAP 128
#define FS_BUFFER_SIZE 16*MAX_LNODE_SIZE // get an actual number for this, there's a formula!
#define MAX_DIR_FILES 8
#define MAX_PATH 256
#define MAX_FILENAME 64
#define MAX_LNODE_SIZE max(sizeof(inode),\
						max(sizeof(imap),\
						max(sizeof(ddata),\
							sizeof(fdata)))) // TODO: check if these cases are all!!

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
	int lsize;
} checkpoint;

typedef enum {
	FS_IMAP, FS_INODE, FS_DDATA, FS_FDATA
} lntype;

typedef struct {
	lntype type; // /!\ MUST BE FIRST!
	dptr next;
	char data[0];
} lnode;

// Hacemos el CR en RAM
// reservar buffer en RAM
// mkfile de /
int init();
void create(int size);
int testfs();
bool file_existence(char * dir);
int sync_cr();
int sync_lbuf();
int fs_mkdir(char * filename);
int fs_cat(char * dir);
int fs_list(char * dir);
int fs_mkfile(char * filename, ftype type, void * data, int bytes);

extern checkpoint * __cp;
extern char __log_buf[FS_BUFFER_SIZE];
extern int __log_buf_size;
extern lnode * __log_buf_list[FS_BUFFER_SIZE/sizeof(lnode)];

#endif