#include "fs.h"

//lookups
static int __get_fst_dirname(char * filename, char * dir);
static int __get_cr_entry(char * filename, cr_entry * creptr);
static int __get_inode(dimap * dimap, int inoden, dinode * retdinode, imap * retimap);
static int __get_last(char * filename, imap * lastpimap, inode * lastpinode, ddata * _didata);
static int __get_data_from_inode(dinode * _dinode, inode * actualinode, ftype * _type, void ** _didata);
static int __get_inode(dimap * dimptr, int inoden, dinode * retdinode, imap * retimap);
//iteration
static lnode * __next_lnode_buf(int * i);
static lnode * __next_lnode(lnode * curr);

//For SHELL use (cd command). Given a direction, searchs for it in the cr
bool __search_cr(char * dir){
	int i;
	for(i=0;i<MAX_IMAP;i++){
		if(strcmp((__cp->map)[i].dir_name, dir)){
			return true;
		}	
	}
	return false;
}

//CUANDO CONSIGO EL INODO EN EL CR PARA USAR EL IMAP
int __get_last(char * filename, imap * lastpimap, inode * lastpinode, ddata * _didata) {
	cr_entry _crentry;
	dinode _dinode;
	dimap _dimap;
	ftype _type;
	imap * _pimap;
	inode * _pinode;
	int read, fnsize;
	char * dir, * newfilename;

	//COMO LLENO LAS ULTIMAS COSAS CUANDO NO ENTRA EN EL CASO FILE?!?!?!?
	while(filename[0] != '\0') {
		fnsize = strlen(filename);
		read = __get_cr_entry(filename, &_crentry); 
		// ^^^ BUSCA DESDE EL CR, NO SUBE NADA A DISCO PORQUE USA CR_ENTRY
		
		/* TODO: cambiar lo del malloc de filename por 
		 newfilename = filename
		 ... 
		 newfilename += read
		*/

		//Cutting the filename - strcut with sprintf
		newfilename = malloc(fnsize-read+1);
		memcpy(newfilename, filename+read, fnsize-read);
		newfilename[fnsize-read+1] = '\0';

		filename = newfilename;
		free(newfilename);

		read = __get_fst_dirname(filename, dir);

		//ATAJO ERRORES
		if (__get_inode(&_dimap, _crentry.inoden, &_dinode, _pimap)==-1) {
			return -1;
		}else if (__get_data_from_inode(&_dinode, _pinode, &_type, &_didata)) {
			return -1;
		} //devuelvo idata y type
		// ^^^ no me tiene que dar el numero de inodo???!?!?!??!

		//TODO: SI ES UN ARCHIVO LO PROXIMO A BUSCAR NO TIENE QUE VOLVER AL WHILE
		//Los archivos no están en el CR!! estan en el imap del directorio al que pertenecen
		if (__is_last_dir(filename)) { //PREGUNTO SI LO QUE QUEDA ES EL ULTIMO
			//LEVANTO LA DATA DE DISCO DEL ULTIMO DIRECTORIO (EL ANTERIOR)
			bool found = false;
			ddata * _ddata = __load_ddata(_didata);
			for (i=0; i<MAX_INODES && !found; i++) { 
				//BUSCO DENTRO DE LOS DATOS DEL DIRECTORIO EL INODO PARA IR AL MAPA
				ddata_entry curr_dde = (_ddata->map)[i];
				if (streq(curr_dde.name, filename)) {
					_crentry.inoden = curr_dde.inoden;
					//ENCONTRE EL INODO: BUSCA EN EL IMAPA QUE YA TENGO.
					for (j=0; j<MAX_INODES; j++) {
						imap_entry curr_impe = (_pimap->map)[j];
						if (curr_impe.inoden == _crentry.inoden) {
							//LO ENCONTREEEEEEEEEEEE
							//SUBO LA DATA Y EL INODO
							lastimap = _pimap; //GUARDO EL imapa 
							__get_inode(_dimap, _dinode, _crentry.inoden, _pimap);
							__get_data_from_inode(_dinode, _pinode, _type, _didata);
							lastpinode = _pinode;
							_pidata = __load_fdata(_didata);
							return 0;
						}
					}
					found = true; //ERA UN ARCHIVO!!! :)
				}
			}
			if (!found) {
				//ERA UN DIRECTORIO Y NO UN ARCHIVO, LO BUSCO DESDE EL CR NORMALMENTE (SIGO EN EL WHILE SIN HACER NADA)
			}
		}
	}
	lastpimap = _pimap;
	lastpinode = _pinode;
	_pidata = __load_ddata(_didata); //SI HUBIERA SIDO UN FILE ENTRABA EN EL CASO ANTES
	return 0;
}

int __get_data_from_inode(dinode * _dinode, inode * actualinode, ftype * _type, void ** _didata) {
	*actualinode = *__load_inode(*_dinode);
	*_type = actualinode.type;
	*_didata = actualinode.idata;
	return 0;
}

//Having the inode number and the piece of the imap, searchs for the inode
//If there is an error, it returns -1.
int __get_inode(dimap * dimptr, int inoden, dinode * retdinode, imap * retimap) {
	int i;
	imap * pimap = __load_imap(*dimptr);
	imap_entry * curr;

	for (i=0; i<MAX_INODES; i++) {
		curr = &(pimap->map[i]);
		if (curr->inoden == inoden) {
			//No sube el inode a disco porque todavía no lo uso
			*retdinode = curr->inode;
			*retimap = *pimap;
			return curr->inoden;
		}
	}
	return -1;
}

//TODO: . y .. !!
// Busca el primer imap desde el CR (sea file o sea directory. Arreglate vos)
// La idea sería obtener el imap del primer directorio para ir mapeando desde ahí
int __get_cr_entry(char * filename, cr_entry * crep) {
	char dir[MAX_FILENAME];
	int i, read = __get_fst_dirname(filename, dir);
	/*if (streq(filename, "/")) {
		//peola
	} else if (streq(filename,".")
		|| streq(filename, "..")) {
		//We have to search the CR with pwd or the one before the pwd
	} else {
		//caso Tere/Downloads
		//agregar el pwd
	}*/
	cr_entry * map = __cp->map;
	for (i=0; i<=MAX_IMAP && !__is_null(map[i].map);i++) {
		if (streq(map[i].dir_name, dir)) {
			*crep = map[i];
			return read;
		}
	}
	return read;
}

bool __is_inode_dir(inode * _inode) {
	if (_inode->type)==FS_DIR) {
		return true; 
	} return false;
}

bool __is_inode_file(inode * _inode) {
	if (_inode->type)==FS_FILE) {
		return true; 
	} return false;
}


bool __is_last_dir(char * dir) {
	if (strlen(dir) == 0) {
		return false;
	}
	int i;
	while (dir[i]!='\0') {
		if (dir[i]=='/') { 
			return false;
		}
		i++;
	}
	return true;
}

// Gets the first director copying everything before /
// Returns number of chars read
int __get_fst_dirname(char * filename, char * dir) {
	int i=1;
	while (filename[i-1] != '\0' && filename[i-1] != '/') {
		i++;
	}
	memcpy(dir, filename, i);
	dir[i]='\0';
	return i;
}