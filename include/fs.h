#ifndef FS_H
#define FS_H
#include "kernel.h"

#define DATA_BLOCK_SIZE 128
#define MAX_INODES 512
#define MAX_IMAP 512 // ??? shouldn't they be the same?
#define BUFFER_SIZE 5*SECTOR_SIZE // get an actual number for this, there's a formula!
#define MAX_PATH 1024
#define MAX_FILENAME 64

typedef struct {
	char[DATA_BLOCK_SIZE] data;
} fdata;

typedef struct {
	char * name;
	int inoden;
} ddata_entry;

typedef struct {
	ddata_entry[MAX_INODES] map;
} ddata;

typedef enum {
	FS_FILE, FS_DIR
} ftype;

typedef disk_addr pidata;

typedef struct {
	int num;
	ftype type;
	pidata[MAX_IDATA] idata;
	int fsize;
} inode;

typedef disk_addr pinode;

typedef struct {
	int inoden;
	pinode inode;
} imap_entry;

typedef struct {
	imap_entry[MAX_INODES] map;
} imap;

typedef disk_addr pimap;

typedef struct {
	char * dir_name;
	int inoden;
	pimap map;
} cr_entry;

typedef struct {
	cr_entry[MAX_IMAP] map;
	disk_addr lstart;
	disk_addr lend;
} checkpoint;

typedef struct {
	FS_IMAP, FS_INODE
} ntype;

typedef struct {
	ntype type;
	ddata_addr next;
	char data[0];
} lnode;

typedef struct {
	unsigned short sector;
	int offset;
} disk_addr;

// Hacemos el CR en RAM
// reservar buffer en RAM
// mkdir de /
int fs_init();

int fs_mkdir(char * path);

#endif