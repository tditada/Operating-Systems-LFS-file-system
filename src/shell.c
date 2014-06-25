#include "kernel.h"
#include "apps.h"
#include "disk.h"
#include "fs.h"

#define BUFSIZE 200
#define NARGS 20
#define PROMPT "SOS> "


static int cd(int argc, char *argv[]);
static int cat(int argc, char *argv[]);
static int ls(int argc, char *argv[]);
static int mkdir(int argc, char *argv[]);
static int touch(int argc, char *argv[]);
static int format(int argc, char *argv[]);
static int inoden(int argc, char*argv[]);
static int print_imap(int argc, char *argv[]);
static int print_log(int argc, char *argv[]);


/*
#ifndef FS_VARS
#define FS_VARS*/
checkpoint * __cp;
char __log_buf[FS_BUFFER_SIZE];
int __log_buf_size = 0;
int __log_buf_count = 0;
lnode * __log_buf_list[FS_BUFFER_SIZE/sizeof(lnode)];
char * pwd="/";
/*#endif
*/
static struct cmdentry{
	char *name;
	int (*func)(int argc, char **argv);
}
cmdtab[] = {
	{	"setkb",		setkb_main },
	{	"shell",		shell_main },
	{	"sfilo",		simple_phil_main },
	{	"filo",			phil_main },
	{	"xfilo",		extra_phil_main },
	{	"afilo",		atomic_phil_main },
	{	"camino",		camino_main },
	{	"camino_ns",	camino_ns_main },
	{	"prodcons",		prodcons_main },
	{	"divz",			divz_main },
	//FS_COMMANDS
	{	"format",		format},
	{ 	"init",			fs_init},
	{	"sync",			fs_sync},
	{	"sync_cr",		fs_sync_cr},
	{	"sync_lbuf",	fs_sync_lbuf},
	{	"cd",			cd},
	{	"cat",			cat},
	{	"ls",			ls},
	{	"mkdir",		mkdir},
	{	"touch",		touch},
	{	"fsdata",		fs_data},
	{	"print_cr",		fs_print_cr},
	{	"print_lbuf",	fs_print_lbuf},
	{	"inoden",		inoden},
	{	"print_imap",	print_imap},
	{	"print_log",	print_log},
	{	"testfs",		testfs }
};

int
shell_main(int argc, char **argv) {
	char buf[BUFSIZE];
	char *args[NARGS+1];
	unsigned nargs;
	struct cmdentry *cp;
	unsigned fg, bg;

/*	fs_init();*/
	mt_cons_getattr(&fg, &bg);
	while (true) {
		mt_cons_setattr(LIGHTGRAY, BLACK);
		cprintk(LIGHTCYAN, BLACK, PROMPT);

		/* leer linea de comando, fraccionarla en tokens y armar argv */
		mt_getline(buf, sizeof buf);
		nargs = separate(buf, args, NARGS);
		if (!nargs)
			continue;
		args[nargs] = NULL;

		/* comandos internos */
		if (strcmp(args[0], "help") == 0 ) {
			printk("Comandos internos:\n");
			printk("\thelp\n");
			printk("\texit\n");
			printk("\treboot\n");
			printk("Aplicaciones:\n");\
			for (cp = cmdtab; cp->name; cp++) {
				printk("\t%s\n", cp->name);
			}
			continue;
		}

		if (strcmp(args[0], "exit") == 0) {
			mt_cons_setattr(fg, bg);
			return nargs > 1 ? atoi(args[1]) : 0;
		}

		if (strcmp(args[0], "reboot") == 0) {
			*(short *) 0x472 = 0x1234;
			while (true){
				outb(0x64, 0xFE);
			}
		}

		/* aplicaciones */
		bool found = false;
		for (cp = cmdtab; cp->name; cp++) {
			if (strcmp(args[0], cp->name) == 0) {
				found = true;
				int n = cp->func(nargs, args);
				if (n != 0 )
					cprintk(LIGHTRED, BLACK, "Status: %d\n", n);
				break;
			}
		}

		if (!found ){
			cprintk(LIGHTRED, BLACK, "Comando %s desconocido\n", args[0]);
		}
	}
}

int cd(int argc, char *argv[]){
	char * dir;
	if(argc == 1){
		dir = "/";
	} else {
		dir = argv[1];
	}
	if(file_existence(dir)){
		if(dir[0]=='/'){
			//Direccion absoluta
			printk("pwd: '%s'->", pwd);
			pwd=dir;
			printk("'%s'\n", dir);
		}else{
			printk("pwd: '%s'->", pwd);
			char newdir[MAX_PATH];
			strcat(newdir, pwd);
			strcat(newdir, "/");
			strcat(newdir, dir);
			strcpy(pwd, newdir);
			printk("'%s'\n", dir);
		}
		return 0;
	} else{
		return -1;
	}
}

int format(int argc, char *argv[]){
	fs_creat(1<<16);
	return 0;
}

int cat(int argc, char *argv[]){
	return fs_cat(argv[1]);
}

int ls(int argc, char *argv[]){
	if (argc == 1) {
		printk("pwd: %s\n", pwd);
		return fs_list(pwd);
	}
	return fs_list(argv[1]);
}

int mkdir(int argc, char *argv[]){
	return fs_mkfile(argv[1], FS_DIR, NULL, 0); 
}

//Parametros para el touch: nombre de archivo, texto
int touch(int argc, char *argv[]){
	printk("arg: %s\n", argv[2]);
	return fs_mkfile(argv[1], FS_FILE, argv[2], strlen(argv[2])+1);
}

int inoden(int argc, char*argv[]) {
	printk("arg: %s\n", argv[1]);
	return fs_print_inoden(argv[1]);
}

int print_imap(int argc, char *argv[]){
	printk("arg: %s\n", argv[1]);
	return fs_print_imap(argv[1]);
}

int print_log(int argc, char *argv[]) {
	printk("arg: %s\n", strlen(argv[1]));
	return fs_print_log(strlen(argv[1]));
}