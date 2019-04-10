/*********************************************************************
*
* File      : lc.c
*
* Author    : Barry Kimelman
*
* Created   : August 29, 2001
*
* Purpose   : List files in a directcory.
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
#include	<regex.h>

#define	LE(s1,s2)	(strcasecmp(s1,s2)<=0)
#define	GT(s1,s2)	(strcasecmp(s1,s2)>0)
#define	GE(s1,s2)	(strcasecmp(s1,s2)>=0)
#define	EQ(s1,s2)	(strcmp(s1,s2)==0)

typedef	struct namestag {
	char	*name;
	struct namestag	*next_name;
} NAMES;

typedef	struct fileclass {
	char	*class_title;
	int	num_entries;
	int	longest_name;
	NAMES	*first_name , *last_name;
} FILECLASS;

FILECLASS regular_class = { "Regular Files" , 0 , 0 , NULL , NULL };
FILECLASS dir_class = { "Directories" , 0 , 0 , NULL , NULL };
FILECLASS char_class = { "Character Special" , 0 , 0 , NULL , NULL };
FILECLASS block_class = { "Block Special" , 0 , 0 , NULL , NULL };
FILECLASS pipe_class = { "Pipes" , 0 , 0 , NULL , NULL };
FILECLASS symlink_class = { "Symbolic Links" , 0 , 0 , NULL , NULL };
FILECLASS socket_class = { "Sockets" , 0 , 0 , NULL , NULL };
FILECLASS misc_class = { "Miscellaneous" , 0 , 0 , NULL , NULL };

int	columns = 0 , opt_d = 0 , opt_f = 0 , opt_b = 0 , opt_x = 0;
int	opt_c = 0 , opt_p = 0 , opt_a = 0 , opt_l = 0 , opt_s = 0;
int	opt_o = 0 , opt_u = 0 , opt_A = 0 , opt_F = 0 , opt_D = 0;
int	opt_P = 0 , opt_U = 0 , opt_v = 0;

char	*dir_path = NULL;
regex_t	uppercase_regexp;
regex_t	pattern_regexp;
char	*progname;

uid_t	my_uid;
gid_t	my_gid;

extern	int	optind , optopt , opterr , tty_num_rows , tty_num_cols;
extern	char	*optarg , *__loc1;

extern	void	system_error() , standout_print() , die();
extern	int		init_termcap();

/*********************************************************************
*
* Function  : debug_print
*
* Purpose   : Optionally print a debugging message.
*
* Inputs    : va_alist - arguments comprising message
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : debug_print("Process the file %s\n",filename);
*
* Notes     : (none)
*
*********************************************************************/

void debug_print(char *format,...)
{
	va_list ap;

	if ( opt_v ) {
		va_start(ap,format);
		vfprintf(stdout, format, ap);
		fflush(stdout);
		va_end(ap);
	} /* IF */

	return;
} /* end of debug_print */

