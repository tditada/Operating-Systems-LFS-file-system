#define MAX_INODES 512
#define MAX_IMAP 512

struct data {
	char * text;
	int inum;
	int offset;
	void * next; //next block on the log
};

typedef data * pdata;

struct inode{
	int num;
	pdata[MAX_DATA] idata;
	void * next;
};

typedef inode * pinode;

struct imap{
	pinode[MAX_INODES] map;
	void * next;
};

typedef imap * pimap;

struct checkpoint{
	pimap[MAX_IMAP] map;
	void * headsegment;
	void * tailsegment;
};


int init(){
	// Hacemos el CR en RAM
	// reservar buffer en RAM
	// mkdir de /
}

int mkdir(char * pwd){
	
}