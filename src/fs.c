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

int __get_last_inode(char * filename) {
	//aca se hace todo el "" hasta consumir todo el camino y llegar al inode que vos querias: 
	// /home/pepe/sarasa/foo.txt te da el inode de foo.txt
	pinode myinode;
	int i,fnsize;
	fnsize=strlen(filename);
	i=__get_fst_inode(filename, myinode);

	//Cutting the filename - strcut with sprintf
	char * newfilename=malloc((fnsize-i)+1);
	sprintf(newfilename, "%.*s", fnsize-i, filename[i]);

	if(newfilename[0]=="\0"){
		//THE END!! File or directory
	}

	//Recorrer el resto de los bloques (por dentro!)
	//Si no es el final, debe ser si o si una carpeta
	char * dir;
	__get_fst_dir(newfilename, dir);
	__get_dir(myinode);
}


// Busca el primer inode desde el CR (sea file o sea directory. Arreglate vos)
// La idea sería obtener el inode del primer directorio para ir mapeando desde ahí
int __get_fst_inode(char * filename, pinode inode) {
	char * dir;
	int read, inoden;

	read = __get_fst_dir(filename, &dir);
	if(strcmp(filename, "/")){
		//peola
	}else if(strcmp(filename,".")|| strcmp(filename, "..")){
		//We have to search the CR with pwd or the one before the pwd
		//TODO:strig compare
	}else{
		//caso Tere/Downloads
		//agregar el pwd
	}
	inode = crp->map[inoden]->map[inoden];
	//Arreglar bajada a disco

	return read;
}

// TODO: . and .. !! 

// Gets the first director copying everything before /
//Returns number of chars read
int __get_fst_dir(char * filename, char * dir) {
	int i=1;
	while (filename[i-1] != '\0' && filename[i-1] != '/') {
		i++;
	}

	dir = malloc(sizeof(char) * (i+1));
	sprintf(dir, "%.*s", i, filename);

	return i;
}