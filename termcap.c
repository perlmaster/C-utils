/***************************************************************
* File:         termcap.c
*
* Created:      08.17.99
*
* By:           Barry Kimelman
*
* Purpose:      Functions to manipulate the screen.
*
*
***************************************************************/

#include        <stdio.h>
#include        <curses.h>
#include        <term.h>
#include        <stdlib.h>
#include        <stdarg.h>

/* characters for drawing a box */
#define BOX_ULC '+'
#define BOX_URC '+'
#define BOX_LLC '+'
#define BOX_LRC '+'
#define BOX_LINESEG     '-'
#define BOX_VERTLINE    '|'

typedef	struct cursor_tag {
	char	*sequence;
	char	*description;
	char	ch;
} CURSOR_KEY;

CURSOR_KEY cursor_keys[5] = {
	{ NULL , "cursor up" , 'u' } ,
	{ NULL , "cursor down" , 'd' } ,
	{ NULL , "cursor right" , 'r' } ,
	{ NULL , "cursor left" , 'l' }
};
int	num_key_sequences = sizeof(cursor_keys) / sizeof(CURSOR_KEY);

/* terminal capability string variables */
static  char    *so , *se , *cm , *cl , *cd , *ce , *us , *ue , *bell_cap;
static	char	*ku , *kd , *kl , *kr , *kh , *sc , *rc , *le;

static  char    *termtype;      /* terminal type (eg. VT100) */

static  int     onechar(char); /* used to be a char parameter */

int     tty_num_rows = 24 , tty_num_cols = 80;

void    move_cursor_home() , move_cursor();
void	standout_print(char *fmt,...);

/*
* Function:     report_error
*
* Purpose:      Print an error message.
*
* Parameters:   file stream + (ala printf)
*
* Returns:      nothing
*
* Example:      report_error(stderr,"%s xyz\n","string1");
*/

static void report_error(FILE *err_fp, char *fmt,...)
{
	va_list ap;

	if ( err_fp != NULL ) {
		va_start(ap,fmt);
		vfprintf(err_fp,fmt,ap);
		fflush(stdout);
		va_end(ap);
	} /* IF */
    return;
} /* end of report_error */

/*
* Function:     onechar
*
* Purpose:      Print one character on the screen.
*
* Parameters:   ch - the character to be displayed.
*
* Returns:      zero
*
* Example:      onechar(ch);
*/

static int onechar(char ch)  /* used to be a char parameter (and is again) */
{
    putchar(ch);
    return(0);
} /* end of onechar */

/*
* Function:     clear_eol
*
* Purpose:      Clear the screen from the current position until
*               the end of the line.
*
* Parameters:   none
*
* Returns:      zero
*
* Example:      clear_eol();
*/

void clear_eol()
{
    tputs(ce,1,onechar);    /* clear to end of line */
    fflush(stdout);
    return;
} /* end of clear_eol */

/*
* Function:     clear_eos
*
* Purpose:      Clear the screen from the current position until
*               the end of the screen.
*
* Parameters:   none
*
* Returns:      zero
*
* Example:      clear_eos();
*/

void clear_eos()
{
    tputs(cd,1,onechar);    /* clear to end of screen */
    fflush(stdout);
    return;
} /* end of clear_eos */

/*
* Function:     clear_rectangle
*
* Purpose:      Clear a rectangular area on the screen.
*
* Parameters:   ulc_x - upper left hand corner x coordinate
*               ulc_y - upper left hand corner y coordinate
*               width - width of area
*               height - height of area
*
* Returns:      zero
*
* Example:      clear_rectangle(x,y,20,20);
*/

void clear_rectangle(int ulc_x, int ulc_y, int width, int height)
{
	int		x , y , count;
	char	blanks[1024];

	sprintf(blanks,"%*.*s",width,width," ");
	x = ulc_x;
	y = ulc_y;
	for ( count = 1 ; count <= height ; ++count , ++y ) {
		move_cursor(y,x);
		printf("%s",blanks);
	} /* FOR */
	fflush(stdout);

    return;
} /* end of clear_rectangle */

/*
* Function:     erase_screen
*
* Purpose:      Clear the screen and move the cursor to the home
*               position.
*
* Parameters:   none
*
* Returns:      nothing
*
* Example:      erase_screen();
*/

void erase_screen()
{
    move_cursor_home();
    clear_eos();
    return;
} /* erase_screen */

/*
* Function:     cursor_bottom
*
* Purpose:      Move the cursor to the 1st col on the last line
*
* Parameters:   (none)
*
* Returns:      nothing
*
* Example:      cursor_bottom();
*/

