#include "kernel.h"
#include "apps.h"
#include "disk.h"
#include "fs.h"

#define BUFSIZE 200
#define NARGS 20
#define PROMPT "sOS> "

static char * pwd="/";
int cd(int argc, char *argv[]);

static struct cmdentry
{
	char *name;
	int (*func)(int argc, char **argv);
}
cmdtab[] =
{
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
	{	"testfs",		testfs },
	{	"sync_cr",		sync_cr},
	{	"sync_lbuf",	sync_lbuf},
	{	"cd",			cd}
};

int
shell_main(int argc, char **argv)
{
	char buf[BUFSIZE];
	char *args[NARGS+1];
	unsigned nargs;
	struct cmdentry *cp;
	unsigned fg, bg;

	mt_cons_getattr(&fg, &bg);
	while ( true )
	{
		mt_cons_setattr(LIGHTGRAY, BLACK);
		cprintk(LIGHTCYAN, BLACK, PROMPT);

		/* leer linea de comando, fraccionarla en tokens y armar argv */
		mt_getline(buf, sizeof buf);
		nargs = separate(buf, args, NARGS);
		if ( !nargs )
			continue;
		args[nargs] = NULL;

		/* comandos internos */
		if ( strcmp(args[0], "help") == 0 )
		{
			printk("Comandos internos:\n");
			printk("\thelp\n");
			printk("\texit\n");
			printk("\treboot\n");
			printk("Aplicaciones:\n");\
			for ( cp = cmdtab ; cp->name ; cp++ )
				printk("\t%s\n", cp->name);
			continue;
		}

		if ( strcmp(args[0], "exit") == 0 )
		{
			mt_cons_setattr(fg, bg);
			return nargs > 1 ? atoi(args[1]) : 0;
		}

		if ( strcmp(args[0], "reboot") == 0 )
		{
			*(short *) 0x472 = 0x1234;
			while ( true )
				outb(0x64, 0xFE);
		}

		/* aplicaciones */
		bool found = false;
		for ( cp = cmdtab ; cp->name ; cp++ )
			if ( strcmp(args[0], cp->name) == 0 )
			{
				found = true;
				int n = cp->func(nargs, args);
				if ( n != 0 )
					cprintk(LIGHTRED, BLACK, "Status: %d\n", n);
				break;
			}

		if ( !found )
			cprintk(LIGHTRED, BLACK, "Comando %s desconocido\n", args[0]);
	}
}

int cd(int argc, char *argv[]){
	//La tabla del CR en RAM siempre está actualizada !!!!
	//uso bool __search_cr(char * dir) de fs.c
	//dir la dirección que no sé como pasar.
	char * dir=argv[1];
	// cprintk(LIGHTRED, BLACK, "Param: %s\n", dir);
	if(argc!=2){
		cprintk(LIGHTRED, BLACK, "Necesita solo %d parametros", 1);
		return -1;
	}
	if(file_existence(dir)){
		if(dir[0]=='/'){
			//Direccion absoluta
			pwd=dir;
		}else{
			strcpy(pwd, dir);
		}
		return 0;
	} else{
		return -1;
	}
}