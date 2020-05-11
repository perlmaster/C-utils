/*********************************************************************
*
* File      : linklist.c
*
* Author    : Barry Kimelman
*
* Created   : May 9, 2020
*
* Purpose   : Demo of linklist handling
*
*********************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>
#include	<time.h>
#include	<string.h>
#include	<stdarg.h>
#include	<getopt.h>
#include	<signal.h>    /* signal name macros, and the signal() prototype */

#define	EQ(s1,s2)	(strcmp(s1,s2)==0)
#define	NE(s1,s2)	(strcmp(s1,s2)!=0)
#define	GT(s1,s2)	(strcmp(s1,s2)>0)
#define	LT(s1,s2)	(strcmp(s1,s2)<0)
#define	LE(s1,s2)	(strcmp(s1,s2)<=0)

typedef	struct node_tag {
	int		value;
	struct node_tag		*next;
} NODE;

typedef	struct list_tag {
	int		num_nodes;
	NODE	*first;
	NODE	*last;
} LINKLIST;

static	int		opt_d = 0 , opt_h = 0;
static	int		num_args;

extern	int		optind , optopt , opterr;

extern	void	system_error() , quit() , die();

/* first, here is the signal handler */
void catch_int(int sig_num)
{
    /* re-set the signal handler again to catch_int, for next time */
    signal(SIGINT, catch_int);
    printf("Don't do that\n");
    fflush(stdout);
} /* end of catch_int */

/* here is another signal handler */
void catch_sig(int sig_num)
{
	 fflush(stderr);
	 fflush(stdout);
    /* re-set the signal handler to default action */
    signal(sig_num, SIG_DFL);
    fprintf(stderr,"Caught signal %d\n",sig_num);
    fflush(stderr);
	 /* abort(); */
	 exit(sig_num);  /* just in case abort() does not work as expected */
} /* end of catch_int */

/*********************************************************************
*
* Function  : debug_print
*
* Purpose   : Display an optional debugging message.
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
* Purpose   : Display a program usage message
*
* Inputs    : char *pgm - name of program
*
* Output    : the usage message
*
* Returns   : nothing
*
* Example   : usage("The answer is %s\n",answer);
*
* Notes     : (none)
*
*********************************************************************/

void usage(char *pgm)
{
	fprintf(stderr,"Usage : %s [-hFgiDdtsr]\n\n",pgm);
	fprintf(stderr,"d - invoke debugging mode\n");
	fprintf(stderr,"h - produce this summary\n");

	return;
} /* end of usage */

/*********************************************************************
*
* Function  : init_list
*
* Purpose   : Initialize a linked list
*
* Inputs    : LINKLIST *list_ptr - pointer to linklink structure
*
* Output    : appropriate error messages
*
* Returns   : IF problem THEN negative ELSE zero
*
* Example   : status = init_list(&linklist);
*
* Notes     : (none)
*
*********************************************************************/

int init_list(LINKLIST *list_ptr)
{
	list_ptr->num_nodes = 0;
	list_ptr->first = NULL;
	list_ptr->last = NULL;

	return 0;
} /* end of init_list */

/*********************************************************************
*
* Function  : append_list
*
* Purpose   : Append a value to the end of the list
*
* Inputs    : LINKLIST *list_ptr - pointer to linklink structure
*             int value - value to be added
*
* Output    : appropriate error messages
*
* Returns   : IF problem THEN negative ELSE zero
*
* Example   : status = append_list(&linklist,99);
*
* Notes     : (none)
*
*********************************************************************/

int append_list(LINKLIST *list_ptr, int value)
{
	NODE	*node;

	node = (NODE *)calloc(1,sizeof(NODE));
	if ( node == NULL ) {
		system_error("Can't allocate new node");
		return -1;
	}
	node->next = NULL;
	list_ptr->num_nodes += 1;
	if ( list_ptr->num_nodes == 1 ) {
		list_ptr->first = node;
	}
	else {
		list_ptr->last->next = node;
	}
	node->value = value;
	list_ptr->last = node;

	return 0;
} /* end of append_list */