/*********************************************************************
*
* Function  : add_to_class
*
* Purpose   : Add the specified unqualified filename to the specified
*             class.
*
* Inputs    : class_ptr - pointer to class structure
*             name - name of file to be added to class structure
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

void add_to_class(FILECLASS *class_ptr, char *name)
{
	NAMES	*node , *curr_node , *prev_node;
	int	namelen;
	regmatch_t	pmatch[2];

	if ( name[0] == '.' && !(opt_a || opt_A) ) {
		return;
	}
	if ( ( EQ(name,".") || EQ(name,"..") ) && opt_A ) {
		return;
	} /* IF */

	if ( opt_u &&
			regexec(&uppercase_regexp, name, (size_t)1, pmatch, 0) != 0 ) {
		return;
	} /* IF */

	if ( opt_P &&
			regexec(&pattern_regexp, name, (size_t)1, pmatch, 0) != 0 ) {
		return;
	} /* IF */

	if ( opt_F ) {
		class_ptr->num_entries += 1;
		return;
	} /* IF */

	debug_print("add_to_class(%s)\n",name);
	node = (NAMES *)malloc(sizeof(NAMES));
	if ( node == NULL ) {
		quit(1,"malloc for NAMES failed");
	}
	node->name = strdup(name);
	if ( node->name == NULL ) {
		quit(1,"malloc failed for filename");
	}
	namelen = strlen(node->name);
	if ( namelen > class_ptr->longest_name ) {
		class_ptr->longest_name = namelen;
	}
	node->next_name = NULL;
	if ( class_ptr->num_entries == 0 ) {
		class_ptr->first_name = class_ptr->last_name = node;
	}
	else {
		if ( LE(name,class_ptr->first_name->name) ) {
			node->next_name = class_ptr->first_name;
			class_ptr->first_name = node;
		}
		else {
			if ( GE(name,class_ptr->last_name->name) ) {
				class_ptr->last_name->next_name = node;
				class_ptr->last_name = node;
			}
			else {
				prev_node = NULL;
				curr_node = class_ptr->first_name;
				while ( GT(name,curr_node->name) ) {
					prev_node = curr_node;
					curr_node = curr_node->next_name;
				}
				node->next_name = curr_node;
				prev_node->next_name = node;
			}
		}
	}
	class_ptr->num_entries += 1;

	return;
} /* end of add_to_class */

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
	NAMES	*node;
	int		line_width , width , length;
	char	path[MAXPATHLEN];

	debug_print("dump_class(%s) count = %d\n",class_ptr->class_title,
					class_ptr->num_entries);
	if ( class_ptr->num_entries <= 0 ) {
		return;
	}
	line_width = 0;
	printf("\n");
	debug_print("dump_class() : call standout_print()\n");
	standout_print("%s [%d]",class_ptr->class_title,
				class_ptr->num_entries);
	printf("\n");
	fflush(stdout);
	debug_print("dump_class() : after standout_print()\n");
	if ( opt_F ) {
		return;
	} /* IF */
	width = class_ptr->longest_name + 1;
	node = class_ptr->first_name;
	for ( ; node != NULL ; node = node->next_name ) {
		if ( line_width+width > columns ) {
			printf("\n");
			line_width = 0;
		}
		if ( opt_x ) {
			sprintf(path,"%s/%s",dir_path,node->name);
			if ( access(path,X_OK) == 0 ) {
				standout_print("%s",node->name);
				length = width - strlen(node->name);
				printf("%-*.*s",length,length," ");
			} /* IF */
			else {
				printf("%-*.*s",width,width,node->name);
			} /* ELSE */
		} /* IF */
		else {
			printf("%-*.*s",width,width,node->name);
		} /* ELSE */
		line_width += width;
	} /* FOR */
	printf("\n");

	return;
} /* end of dump_class */

/*********************************************************************
*
* Function  : extract_name
*
* Purpose   : Extract a name from the list of names.
*
* Inputs    : names - list of names
*             num_names - number of names in list
*             row - row position
*             col - column position
*
* Output    : (none)
*
* Returns   : pointer to located name
*
* Example   : name = extract_name(names,num_entries,needed_rows,row,col);
*
* Notes     : (none)
*
*********************************************************************/

char *extract_name(char **names, int num_names, int num_rows,int row,int col)
{
	int		position , count;
	char	*name;

	position = (col * num_rows) + row;
	name = (position < num_names) ? names[position] : NULL;

	return(name);
} /* end of extract_name */

/*********************************************************************
*
* Function  : dump_class2
*
* Purpose   : Display the contents of the specified class structure.
*
* Inputs    : class_ptr - pointer to class structure
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : dump_class2(&dir_class);
*
* Notes     : (none)
*
*********************************************************************/

