/*********************************************************************
*
* File      : readtar.c
*
* Author    : Barry Kimelman
*
* Created   : February 21, 2008
*
* Purpose   : Search the contents of a Tar archive file member for a pattern.
*
*********************************************************************/

#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<stdarg.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<time.h>
#include	<regex.h>
#include	<errno.h>
#include	<malloc.h>
#include	<pwd.h>
#include	<grp.h>
#include	"mytar.h"

#define	EQ(s1,s2)	(strcmp(s1,s2)==0)

typedef	struct member_tag {
	long		member_offset;
	char		*member_name;
	int			num_blocks;
	long		member_size;
	struct stat	member_stats;
	struct member_tag	*next;
} MEMBER;

static	char	*progname = NULL;
static	int		num_args , opt_d = 0;
static	union tar_header header1;
static	struct stat	member_stats;

extern	void	die() , quit();

/*********************************************************************
*
* Function  : debug_print
*
* Purpose   : Display an optiona debugging message.
*
* Inputs    : char *format - the format string (ala printf)
*             ... - the data values for the format string
*
* Output    : the debugging message
*
* Returns   : nothing
*
* Example   : debug_print("The answer is %s\n",answer);
*
* Notes     : (none)
*
*********************************************************************/

void debug_print(char *format,...)
{
	va_list ap;

	if ( opt_d ) {
		va_start(ap,format);
		vfprintf(stdout, format, ap);
		fflush(stdout);
		va_end(ap);
	} /* IF debug mode is on */

	return;
} /* end of debug_print */

/*********************************************************************
*
* Function  : usage
*
* Purpose   : Display program usage message.
*
* Inputs    : (none)
*
* Output    : Program usage message.
*
* Returns   : (nothing)
*
* Example   : usage();
*
* Notes     : (none)
*
*********************************************************************/

void usage()
{
	fprintf(stderr,"Usage : %s [-d] member_name string [tar_file]\n",progname);

	return;
} /* end of usage */

/*********************************************************************
*
* Function  : process_archive
*
* Purpose   : Display the contents of the specified archive member.
*
* Inputs    : char *member_name - name of archive member
*             char *string - search argument
*             int tar_fd - file descriptor of open tar archive file
*             char *archive_name - name of tar archive file
*
* Output    : contents of the specified archive member
*
* Returns   : (nothing)
*
* Example   : process_archive(member_name,string,0,"--");
*
* Notes     : (none)
*
*********************************************************************/