/*********************************************************************
*
* Function  : prepend_list
*
* Purpose   : Prepend a value to the end of the list
*
* Inputs    : LINKLIST *list_ptr - pointer to linklink structure
*             int value - value to be added
*
* Output    : appropriate error messages
*
* Returns   : IF problem THEN negative ELSE zero
*
* Example   : status = prepend_list(&linklist,99);
*
* Notes     : (none)
*
*********************************************************************/

int prepend_list(LINKLIST *list_ptr, int value)
{
	NODE	*node;

	node = (NODE *)calloc(1,sizeof(NODE));
	if ( node == NULL ) {
		system_error("Can't allocate new node");
		return -1;
	}
	node->next = NULL;
	list_ptr->num_nodes += 1;
	if ( list_ptr->num_nodes == 1 ) {
		list_ptr->first = node;
	}
	else {
		node->next = list_ptr->first;
	}
	node->value = value;
	list_ptr->first = node;

	return 0;
} /* end of prepend_list */

/*********************************************************************
*
* Function  : print_list
*
* Purpose   : Print the contents of a linklist
*
* Inputs    : LINKLIST *list_ptr - pointer to linklink structure
*             char *title - display title
*
* Output    : appropriate error messages
*
* Returns   : (nothing)
*
* Example   : print_list(&linklist,"My List",&prev);
*
* Notes     : (none)
*
*********************************************************************/

int print_list(LINKLIST *list_ptr, char *title)
{
	NODE	*node;

	printf("\n%s\n",title);
	printf("%d nodes in list\n",list_ptr->num_nodes);
	node = list_ptr->first;
	for ( ; node != NULL ; node = node->next ) {
		printf("%d\n",node->value);
	}

	return 0;
} /* end of print_list */

/*********************************************************************
*
* Function  : search_list
*
* Purpose   : Search a linklist for a specific value
*
* Inputs    : LINKLIST *list_ptr - pointer to linklink structure
*             int value - search value
*             NODE **prev - addr of ptr to receive ptr to prev node
*
* Output    : (none)
*
* Returns   : IF found THEN ptr to node ELSE NULL
*
* Example   : node_ptr = search_list(&linklist,77,&prev);
*
* Notes     : (none)
*
*********************************************************************/

NODE *search_list(LINKLIST *list_ptr, int value, NODE **prev)
{
	NODE	*node;

	*prev = NULL;
	for ( node = list_ptr->first ; node != NULL ; node = node->next ) {
		if ( value == node->value ) {
			return node;
		}
		*prev = node;
	}

	return NULL;
} /* end of search_list */

/*********************************************************************
*
* Function  : insert_list_after
*
* Purpose   : Insert a node into a linklist after a specific node
*
* Inputs    : LINKLIST *list_ptr - pointer to linklink structure
*             int search_value - search value
*             int new_value - new node value
*
* Output    : appropriate error messages
*
* Returns   : IF problem THEN NULL ELSE node pointer
*
* Example   : node = insert_list_after(&linklist,33,1313);
*
* Notes     : (none)
*
*********************************************************************/

NODE *insert_list_after(LINKLIST *list_ptr, int search_value, int new_value)
{
	NODE	*old_node , *node , *prev;

	debug_print("\ninsert_list_after() ; search = %d , new = %d\n",search_value,new_value);
	old_node = search_list(list_ptr,search_value,&prev);
	if ( old_node == NULL ) {
		printf("\nAttempt to insert the value %d after a non-existant value %d\n",new_value,search_value);
		return NULL;
	}
	node = (NODE *)calloc(1,sizeof(NODE));
	if ( node == NULL ) {
		system_error("Can't allocate new node");
		return NULL;
	}
	node->value = new_value;
	if ( old_node == list_ptr->last ) {
		list_ptr->last->next = node;
		list_ptr->last = node;
	}
	else {
		node->next = old_node->next;
		old_node->next = node;
	}

	return node;
} /* end of insert_list_after */