void cursor_bottom()
{
	move_cursor(tty_num_rows-1,0);
    fflush(stdout);
    return;
} /* end of cursor_bottom */

/*
* Function:     move_cursor_home
*
* Purpose:      Move the cursor to the "home" position
*
* Parameters:   (none)
*
* Returns:      nothing
*
* Example:      move_cursor_home();
*/

void move_cursor_home()
{
    tputs(cl,1,onechar);    /* move cursor to home position */
    fflush(stdout);
    return;
} /* end of move_cursor_home */

/*
* Function:     draw_box
*
* Purpose:      Draw a box on the screen.
*
* Parameters:   ulc_row - row of upper left corner of box
*               ulc_col - column of upper left corner of box
*               lrc_row - row of lower right corner of box
*               lrc_col - column of lower right corner of box
*
* Returns:      zero
*
* Example:      draw_box(5,5,20,20);
*/

int draw_box(int ulc_row, int ulc_col, int lrc_row, int lrc_col)
{
    int     box_height , box_width , count , row , col;
    int     width;

    box_height = (lrc_row - ulc_row) + 1;
    box_width = (lrc_col - ulc_col) + 1;
    if ( box_height < 3 || box_width < 3 ) {
            return(1);
    } /* IF */
    move_cursor(ulc_row,ulc_col);
    printf("%c",BOX_ULC);
    for ( count = 2 ; count < box_width ; ++count ) {
            printf("%c",BOX_LINESEG);
    }
    printf("%c",BOX_URC);

    width = box_width - 2;
    row = ulc_row + 1;
    for ( count = 2 ; count < box_height ; ++count ) {
        move_cursor(row,ulc_col);
        printf("%c",BOX_VERTLINE);
        move_cursor(row,lrc_col);
        printf("%c",BOX_VERTLINE);
        row += 1;
    } /* FOR */
    move_cursor(lrc_row,ulc_col);
    printf("%c",BOX_LLC);
    for ( count = 2 ; count < box_width ; ++count ) {
            printf("%c",BOX_LINESEG);
    }
    printf("%c",BOX_LRC);

    return(0);
} /* end of draw_box */

/*
* Function:     draw_smooth_box
*
* Purpose:      Draw a smooth edged box on the screen.
*
* Parameters:   ulc_x - column of upper left corner of box
*               ulc_y - row of upper left corner of box
*               height - height of box (including border)
*               width - width of box (including border)
*
* Returns:      zero
*
* Example:      draw_smooth_box(5,5,20,20);
*/

int draw_smooth_box(int x,int y,int height,int width)
{
	int		row , col , count;
	char	blanks[200] , blanks2[200];

	move_cursor(y,x);
	sprintf(blanks,"%*.*s",width,width," ");
	sprintf(blanks2,"%*.*s",width-2,width-2," ");
	standout_print("%s",blanks);
	row = y + 1;
	for ( count = 2 ; count < height ; ++count , ++row ) {
		move_cursor(row,x);
		standout_print(" ");
		printf("%s",blanks2);
		standout_print(" ");
	} /* FOR */
	move_cursor(row,x);
	standout_print("%s",blanks);
} /* end of draw_smooth_box */

/*
* Function:     end_standout_mode
*
* Purpose:      Turn off standout mode.
*
* Parameters:   none
*
* Returns:      nothing
*
* Example:      end_standout_mode();
*/

void end_standout_mode()
{
    tputs(se,1,onechar);    /* end standout mode */
    fflush(stdout);
    return;
} /* end_standout_mode */

/*
* Function:     end_underline_mode
*
* Purpose:      Turn off underline mode.
*
* Parameters:   none
*
* Returns:      nothing
*
* Example:      end_underline_mode();
*/

void end_underline_mode()
{
    tputs(ue,1,onechar);    /* end underline mode */
    fflush(stdout);
    return;
} /* end_underline_mode */

/*
* Function:     init_termcap
*
* Purpose:      Perform initialization work required by the other
*               screen manipulation functions.
*
* Parameters:   errors_fp - if not null then this is the stream to
*                           receive error messages
*
* Returns:      0 --> success , 1 --> error
*
* Example:      status = init_termcap();
*/

