#ifndef FS_H
#define FS_H
#include "kernel.h"

#define MAX_INODES 512
#define MAX_IMAP 512 // ??? shouldn't they be the same?
#define BUFFER_SIZE 5*SECTOR_SIZE // get an actual number for this, there's a formula!
#define MAX_PATH 256

typedef struct {
	char * text;
	int inum;
	int offset;
	void * next; //next block on the log
} file_data;

typedef struct {
	char * name;
	int inoden;
} dir_data_pair;

typedef struct {
	dir_data_pair[MAX_INODES] mdata;
	int inum;
	int offset;
	void * next; //next block on the log
} dir_data;

typedef enum {
	FS_FILE, FS_DIR
} ftype;

typedef struct{
	dir_data * ddata;
	file_data * fdata;
} idata;

typedef struct {
	int num;
	ftype type;
	char[MAX_DATA] idata;
	void * next;
} inode;

typedef inode * pinode;

typedef struct {
	int inoden;
	pinode * inode;
} imap_pair;

typedef struct {
	imap_pair[MAX_INODES] map;
	void * next;
} imap;

typedef imap * pimap;

typedef struct {
	char * dir_name;
	int inoden;
	pimap myimap;
}cr_entry;

typedef struct {
	cr_entry[MAX_IMAP] map;
	disk_addr lstart;
	disk_addr lend;
} checkpoint;

// Hacemos el CR en RAM
// reservar buffer en RAM
// mkdir de /
int fs_init();

int fs_mkdir(char * path);

#endif