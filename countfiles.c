/*********************************************************************
*
* File      : countfiles.c
*
* Author    : Barry Kimelman
*
* Created   : August 13, 2020
*
* Purpose   : Count file types in a directory tree
*
*********************************************************************/

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<dirent.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<stdarg.h>
#include	<libgen.h>
#include	<strings.h>

#define	LT(s1,s2)	(strcasecmp(s1,s2)<0)
#define	GT(s1,s2)	(strcasecmp(s1,s2)>0)
#define	EQ(s1,s2)	(strcmp(s1,s2)==0)
#define	MAX_DIRS	1024

typedef	struct fileclass {
	char	*class_title;
	int		num_entries;
	int		longest_name;
	char	**classnames;
	int		max_entries;
} FILECLASS;

int		init_names_size = 100 , increment_names_size = 25;

FILECLASS regular_class = { "Regular Files" , 0 , 0 , NULL };
FILECLASS dir_class = { "Directories" , 0 , 0 , NULL };
FILECLASS char_class = { "Character Special" , 0 , 0 , NULL };
FILECLASS block_class = { "Block Special" , 0 , 0 , NULL };
FILECLASS pipe_class = { "Pipes" , 0 , 0 , NULL };
FILECLASS symlink_class = { "Symbolic Links" , 0 , 0 , NULL };
FILECLASS socket_class = { "Sockets" , 0 , 0 , NULL };
FILECLASS misc_class = { "Miscellaneous" , 0 , 0 , NULL };

FILECLASS	*class_list[] = { &regular_class , &dir_class , &char_class ,
			&block_class , &pipe_class , &symlink_class , &socket_class ,
			&misc_class };
int		num_classes = sizeof(class_list) / sizeof(FILECLASS *);

int	opt_d = 0 , opt_h = 0;
char	*progname;

extern	int	optind , optopt , opterr;
extern	void	system_error() , die() , quit();

/*********************************************************************
*
* Function  : usage
*
* Purpose   : Display a brief help summary
*
* Inputs    : (none)
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : usage()
*
* Notes     : (none)
*
*********************************************************************/

void usage()
{
	fprintf(stderr,"Usage : %s [-dh]\n",progname);

	return;
} /* end of usage */

/*********************************************************************
*
* Function  : add_to_class
*
* Purpose   : Add the specified unqualified filename to the specified
*             class.
*
* Inputs    : class_ptr - pointer to class structure
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : add_to_class(&dir_class,filename);
*
* Notes     : (none)
*
*********************************************************************/

void add_to_class(FILECLASS *class_ptr)
{
	int		namelen , new_size;

	class_ptr->num_entries += 1;

	return;
} /* end of add_to_class */

/*********************************************************************
*
* Function  : process_dir
*
* Purpose   : Process a directory
*
* Inputs    : char *dirname - directory name
*
* Output    : appropriate messages
*
* Returns   : (nothing)
*
* Example   : process_dir(".")
*
* Notes     : (none)
*
*********************************************************************/

void process_dir(char *dirname)
{
	DIR	*dirptr;
	char	filepath[MAXPATHLEN] , *string;
	struct dirent	*entry;
	struct stat	filestats;
	mode_t	filemode;
	FILECLASS	*class_ptr;
	char	*subdirs[MAX_DIRS];
	int		num_subdirs = 0;
	int		index;

	dirptr = opendir(dirname);
	if ( dirptr == NULL ) {
		quit(1,"opendir failed for '%s'",dirname);
	}

	entry = readdir(dirptr);
	for ( ; entry != NULL ; entry = readdir(dirptr) ) {
		sprintf(filepath,"%s/%s",dirname,entry->d_name);
		if ( EQ(entry->d_name,".") || EQ(entry->d_name,"..") ) {
			continue;
		} /* skip over '.' and '..' */
		if ( lstat(filepath,&filestats) == 0 ) {
			filemode = filestats.st_mode & S_IFMT;
			switch ( filemode ) {
			case S_IFDIR:
				add_to_class(&dir_class);
				if ( num_subdirs >= MAX_DIRS ) {
					die(1,"Limit of %d subdirs exceeded under '%s'\n",MAX_DIRS,dirname);
				} /* IF */
				subdirs[num_subdirs] = strdup(filepath);
				if ( subdirs[num_subdirs] == NULL ) {
					quit(1,"strdup failed");
				} /* IF */
				num_subdirs += 1;
				break;
			case S_IFREG:
				add_to_class(&regular_class);
				break;
			case S_IFBLK:
				add_to_class(&block_class);
				break;
			case S_IFCHR:
				add_to_class(&char_class);
				break;
			case S_IFIFO:
				add_to_class(&pipe_class);
				break;
			case S_IFLNK:
				add_to_class(&symlink_class);
				break;
			case S_IFSOCK:
				add_to_class(&socket_class);
				break;
			default:
				fprintf(stderr,"Unexpected mode %o for %s\n", filemode,entry->d_name);
				add_to_class(&misc_class);
			} /* end of SWITCH */
		} /* IF lstat succeeded */
		else {
			system_error("lstat failed for '%s'",filepath);
		} /* ELSE lstat failed */
	} /* FOR loop over directory entries */
	closedir(dirptr);
	for ( index = 0 ; index < num_subdirs ; ++index ) {
		process_dir(subdirs[index]);
	}

	return;
} /* end of process_dir */

/*********************************************************************
*
* Function  : dump_class
*
* Purpose   : Display the contents of the specified class structure.
*
* Inputs    : class_ptr - pointer to class structure
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : dump_class(&dir_class);
*
* Notes     : (none)
*
*********************************************************************/

void dump_class(FILECLASS *class_ptr)
{
	printf("%-24.24s [%d]\n",class_ptr->class_title, class_ptr->num_entries);

	return;
} /* end of dump_class */

/*********************************************************************
*
* Function  : main
*
* Purpose   : program entry point
*
* Inputs    : argc - number of parameters
*             argv - list of parameters
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : lc -a ~/test
*
* Notes     : (none)
*
*********************************************************************/

int main(int argc,char *argv[])
{
	DIR	*dirptr;
	char	filepath[MAXPATHLEN] , *string;
	struct dirent	*entry;
	struct stat	filestats;
	mode_t	filemode;
	int	opt , errflag , anytypes;
	int		errcode , count;
	char	errmsg[256];
	FILECLASS	*class_ptr;

	progname = argv[0];
	anytypes = 0;

	errflag = 0;
	while ( (opt = getopt(argc,argv,":dh")) != -1 ) {
		switch (opt) {
		case 'h':
			opt_h = 1;
			break;
		case 'd':
			opt_d = 1;
			break;
		case '?':
			fprintf(stderr,"Unknown option '%c'\n",optopt);
			errflag += 1;
			break;
		case ':':
			fprintf(stderr,"Missing value for option '%c'\n",optopt);
			errflag += 1;
			break;
		default:
			fprintf(stderr,"Unexpected value returned from getopt() = '%c'\n",
					opt);
			errflag += 1;
		} /* SWITCH */
	} /* WHILE loop over options */
	if ( errflag ) {
		usage();
		die(1,"\n%s aborted.\n",argv[0]);
	}

	process_dir(".");
	dump_class(&regular_class);
	dump_class(&dir_class);
	dump_class(&char_class);
	dump_class(&block_class);
	dump_class(&pipe_class);
	dump_class(&symlink_class);
	dump_class(&socket_class);
	dump_class(&misc_class);

	exit(0);
} /* main */