int init_termcap(FILE *errors_fp)
{
    int		status;
    char    termcap[10240] , *string;

	tty_num_rows = 24;
	tty_num_cols = 80;
    termtype = getenv("TERM");
    if ( termtype == NULL ) {
           	report_error(errors_fp,"No TERM available.\n");
            return(1);
    } /* IF */

    status = tgetent(termcap,termtype);
    if ( status == ERR ) {
           	report_error(errors_fp,"tgetent failed for %s, status = %d\n",
							termtype,status);
            return(1);
    } /* IF */
    so = tgetstr("so",NULL);
    if ( so == NULL ) {
	        report_error(errors_fp,"No standout mode for %s\n",termtype);
            return(1);
    } /* IF */
    se = tgetstr("se",NULL);
    if ( se == NULL ) {
            report_error(errors_fp,"No end-standout mode for %s\n",termtype);
            return(1);
    } /* IF */
    us = tgetstr("us",NULL);
    if ( us == NULL ) {
            report_error(errors_fp,"No underline mode for %s\n",termtype);
            return(1);
    } /* IF */
    ue = tgetstr("ue",NULL);
    if ( ue == NULL ) {
            report_error(errors_fp,"No end-underline mode for %s\n",termtype);
            return(1);
    } /* IF */
    cm = tgetstr("cm",NULL);
    if ( cm == NULL ) {
            report_error(errors_fp,"No cursor-move for %s\n",termtype);
            return(1);
    } /* IF */
    cl = tgetstr("cl",NULL);
    if ( cl == NULL ) {
            report_error(errors_fp,"No cursor-home (cl) for %s\n",termtype);
            return(1);
    } /* IF */
    cd = tgetstr("cd",NULL);
    if ( cd == NULL ) {
            report_error(errors_fp,"No cursor-clear-down for %s\n",termtype);
            return(1);
    } /* IF */
    ce = tgetstr("ce",NULL);
    if ( ce == NULL ) {
            report_error(errors_fp,"No cursor-clear-line for %s\n",termtype);
            return(1);
    } /* IF */
    le = tgetstr("le",NULL);
    if ( le == NULL ) {
            report_error(errors_fp,"No cursor-backup-char for %s\n",termtype);
            return(1);
    } /* IF */
    bell_cap = tgetstr("bell",NULL);
    tty_num_rows = tgetnum("li");
    tty_num_cols = tgetnum("co");
    sc = tgetstr("sc",NULL);
    if ( sc == NULL ) {
            report_error(errors_fp,"No save cursor for %s\n",termtype);
            return(1);
    } /* IF */
    rc = tgetstr("rc",NULL);
    if ( rc == NULL ) {
            report_error(errors_fp,"No restore cursor for %s\n",termtype);
            return(1);
    } /* IF */

    ku = tgetstr("ku",NULL);
    if ( ku == NULL ) {
            report_error(errors_fp,"No cursor-up for %s\n",termtype);
            return(1);
    } /* IF */
	else {
		cursor_keys[0].sequence = ku;
	} /* ELSE */

    kd = tgetstr("kd",NULL);
    if ( kd == NULL ) {
            report_error(errors_fp,"No cursor-down for %s\n",termtype);
            return(1);
    } /* IF */
	else {
		cursor_keys[1].sequence = kd;
	} /* ELSE */

    kr = tgetstr("kr",NULL);
    if ( kr == NULL ) {
            report_error(errors_fp,"No cursor-right for %s\n",termtype);
            return(1);
    } /* IF */
	else {
		cursor_keys[2].sequence = kr;
	} /* ELSE */

    kl = tgetstr("kl",NULL);
    if ( kl == NULL ) {
            report_error(errors_fp,"No cursor-left for %s\n",termtype);
            return(1);
    } /* IF */
	else {
		cursor_keys[3].sequence = kl;
	} /* ELSE */

    kh = tgetstr("kh",NULL);
    if ( kh == NULL ) {
            report_error(NULL,"No cursor-home (kh) for %s\n",termtype);
    } /* IF */

    return(0);
} /* end of init_termcap */

/*
* Function:     move_cursor
*
* Purpose:      Move the cursor to the specified coordinates.
*
* Parameters:   row - row on screen
*               col - column within row
*
* Returns:      nothing
*
* Example:      move_cursor(10,10);
*/

void move_cursor(int row, int col)
{
    char    *string;

    string = (char *)tgoto(cm,col,row);
    tputs(string,1,onechar);        /* move cursor to row,col */
    fflush(stdout);
    return;
} /* move_cursor */

/*
* Function:     recall_cursor_position
*
* Purpose:      Restore a saved cursor position.
*
* Parameters:   none
*
* Returns:      zero
*
* Example:      recall_cursor_position();
*/

void recall_cursor_position()
{
    tputs(rc,1,onechar);    /* restore cursor position */
    fflush(stdout);
    return;
} /* end of recall_cursor_position */

/*
* Function:     move_cursor_left
*
* Purpose:      Restore a saved cursor position.
*
* Parameters:   none
*
* Returns:      zero
*
* Example:      move_cursor_left();
*/

