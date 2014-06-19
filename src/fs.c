#include "fs.h"

// Hacemos el CR en RAM
static checkpoint * crp;
// reservar buffer en RAM
static char log_buffer[MAX_BUFFER_SIZE];

static char path_buffer[MAX_PATH];

static char * pwd;

int create(){
	mkdir("/");
	// mkdir de /
}

int init() {
	//aca cargas de disco el cr en cr 
}

int mkdir(char * filename){
	// aca hago un lookup, primero en el buffer
	int i;
	char * s;
	

}

//TODO: como escribir en el log_buffer (memcpy al final o castear el final al tipo que tengas, trabajar ahi y listo)
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

int __get_dir() {
	// llama a get last inode y castea a carpeta. Handlea errores
}

int __get_file() {
	// llama a get last inode y castea a file. Handlea errores
}

int __get_last_inode() {
	//aca se hace todo el "" hasta consumir todo el camino y llegar al inode que vos querias: 
	// /home/pepe/sarasa/foo.txt te da el inode de foo.txt
}

// la idea aca es que te da el inode (sea lo que sea que vos querias, sea file o sea directory. Arreglate vos)
int __get_next_inode(char * filename, pinode * inode) {
	char * dir;
	int read, inoden;

	read = __get_fst_dir(&dir);
	inode = crp->map[inoden]->map[inoden];
}

// TODO: . and .. !! 

/*
usage:
	char * dir;

	filename += __get_fst_dir(&dir);
*/
int __get_fst_dir(char * filename) {
	int i = 1;

	while (filename[i-1] != '\0' && filename[i-1] != '/') {
		i++;
	}

	dir = malloc(sizeof(char) * (i+1));
	sprintf(dir, "%.*s", i, filename);

	return i;
}