/*********************************************************************
*
* Function  : insert_list_before
*
* Purpose   : Insert a node into a linklist before a specific node
*
* Inputs    : LINKLIST *list_ptr - pointer to linklink structure
*             int search_value - search value
*             int new_value - new node value
*
* Output    : appropriate error messages
*
* Returns   : IF problem THEN NULL ELSE node pointer
*
* Example   : node = insert_list_before(&linklist,33,1313);
*
* Notes     : (none)
*
*********************************************************************/

NODE *insert_list_before(LINKLIST *list_ptr, int search_value, int new_value)
{
	NODE	*old_node , *node , *prev;

	debug_print("\ninsert_list_before() ; search = %d , new = %d\n",search_value,new_value);
	old_node = search_list(list_ptr,search_value,&prev);
	if ( old_node == NULL ) {
		printf("\nAttempt to insert the value %d before a non-existant value %d\n",new_value,search_value);
		return NULL;
	}
	node = (NODE *)calloc(1,sizeof(NODE));
	if ( node == NULL ) {
		system_error("Can't allocate new node");
		return NULL;
	}
	node->value = new_value;
	if ( old_node == list_ptr->first ) {
		node->next = list_ptr->first;
		list_ptr->first = node;
	}
	else {
		node->next = old_node;
		prev->next = node;
	}

	return node;
} /* end of insert_list_before */

/*********************************************************************
*
* Function  : remove_from_list
*
* Purpose   : Remove a value from a linklist
*
* Inputs    : LINKLIST *list_ptr - pointer to linklink structure
*             int value - value to be removed
*
* Output    : appropriate error messages
*
* Returns   : IF problem THEN negative ELSE zero
*
* Example   : status = remove_from_list(&linklist,99);
*
* Notes     : (none)
*
*********************************************************************/

int remove_from_list(LINKLIST *list_ptr, int value)
{
	NODE	*node , *prev;

	node = search_list(list_ptr,value,&prev);
	if ( node == NULL ) {
		printf("Value %d was not found in the list\n",value);
		return -1;
	}
	list_ptr->num_nodes -= 1;
	if ( node == list_ptr->first ) {
		list_ptr->first = node->next;
	}
	else {
		if ( node == list_ptr->last ) {
			prev->next = NULL;
		}
		else {
			prev->next = node->next;
		}
	}
	free(node);

	return 0;
} /* end of remove_from_list */

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
* Example   : qsort1
*
* Notes     : (none)
*
*********************************************************************/

int main(int argc, char *argv[])
{
	int		status , errflag , c;
	LINKLIST	linklist;
	NODE	*node;

	errflag = 0;
	while ( (c = _getopt(argc,argv,":dh")) != -1 ) {
		switch (c) {
		case 'h':
			opt_h = 1;
			break;
		case 'd':
			opt_d = 1;
			break;
		case '?':
			printf("Unknown option '%c'\n",optopt);
			errflag += 1;
			break;
		case ':':
			printf("Missing value for option '%c'\n",optopt);
			errflag += 1;
			break;
		default:
			printf("Unexpected value from getopt() '%c'\n",c);
		} /* SWITCH */
	} /* WHILE */
	if ( errflag ) {
		usage(argv[0]);
		die(1,"\nAborted due to parameter errors\n");
	} /* IF */
	if ( opt_h ) {
		usage(argv[0]);
		exit(0);
	} /* IF */

    /* set the INT (Ctrl-C) signal handler to 'catch_int' */
    signal(SIGINT, catch_int);
    /* signal(SIGBUS, catch_sig); */
    signal(SIGSEGV, catch_sig);

	num_args = argc - optind;
	status = init_list(&linklist);

	append_list(&linklist,33);
	append_list(&linklist,66);
	prepend_list(&linklist,13);
	print_list(&linklist,"My List");

	node = insert_list_after(&linklist,33,55);
	print_list(&linklist,"My List");

	node = insert_list_before(&linklist,66,60);
	print_list(&linklist,"My List");

	remove_from_list(&linklist,55);
	print_list(&linklist,"My List");

	exit(0);
} /* end of main */