void dump_class2(FILECLASS *class_ptr)
{
	NAMES	*node , *nameslist;
	int		line_width , width , length , cols_per_row , needed_rows;
	int		row , col , num_entries , count;
	char	path[MAXPATHLEN] , *name , **names;

	num_entries = class_ptr->num_entries;
	if ( num_entries <= 0 ) {
		return;
	}
	printf("\n");
	standout_print("%s [%d]",class_ptr->class_title,num_entries);
	printf("\n");
	fflush(stdout);
	if ( opt_F ) {
		return;
	} /* IF */
	nameslist = class_ptr->first_name;
	names = (char **)calloc(num_entries,sizeof(char *));
	if ( names == NULL ) {
		quit(1,"calloc failed");
	} /* IF */
	node = nameslist;
	for ( count = 0 ; count < num_entries ; ++count ) {
		names[count] = strdup(node->name);
		if ( names[count] == NULL ) {
			quit(1,"malloc failed");
		} /* IF */
		node = node->next_name;
	} /* FOR */

	line_width = 0;
	width = class_ptr->longest_name + 1;

	cols_per_row = columns / width;
	needed_rows = (class_ptr->num_entries + cols_per_row - 1) / cols_per_row;

	for ( row = 0 ; row < needed_rows ; ++row ) {
		for ( col = 0 ; col < cols_per_row ; ++col ) {
			name = extract_name(names,num_entries,needed_rows,row,col);
			if ( line_width+width > columns ) {
				printf("\n");
				line_width = 0;
			} /* IF */
			line_width += width;
			if ( name == NULL ) {
				printf("%-*.*s",width,width," ");
				continue;
			} /* IF */
			if ( opt_x ) {
				sprintf(path,"%s/%s",dir_path,name);
				if ( access(path,X_OK) == 0 ) {
					standout_print("%s",name);
					length = width - strlen(name);
					printf("%-*.*s",length,length," ");
				} /* IF */
				else {
					printf("%-*.*s",width,width,name);
				} /* ELSE */
			} /* IF */
			else {
				printf("%-*.*s",width,width,name);
			} /* ELSE */
		} /* FOR over columns per row */
	} /* FOR over rows */
	printf("\n");
	free(names);

#ifdef OLD_STUFF
	node = nameslist;
	for ( ; node != NULL ; node = node->next_name ) {
		if ( line_width+width > columns ) {
			printf("\n");
			line_width = 0;
		}
		if ( opt_x ) {
			sprintf(path,"%s/%s",dir_path,node->name);
			if ( access(path,X_OK) == 0 ) {
				standout_print("%s",node->name);
				length = width - strlen(node->name);
				printf("%-*.*s",length,length," ");
			} /* IF */
			else {
				printf("%-*.*s",width,width,node->name);
			} /* ELSE */
		} /* IF */
		else {
			printf("%-*.*s",width,width,node->name);
		} /* ELSE */
		line_width += width;
	} /* FOR */
	printf("\n");
#endif

	return;
} /* end of dump_class2 */

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
	fprintf(stderr,"Usage : %s [-oadfbcpls] [-C num_columns] [dir_path]\n",progname);

	return;
} /* end of usage */

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
	int		errcode;
	char	errmsg[256];

	progname = argv[0];
	init_termcap(stderr);
	anytypes = 0;
	if ( getenv("COLUMNS") != NULL ) {
		columns = atoi(getenv("COLUMNS"));
	} /* IF */
	else {
		columns = tty_num_cols;
	} /* ELSE */

	errflag = 0;
	while ( (opt = getopt(argc,argv,":UFoaAdDfbcplsxuDC:P:v")) != -1 ) {
		switch (opt) {
		case 'o':
			opt_o = 1;
			break;
		case 'v':
			opt_v = 1;
			break;
		case 'U':
			opt_U = 1;
			break;
		case 'C':
			columns = atoi(optarg);
			break;
		case 'P':
			opt_P = 1;
			errcode = regcomp(&pattern_regexp, optarg, REG_EXTENDED);
			if ( errcode != 0 ) {
				regerror(errcode,&pattern_regexp,errmsg,sizeof(errmsg));
				fprintf(stderr,"Bad data pattern : %s\n",errmsg);
				exit(1);
			} /* IF */
			break;
		case 'a':
			if ( opt_A ) {
				fprintf(stderr,"-a and -A are mutually exclusive\n");
				errflag += 1;
			} /* IF */
			opt_a = 1;
			break;
		case 'A':
			if ( opt_a ) {
				fprintf(stderr,"-A and -a are mutually exclusive\n");
				errflag += 1;
			} /* IF */
			opt_A = 1;
			break;
		case 'x':
			opt_x = 1;
			break;
		case 'F':
			opt_F = 1;
			break;
		case 'u':
			opt_u = 1;
			string = "[A-Z]";
			errcode = regcomp(&uppercase_regexp, string, REG_EXTENDED);
			if ( errcode != 0 ) {
				regerror(errcode,&uppercase_regexp,errmsg,sizeof(errmsg));
				fprintf(stderr,"Bad data pattern for uppercase : %s\n",errmsg);
				exit(1);
			} /* IF */
			break;
		case 'd':
			opt_d = 1;
			anytypes = 1;
			break;
		case 'D':
			opt_D = 1;
			anytypes = 1;
			break;
		case 'f':
			opt_f = 1;
			anytypes = 1;
			break;
		case 'b':
			opt_b = 1;
			anytypes = 1;
			break;
		case 'c':
			opt_c = 1;
			anytypes = 1;
			break;
		case 'p':
			opt_p = 1;
			anytypes = 1;
			break;
		case 'l':
			opt_l = 1;
			anytypes = 1;
			break;
		case 's':
			opt_s = 1;
			anytypes = 1;
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
	if ( columns < 40 ) {
		columns = 40;
	} /* IF */
	columns -= 1;
	if ( ! anytypes ) {
		opt_d = opt_f = opt_b = opt_c = opt_p = opt_l = opt_s = 1;
	}

	my_uid = getuid();
	my_gid = getgid();

	dir_path = (optind < argc) ? argv[optind] : ".";
	debug_print("Process directory [%s]\n",dir_path);
	dirptr = opendir(dir_path);
	if ( dirptr == NULL ) {
		quit(1,"opendir failed for \"%s\"",dir_path);
	}

	entry = readdir(dirptr);
	for ( ; entry != NULL ; entry = readdir(dirptr) ) {
		debug_print("Process directory entry [%s]\n",entry->d_name);
		sprintf(filepath,"%s/%s",dir_path,entry->d_name);
		if ( lstat(filepath,&filestats) == 0 ) {
			if ( opt_U && filestats.st_uid != my_uid )
				continue;
			filemode = filestats.st_mode & S_IFMT;
			switch ( filemode ) {
			case S_IFDIR:
				if ( opt_d ) {
					add_to_class(&dir_class,entry->d_name);
				}
				break;
			case S_IFREG:
				if ( opt_f ) {
					add_to_class(&regular_class,entry->d_name);
				}
				break;
			case S_IFBLK:
				if ( opt_b ) {
					add_to_class(&block_class,entry->d_name);
				}
				break;
			case S_IFCHR:
				if ( opt_c ) {
					add_to_class(&char_class,entry->d_name);
				}
				break;
			case S_IFIFO:
				if ( opt_p ) {
					add_to_class(&pipe_class,entry->d_name);
				}
				break;
			case S_IFLNK:
				if ( opt_l ) {
					add_to_class(&symlink_class,entry->d_name);
				}
				break;
			case S_IFSOCK:
				if ( opt_s ) {
					add_to_class(&socket_class,entry->d_name);
				}
				break;
			default:
				fprintf(stderr,"Unexpected mode %o for %s\n",
						filemode,entry->d_name);
				if ( !anytypes ) {
					add_to_class(&misc_class,entry->d_name);
				} /* IF */
			} /* end of SWITCH */
		} /* IF lstat succeeded */
		else {
			system_error("lstat failed for \"%s\"",filepath);
		} /* ELSE lstat failed */
	} /* FOR loop over directory entries */
	debug_print("close directory\n");
	closedir(dirptr);
	debug_print("directory is now closed\n");
	if ( opt_o ) {
		dump_class2(&regular_class);
		dump_class2(&dir_class);
		dump_class2(&char_class);
		dump_class2(&block_class);
		dump_class2(&pipe_class);
		dump_class2(&symlink_class);
		dump_class2(&socket_class);
		dump_class2(&misc_class);
	} /* IF */
	else {
		dump_class(&regular_class);
		dump_class(&dir_class);
		dump_class(&char_class);
		dump_class(&block_class);
		dump_class(&pipe_class);
		dump_class(&symlink_class);
		dump_class(&socket_class);
		dump_class(&misc_class);
	} /* ELSE */

	exit(0);
} /* end of main */
