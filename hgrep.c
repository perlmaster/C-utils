/*********************************************************************
*
* File      : hgrep.c
*
* Author    : Barry Kimelman
*
* Created   : December 6, 2001
*
* Purpose   : Search file for patterns (ala fgrep) and highlight all
*             matching text.
*
*********************************************************************/

#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<string.h>
#include	<regex.h>
#include	<errno.h>
#include	<stdarg.h>

#define		DEFAULT_BUFFER_SIZE		32768	/* default buffer size is 32Kb */
#define	MAX_DATA_PATTERNS	2048

static	int		buffer_size = 0;
static	int		num_files = 0;
static	char	*record_buffer = NULL , *temp_buffer = NULL;
regex_t	re_patterns[MAX_DATA_PATTERNS] , exclude_expr;
int		num_data_patterns = 0;
int		pattern_search_flags = 0;

int search_file();

static	int	opt_n = 0, opt_i = 0 , opt_d = 0 , opt_f = 0 , opt_B = 0;
static	int	opt_e = 0 , opt_l = 0;

extern	void	system_error() , die() , quit() , standout_print();
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
* Function  : display_text
*
* Purpose   : Display a line of text containing a match. The occurrences
*             of the macthes will be highlited.
*
* Inputs    : char *pattern - string defining regular expression
*
* Output    : (none)
*
* Returns   : pointer to compiled regular expression
*
* Example   : regexp = display_text("data[0-9]");
*
* Notes     : (none)
*
*********************************************************************/

void display_text(regmatch_t *pmatch, char *ptr1)
{
	int		errcode , length , count;
	char	temp_buffer[1024];

	errcode = 0;
	while ( errcode == 0 ) {
	/* First print the "chunk" extending from ptr1 to the byte
	   just before the 1st matched byte. Must check to see if
	   the match started at the beginning of the current buffer
	   pointer.
	*/
		if ( pmatch[0].rm_so > 0 ) {
			length = pmatch[0].rm_so;
			strncpy(temp_buffer,ptr1,length);
			temp_buffer[length] = '\0';
			printf("%s",temp_buffer);
		} /* IF */

		/* Highlite the matching string */
		length = pmatch[0].rm_eo - pmatch[0].rm_so;
		strncpy(temp_buffer,&ptr1[pmatch[0].rm_so],length);
		temp_buffer[length] = '\0';
		standout_print("%s",temp_buffer);

		/* Search for the next match in the current record */
		ptr1 = &ptr1[pmatch[0].rm_eo];
/*****	errcode = regexec(pattern, ptr1, (size_t)1, pmatch, 0); *****/
		errcode = REG_NOMATCH;
		for ( count = 0 ; errcode != 0 && count < num_data_patterns ; ++count ) {
			errcode = regexec(&re_patterns[count], ptr1, (size_t)1, pmatch, 0);
		} /* FOR */
	} /* WHILE loop finding matches in record */
	/* Print remaining unmatched portion of record */
	printf("%s\n",ptr1);
} /* end of display_text */

/*********************************************************************
*
* Function  : compile_data_pattern
*
* Purpose   : Compile a regular expression used to match input
*             file records.
*
* Inputs    : char *pattern - string defining regular expression
*
* Output    : (none)
*
* Returns   : pointer to compiled regular expression
*
* Example   : regexp = compile_data_pattern("data[0-9]");
*
* Notes     : (none)
*
*********************************************************************/

regex_t *compile_data_pattern(char *pattern)
{
	int		errcode;
	char	errmsg[256];
	regex_t	*expression;

	if ( num_data_patterns >= MAX_DATA_PATTERNS ) {
		die(1,"Data patterns limit of %d exceeded.\n",MAX_DATA_PATTERNS);
	} /* IF */

	expression = &re_patterns[num_data_patterns++];
	errcode = regcomp(expression, pattern, pattern_search_flags|REG_EXTENDED);
	if ( errcode != 0 ) {
		regerror(errcode,expression,errmsg,sizeof(errmsg));
		die(1,"Bad data pattern : %s\n",errmsg);
	} /* IF */
	return(expression);
} /* end of compile_data_pattern */

/*********************************************************************
*
* Function  : compile_list_of_data_patterns
*
* Purpose   : Compile all the data patterns in the named file.
*
* Inputs    : char *filename - file containing patterns
*
* Output    : (none)
*
* Returns   : number of patterns
*
* Example   : count = compile_list_of_data_patterns("patterns.txt");
*
* Notes     : (none)
*
*********************************************************************/

int compile_list_of_data_patterns(char *filename)
{
	char	recbuff[4096];
	FILE	*input;
	int	num_patterns;

	input = fopen(filename,"r");
	if ( input == NULL ) {
		quit(1,"Can't open patterns file \"%s\"",filename);
	} /* IF */
	num_patterns = 0;
	while ( fgets(recbuff,sizeof(recbuff),input) != NULL ) {
		recbuff[strlen(recbuff)-1] = '\0'; /* trim trailing NL */
		compile_data_pattern(recbuff);
		num_patterns += 1;
	} /* WHILE */
	fclose(input);
	if ( num_patterns < 1 ) {
		die(1,"No valid data patterns in file \"%s\"\n",filename);
	} /* IF */
	return(num_patterns);
} /* end of compile_list_of_data_patterns */

/*********************************************************************
*
* Function  : search_file
*
* Purpose   : Search the specified file for the specified pattern.
*
* Inputs    : FILE *input - file stream pointer of openned input file
*             regex_t *pattern - the compiled regular expression
*             char *filename - name of input file
*
* Output    : (none)
*
* Returns   : number of matches
*
* Example   : search_file(input_fp,regexp,filename);
*
* Notes     : If an I/O error occurs then program execution will be
*             terminated with an appropriate error message.
*
*********************************************************************/