int process_archive(char *archive_member_name, char *string, int input_fd, char *archive_name)
{
	int		num_bytes , mode , num_blocks , count , found;
	char	member_name[10+sizeof(header1.tar_buf.t_name)] , buffer[TBLOCK_SIZE];
	MEMBER	*member;
	unsigned char	*member_buffer , *recptr1 , *recptr2;

	found = 0;
	while ( 1 ) {
		num_bytes = read(input_fd,&header1,TBLOCK_SIZE);
		if ( num_bytes == 0 )
			break;
		if ( num_bytes != TBLOCK_SIZE )
			quit(1,"Can't read header block from archive");

		memset(member_name, 0, sizeof(member_name));
		memcpy(member_name, header1.tar_buf.t_name, sizeof(header1.tar_buf.t_name));
		if ( member_name[0] == '\0' )
			break;
		member = (MEMBER *)calloc(1,sizeof(MEMBER));
		if ( member == NULL )
			quit(1,"calloc failed");
		member->member_name = strdup(member_name);
		if ( member->member_name == NULL )
			quit(1,"strdup failed");

		memset(&member_stats, 0, sizeof(member_stats));
		member_stats.st_uid = -1;
		member_stats.st_gid = -1;
		member_stats.st_size = -1;
		sscanf(header1.tar_buf.t_uid,"%o",&member_stats.st_uid);
		sscanf(header1.tar_buf.t_gid,"%o",&member_stats.st_gid);
		sscanf(header1.tar_buf.t_size,"%o",&member_stats.st_size);
		sscanf(header1.tar_buf.t_mtime,"%o",&member_stats.st_mtime);
		sscanf(header1.tar_buf.t_mode,"%o",&mode);
		member_stats.st_mode = mode;
		memcpy(&member->member_stats, &member_stats, sizeof(member_stats));

		num_blocks = (member_stats.st_size + TBLOCK_SIZE - 1) / TBLOCK_SIZE;
		member->num_blocks = num_blocks;
		member->member_size = num_blocks * TBLOCK_SIZE;

		debug_print("file in archive is '%s' (%c) [%s] [0%o] , num_blocks = %d\n",member_name,
						header1.tar_buf.t_typeflag,header1.tar_buf.t_magic,mode,member->num_blocks);
		if ( EQ(member_name,archive_member_name) ) {
			found = 1;
			member_buffer = (unsigned char *)calloc(1,member_stats.st_size);
			if ( member_buffer == NULL )
				quit(1,"calloc failed for %d bytes",member_stats.st_size);
			num_bytes = read(input_fd,member_buffer,member_stats.st_size);
			if ( num_bytes < member_stats.st_size )
				quit(1,"Can't read %d bytes from archive",member_stats.st_size);
			recptr1 = member_buffer;
			count = 0;
			while ( recptr1 != NULL ) {
				recptr2 = (unsigned char *)strchr((char *)recptr1,'\n');
				if ( recptr2 != NULL )
					*recptr2 = '\0';
				if ( strstr((char *)recptr1,string) != NULL ) {
					if ( ++count == 1 )
						printf("*** %s [%s] ***\n",archive_name,archive_member_name);
					printf("%s\n",recptr1);
				} /* IF string found in archive member */
				if ( recptr2 != NULL )
					recptr1 = recptr2 + 1;
				else
					recptr1 = NULL;
			} /* WHILE over records from archive member */
			break;  /* get out of big loop over archive members */
		} /* IF */

		for ( count = 0 ; count < member->num_blocks ; ++count ) {
			num_bytes = read(input_fd,buffer,TBLOCK_SIZE);
			if ( num_bytes != TBLOCK_SIZE )
				quit(1,"Can't read data block from archive");
		} /* FOR */

	} /* WHILE more data to be processed */

	return(found);
} /* end of process_archive */

/*********************************************************************
*
* Function  : main
*
* Purpose   : Search contents of Tar archive file members.
*
* Inputs    : int argc - number of arguments
*             char *argv[] - list of arguments
*
* Output    : Archive listing
*
* Returns   : 0 --> success , 1 --> error
*
* Example   : greptar member_name
*
* Notes     : (none)
*
*********************************************************************/

int main(int argc, char *argv[])
{
	int		status , errors , c , tar_fd;
	char	*argptr , *member_name , *string , *filename;

	progname = argv[0];
	errors = 0;
	while ( (c = getopt(argc,argv,":d")) != EOF ) {
		switch ( c ) {
		case 'd':
			opt_d = 1;
			break;
		case '?':
			errors = 1;
			break;
		case ':':
			printf("Missing value for option '%c'\n",optopt);
			errors += 1;
			break;
		} /* SWITCH */
	} /* WHILE over optional args */
	num_args = argc - optind;
	if ( errors || num_args < 2 ) {
		usage();
		exit(1);
	} /* IF */

	member_name = argv[optind++];
	string = argv[optind++];
	if ( optind >= argc ) {
		status = process_archive(member_name,string,0,"--");
	} /* IF */
	else {
		filename = argv[optind];
		tar_fd = open(filename,O_RDONLY);
		if ( tar_fd < 0 )
			quit(1,"open failed for \"%s\"",filename);
		status = process_archive(member_name,string,tar_fd,filename);
		close(tar_fd);
	} /* ELSE */

	if ( !status ) {
		printf("'%s' is not a member of the archive\n",member_name);
	} /* IF */

	exit(0);
} /* end of main */
