#ifndef FS_H
#define FS_H

#include "kernel.h"
#include "disk.h"

#define DATA_BLOCK_SIZE 128
#define MAX_IDATA 10 // Unix default
#define MAX_INODES 512
#define MAX_IMAP 128 // ??? shouldn't they be the same?
#define BUFFER_SIZE 20*MAX_LNODE_SIZE // get an actual number for this, there's a formula!
#define MAX_DIR_FILES 36
#define MAX_PATH 1024
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
	char name[MAX_FILENAME];
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
	didata idata[MAX_IDATA];
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
	char dir_name[MAX_FILENAME];
	int inoden;
	dimap map;
} cr_entry;

typedef struct {
	cr_entry map[MAX_IMAP];
	dptr lstart;
	dptr lend;
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
// mkdir de /
void init();
void create(int drive, int size);
int testfs();
bool __search_cr(char * dir);

/*int fs_mkdir(char * path);*/

#endif