int search_file(FILE *input, regex_t *pattern, char *filename)
{
	char	*ptr1;
	int		num_matches , num_records , length , errcode , count;
	regmatch_t	pmatch[2];

	num_matches = num_records = 0;
	while ( fgets(record_buffer , buffer_size , input) != NULL ) {
		num_records += 1;
		record_buffer[strlen(record_buffer)-1] = '\0'; /* kill trailing newline */
		ptr1 = record_buffer;
		errcode = REG_NOMATCH;
		for ( count = 0 ; errcode != 0 && count < num_data_patterns ; ++count ) {
			errcode = regexec(&re_patterns[count], record_buffer, (size_t)1, pmatch, 0);
		} /* FOR */
		if ( errcode == 0 ) {
			if ( opt_e && regexec(&exclude_expr, record_buffer, 0, NULL, 0) == 0 ) {
				continue;	/* Ignore this record if exclude pattern detected */
			} /* IF */
			num_matches += 1;
			if ( num_matches == 1 ) {
				printf("\n");
			} /* IF */
			if ( opt_f || num_files > 1 ) {
				printf("%s:",filename);
			} /* IF */
			if ( opt_n )
				printf("%5d:\t",num_records);
			if ( opt_l ) {
				standout_print("%s\n",record_buffer);
			} /* IF */
			else {
				display_text(pmatch,ptr1);
			} /* ELSE */
			if ( opt_B ) {
				printf("\n");
			} /* IF */
		} /* IF */
	} /* WHILE loop over all records in file */
	return(num_matches);
} /* End of search_file */

/*********************************************************************
*
* Function  : main
*
* Purpose   : program entry point
*
* Inputs    : char *argv[] - optional flags and directory name
*             int argc - arguments counter
*
* Output    : (none)
*
* Returns   : If 0 then success  Else error
*
* Example   : hgrep -d 'foo[0-9]' *.c
*
* Notes     : (none)
*
*********************************************************************/

int main(int argc, char *argv[])
{
	int		num_matches , errcode , errflg , c , flags;
	char	*filename;
	char	*pattern , errmsg[256];
	regex_t	expression;
	FILE	*input;

	errflg = 0;
	pattern_search_flags = 0;
	while ((c = getopt(argc, argv, ":ndBlfib:F:p:e:")) != -1) {
		switch (c) {
		case 'd':	/* activate debug mode */
			opt_d = 1;
			break;
		case 'l':	/* highlight entire line */
			opt_l = 1;
			break;
		case 'n':	/* activate line numbering */
			opt_n = 1;
			break;
		case 'i':	/* activate case insensitive searching */
			pattern_search_flags = REG_ICASE;
			opt_i = 1;
			break;
		case 'f':	/* always print filename */
			opt_f = 1;
			break;
		case 'B':	/* print an extra blank line after matches */
			opt_B = 1;
			break;
		case 'b':	/* set buffer size */
			buffer_size = atoi(optarg);
			break;
		case 'F':	/* get patterns from file */
			compile_list_of_data_patterns(optarg);
			break;
		case 'p':	/* compile a pattern */
			compile_data_pattern(optarg);
			break;
		case 'e':	/* compile an exclude pattern */
			opt_e = 1;
			errcode = regcomp(&exclude_expr, optarg, pattern_search_flags|REG_EXTENDED);
			if ( errcode != 0 ) {
				regerror(errcode,&exclude_expr,errmsg,sizeof(errmsg));
				die(1,"Bad exclude pattern : %s\n",errmsg);
			} /* IF */
			break;
		case ':':           /* missing option value */
			fprintf(stderr,"Option -%c requires an operand\n", optopt);
						errflg++;
			break;
		case '?':
			fprintf(stderr,"Unrecognized option: -%c\n", optopt);
			errflg++;
		} /* SWITCH */
	} /* WHILE loop over optional parameters */
	if ( errflg || optind >= argc ) {
		die(1,"Usage : %s [-dfBnil] [-b buffsize] [-F patternfile] [-e exclude_pattern] [-p pattern] pattern [... filename]\n",
					argv[0]);
	} /* IF parameter error */

	pattern = argv[optind++];
	compile_data_pattern(pattern);

	if ( buffer_size < DEFAULT_BUFFER_SIZE ) {
		buffer_size = DEFAULT_BUFFER_SIZE;
	} /* IF */
	record_buffer = malloc(buffer_size);
	if ( record_buffer == NULL ) {
		quit(1,"Can't allocate %d bytes for record buffer",buffer_size);
	} /* IF */
	temp_buffer = malloc(buffer_size);
	if ( temp_buffer == NULL ) {
		system_error("Can't allocate %d bytes for temp buffer",buffer_size);
		exit(1);
	} /* IF */

	init_termcap(stderr);
	num_matches = 0;
	if ( optind < argc ) {
		num_files = argc - optind;
		filename = argv[optind];
		for ( ; optind < argc ; filename = argv[++optind] ) {
			input = fopen(filename,"r");
			if ( input == NULL ) {
				system_error("Can't open file \"%s\"",filename);
				continue;
			} /* IF */
			num_matches += search_file(input,&expression,filename);
			fclose(input);
		} /* FOR */
	} /* IF */
	else {
		num_matches += search_file(stdin,&expression,"--stdin--");
	} /* ELSE */
	if ( num_matches <= 0 )
		printf("No matches found to \"%s\".\n",pattern);

	exit(0);
} /* End of main */