void move_cursor_left()
{
    tputs(le,1,onechar);    /* restore cursor position */
    fflush(stdout);
    return;
} /* end of move_cursor_left */

/*
* Function:     remember_cursor_position
*
* Purpose:      Save cursor position.
*
* Parameters:   none
*
* Returns:      zero
*
* Example:      remember_cursor_position();
*/

void remember_cursor_position()
{
    tputs(sc,1,onechar);    /* save cursor position */
    fflush(stdout);
    return;
} /* end of remember_cursor_position */

/*
* Function:     tty_ring_bell
*
* Purpose:      Ring the bell.
*
* Parameters:   (none)
*
* Returns:      zero
*
* Example:      tty_ring_bell();
*/

int tty_ring_bell()
{
	if ( bell_cap != NULL )
	    tputs(bell_cap,1,onechar);    /* ring the bell */
	else
		onechar('\007');
    fflush(stdout);
    return(0);
} /* end of tty_ring_bell */

/*
* Function:     standout_mode
*
* Purpose:      Turn on standout mode.
*
* Parameters:   none
*
* Returns:      nothing
*
* Example:      standout_mode();
*/

void standout_mode()
{
    tputs(so,1,onechar);    /* standout mode */
    fflush(stdout);
    return;
} /* standout_mode */

/*
* Function:     standout_print
*
* Purpose:      Print a message on the screen in "standout" mode.
*
* Parameters:   (ala printf)
*
* Returns:      nothing
*
* Example:      standout_print("%s xyz\n","string1");
*/

void standout_print(char *fmt,...)
{
    va_list ap;

    standout_mode();
    va_start(ap,fmt);
    vprintf(fmt,ap);
    fflush(stdout);
    va_end(ap);
    end_standout_mode();
    return;
} /* end of standout_print */

/*
* Function:     underline_mode
*
* Purpose:      Turn on underline mode.
*
* Parameters:   none
*
* Returns:      nothing
*
* Example:      underline_mode();
*/

void underline_mode()
{
    tputs(us,1,onechar);    /* underline mode */
    fflush(stdout);
    return;
} /* underline_mode */

/*
* Function:     underline_print
*
* Purpose:      Print a message on the screen in "underline" mode.
*
* Parameters:   (ala printf)
*
* Returns:      nothing
*
* Example:      underline_print("%s xyz\n","string1");
*/

void underline_print(char *fmt,...)
{
    va_list ap;

    underline_mode();
    va_start(ap,fmt);
    vprintf(fmt,ap);
    fflush(stdout);
    va_end(ap);
    end_underline_mode();
    return;
} /* end of underline_print */

/*
* Function:     read_cursor_key
*
* Purpose:      To read the escape sequence for one of the supported
*               cursor keys ( Up , Down , Right , Left , Home )
*
* Parameters:   int escape - boolean flag indicating if the escape
*                            key has already been read
*
* Returns:      single character indicating which cursor key was read
*                   'u' --> up
*                   'd' --> down
*                   'l' --> left
*                   'r' --> right
*                   'h' --> home
*                   ' ' --> invalid/unsupported key sequence
*
* Example:      cursor_key = read_cursor_key(1);
*
* Notes:        This routine makes several assumptions:
*               (1) All the cursor key sequences start with an
*                   escape character
*               (2) All the cursor key sequences are the same length
*                   (ie. all are 3 characters)
*               (3) The terminal attributes have been set so that a
*                   single character can be read without waiting for
*                   a carriage-return to be pressed
*/

char read_cursor_key(int escape)
{
	unsigned char ch , chars[4];
	int		count , index;

	if ( escape ) {
		chars[0] = '\033';
		read(0,&chars[1],2);  /* read the Escape character */
	} /* IF */
	else {
		read(0,chars,3);  /* read the Escape character */
	} /* ELSE */
	chars[3] = '\0';
#ifdef TTY_JUNK
	printf("Sequence : %02x %02x %02x",chars[0],chars[1],chars[2]);
	fflush(stdout);
#endif

	for ( index = 0 ; index < num_key_sequences ; ++index ) {
#ifdef JUNK
		if ( cursor_keys[index].sequence != NULL &&
					memcmp(chars,&cursor_keys[index].sequence) == 0 ) {
			return(cursor_keys[index].ch);
		} /* IF */
#else
/************************************************************
A fudge for now. The /etc/termcap file disagrees with the
actual characters being sent from my keyboard in the 2nd
character position.
************************************************************/
		if ( cursor_keys[index].sequence != NULL &&
				chars[2] == cursor_keys[index].sequence[2] ) {
			return(cursor_keys[index].ch);
		} /* IF */
#endif
	} /* FOR */

	return(' ');
} /* end of read_cursor_key */
