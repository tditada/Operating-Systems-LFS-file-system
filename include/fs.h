#ifndef FS_H
#define FS_H

#include "kernel.h"
#include "disk.h"

#define DATA_BLOCK_SIZE 128
#define MAX_IDATA 8
#define MAX_INODES 512
#define MAX_IMAP 512 // ??? shouldn't they be the same?
#define BUFFER_SIZE 5*SECTOR_SIZE // get an actual number for this, there's a formula!
#define MAX_PATH 1024
#define MAX_FILENAME 64

typedef struct {
	unsigned short sector;
	int offset;
} disk_addr;

typedef struct {
	char data[DATA_BLOCK_SIZE];
} fdata;

typedef struct {
	char name[MAX_FILENAME];
	int inoden;
} ddata_entry;

typedef struct {
	ddata_entry map[MAX_INODES];
} ddata;

typedef enum {
	FS_FILE, FS_DIR
} ftype;

typedef disk_addr pidata;

typedef struct {
	int num;
	ftype type;
	pidata idata[MAX_IDATA];
	int fsize;
} inode;

typedef disk_addr pinode;

typedef struct {
	int inoden;
	pinode inode;
} imap_entry;

typedef struct {
	imap_entry map[MAX_INODES];
} imap;

typedef disk_addr pimap;

typedef struct {
	char dir_name[MAX_FILENAME];
	int inoden;
	pimap map;
} cr_entry;

typedef struct {
	cr_entry map[MAX_IMAP];
	disk_addr lstart;
	disk_addr lend;
} checkpoint;

typedef enum {
	FS_IMAP, FS_INODE
} ntype;

typedef struct {
	ntype type;
	disk_addr next;
	char data[0];
} lnode;

// Hacemos el CR en RAM
// reservar buffer en RAM
// mkdir de /
void init();
void create(int drive, int size);
int testfs();

/*int fs_mkdir(char * path);*/

#endif