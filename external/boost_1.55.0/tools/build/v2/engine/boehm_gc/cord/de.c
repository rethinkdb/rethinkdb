/*
 * Copyright (c) 1993-1994 by Xerox Corporation.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 * Author: Hans-J. Boehm (boehm@parc.xerox.com)
 */
/*
 * A really simple-minded text editor based on cords.
 * Things it does right:
 * 	No size bounds.
 *	Inbounded undo.
 *	Shouldn't crash no matter what file you invoke it on (e.g. /vmunix)
 *		(Make sure /vmunix is not writable before you try this.)
 *	Scrolls horizontally.
 * Things it does wrong:
 *	It doesn't handle tabs reasonably (use "expand" first).
 *	The command set is MUCH too small.
 *	The redisplay algorithm doesn't let curses do the scrolling.
 *	The rule for moving the window over the file is suboptimal.
 */
/* Boehm, February 6, 1995 12:27 pm PST */

/* Boehm, May 19, 1994 2:20 pm PDT */
#include <stdio.h>
#include "gc.h"
#include "cord.h"

#ifdef THINK_C
#define MACINTOSH
#include <ctype.h>
#endif

#if defined(__BORLANDC__) && !defined(WIN32)
    /* If this is DOS or win16, we'll fail anyway.	*/
    /* Might as well assume win32.			*/
#   define WIN32
#endif

#if defined(WIN32)
#  include <windows.h>
#  include "de_win.h"
#elif defined(MACINTOSH)
#	include <console.h>
/* curses emulation. */
#	define initscr()
#	define endwin()
#	define nonl()
#	define noecho() csetmode(C_NOECHO, stdout)
#	define cbreak() csetmode(C_CBREAK, stdout)
#	define refresh()
#	define addch(c) putchar(c)
#	define standout() cinverse(1, stdout)
#	define standend() cinverse(0, stdout)
#	define move(line,col) cgotoxy(col + 1, line + 1, stdout)
#	define clrtoeol() ccleol(stdout)
#	define de_error(s) { fprintf(stderr, s); getchar(); }
#	define LINES 25
#	define COLS 80
#else
#  include <curses.h>
#  define de_error(s) { fprintf(stderr, s); sleep(2); }
#endif
#include "de_cmds.h"

/* List of line number to position mappings, in descending order. */
/* There may be holes.						  */
typedef struct LineMapRep {
    int line;
    size_t pos;
    struct LineMapRep * previous;
} * line_map;

/* List of file versions, one per edit operation */
typedef struct HistoryRep {
    CORD file_contents;
    struct HistoryRep * previous;
    line_map map;	/* Invalid for first record "now" */
} * history;

history now = 0;
CORD current;		/* == now -> file_contents.	*/
size_t current_len;	/* Current file length.		*/
line_map current_map = 0;	/* Current line no. to pos. map	 */
size_t current_map_size = 0;	/* Number of current_map entries.	*/
				/* Not always accurate, but reset	*/
				/* by prune_map.			*/
# define MAX_MAP_SIZE 3000

/* Current display position */
int dis_line = 0;
int dis_col = 0;

# define ALL -1
# define NONE - 2
int need_redisplay = 0;	/* Line that needs to be redisplayed.	*/


/* Current cursor position. Always within file. */
int line = 0; 
int col = 0;
size_t file_pos = 0;	/* Character position corresponding to cursor.	*/

/* Invalidate line map for lines > i */
void invalidate_map(int i)
{
    while(current_map -> line > i) {
        current_map = current_map -> previous;
        current_map_size--;
    }
}

/* Reduce the number of map entries to save space for huge files. */
/* This also affects maps in histories.				  */
void prune_map()
{
    line_map map = current_map;
    int start_line = map -> line;
    
    current_map_size = 0;
    for(; map != 0; map = map -> previous) {
    	current_map_size++;
    	if (map -> line < start_line - LINES && map -> previous != 0) {
    	    map -> previous = map -> previous -> previous;
    	}
    }
}
/* Add mapping entry */
void add_map(int line, size_t pos)
{
    line_map new_map = GC_NEW(struct LineMapRep);
    
    if (current_map_size >= MAX_MAP_SIZE) prune_map();
    new_map -> line = line;
    new_map -> pos = pos;
    new_map -> previous = current_map;
    current_map = new_map;
    current_map_size++;
}



/* Return position of column *c of ith line in   */
/* current file. Adjust *c to be within the line.*/
/* A 0 pointer is taken as 0 column.		 */
/* Returns CORD_NOT_FOUND if i is too big.	 */
/* Assumes i > dis_line.			 */
size_t line_pos(int i, int *c)
{
    int j;
    size_t cur;
    size_t next;
    line_map map = current_map;
    
    while (map -> line > i) map = map -> previous;
    if (map -> line < i - 2) /* rebuild */ invalidate_map(i);
    for (j = map -> line, cur = map -> pos; j < i;) {
	cur = CORD_chr(current, cur, '\n');
        if (cur == current_len-1) return(CORD_NOT_FOUND);
        cur++;
        if (++j > current_map -> line) add_map(j, cur);
    }
    if (c != 0) {
        next = CORD_chr(current, cur, '\n');
        if (next == CORD_NOT_FOUND) next = current_len - 1;
        if (next < cur + *c) {
            *c = next - cur;
        }
        cur += *c;
    }
    return(cur);
}

void add_hist(CORD s)
{
    history new_file = GC_NEW(struct HistoryRep);
    
    new_file -> file_contents = current = s;
    current_len = CORD_len(s);
    new_file -> previous = now;
    if (now != 0) now -> map = current_map;
    now = new_file;
}

void del_hist(void)
{
    now = now -> previous;
    current = now -> file_contents;
    current_map = now -> map;
    current_len = CORD_len(current);
}

/* Current screen_contents; a dynamically allocated array of CORDs	*/
CORD * screen = 0;
int screen_size = 0;

# ifndef WIN32
/* Replace a line in the curses stdscr.	All control characters are	*/
/* displayed as upper case characters in standout mode.  This isn't	*/
/* terribly appropriate for tabs.									*/
void replace_line(int i, CORD s)
{
    register int c;
    CORD_pos p;
    size_t len = CORD_len(s);
    
    if (screen == 0 || LINES > screen_size) {
        screen_size = LINES;
    	screen = (CORD *)GC_MALLOC(screen_size * sizeof(CORD));
    }
#   if !defined(MACINTOSH)
        /* A gross workaround for an apparent curses bug: */
        if (i == LINES-1 && len == COLS) {
            s = CORD_substr(s, 0, CORD_len(s) - 1);
        }
#   endif
    if (CORD_cmp(screen[i], s) != 0) {
        move(i, 0); clrtoeol(); move(i,0);

        CORD_FOR (p, s) {
            c = CORD_pos_fetch(p) & 0x7f;
            if (iscntrl(c)) {
            	standout(); addch(c + 0x40); standend();
            } else {
    	        addch(c);
    	    }
    	}
    	screen[i] = s;
    }
}
#else
# define replace_line(i,s) invalidate_line(i)
#endif

/* Return up to COLS characters of the line of s starting at pos,	*/
/* returning only characters after the given column.			*/
CORD retrieve_line(CORD s, size_t pos, unsigned column)
{
    CORD candidate = CORD_substr(s, pos, column + COLS);
    			/* avoids scanning very long lines	*/
    int eol = CORD_chr(candidate, 0, '\n');
    int len;
    
    if (eol == CORD_NOT_FOUND) eol = CORD_len(candidate);
    len = (int)eol - (int)column;
    if (len < 0) len = 0;
    return(CORD_substr(s, pos + column, len));
}

# ifdef WIN32
#   define refresh();

    CORD retrieve_screen_line(int i)
    {
    	register size_t pos;
    	
    	invalidate_map(dis_line + LINES);	/* Prune search */
    	pos = line_pos(dis_line + i, 0);
    	if (pos == CORD_NOT_FOUND) return(CORD_EMPTY);
    	return(retrieve_line(current, pos, dis_col));
    }
# endif

/* Display the visible section of the current file	 */
void redisplay(void)
{
    register int i;
    
    invalidate_map(dis_line + LINES);	/* Prune search */
    for (i = 0; i < LINES; i++) {
        if (need_redisplay == ALL || need_redisplay == i) {
            register size_t pos = line_pos(dis_line + i, 0);
            
            if (pos == CORD_NOT_FOUND) break;
            replace_line(i, retrieve_line(current, pos, dis_col));
            if (need_redisplay == i) goto done;
        }
    }
    for (; i < LINES; i++) replace_line(i, CORD_EMPTY);
done:
    refresh();
    need_redisplay = NONE;
}

int dis_granularity;

/* Update dis_line, dis_col, and dis_pos to make cursor visible.	*/
/* Assumes line, col, dis_line, dis_pos are in bounds.			*/
void normalize_display()
{
    int old_line = dis_line;
    int old_col = dis_col;
    
    dis_granularity = 1;
    if (LINES > 15 && COLS > 15) dis_granularity = 2;
    while (dis_line > line) dis_line -= dis_granularity;
    while (dis_col > col) dis_col -= dis_granularity;
    while (line >= dis_line + LINES) dis_line += dis_granularity;
    while (col >= dis_col + COLS) dis_col += dis_granularity;
    if (old_line != dis_line || old_col != dis_col) {
        need_redisplay = ALL;
    }
}

# if defined(WIN32)
# elif defined(MACINTOSH)
#		define move_cursor(x,y) cgotoxy(x + 1, y + 1, stdout)
# else
#		define move_cursor(x,y) move(y,x)
# endif

/* Adjust display so that cursor is visible; move cursor into position	*/
/* Update screen if necessary.						*/
void fix_cursor(void)
{
    normalize_display();
    if (need_redisplay != NONE) redisplay();
    move_cursor(col - dis_col, line - dis_line);
    refresh();
#   ifndef WIN32
      fflush(stdout);
#   endif
}

/* Make sure line, col, and dis_pos are somewhere inside file.	*/
/* Recompute file_pos.	Assumes dis_pos is accurate or past eof	*/
void fix_pos()
{
    int my_col = col;
    
    if ((size_t)line > current_len) line = current_len;
    file_pos = line_pos(line, &my_col);
    if (file_pos == CORD_NOT_FOUND) {
        for (line = current_map -> line, file_pos = current_map -> pos;
             file_pos < current_len;
             line++, file_pos = CORD_chr(current, file_pos, '\n') + 1);
    	line--;
        file_pos = line_pos(line, &col);
    } else {
    	col = my_col;
    }
}

#if defined(WIN32)
#  define beep() Beep(1000 /* Hz */, 300 /* msecs */) 
#elif defined(MACINTOSH)
#	define beep() SysBeep(1)
#else
/*
 * beep() is part of some curses packages and not others.
 * We try to match the type of the builtin one, if any.
 */
#ifdef __STDC__
    int beep(void)
#else
    int beep()
#endif
{
    putc('\007', stderr);
    return(0);
}
#endif

#   define NO_PREFIX -1
#   define BARE_PREFIX -2
int repeat_count = NO_PREFIX;	/* Current command prefix. */

int locate_mode = 0;			/* Currently between 2 ^Ls	*/
CORD locate_string = CORD_EMPTY;	/* Current search string.	*/

char * arg_file_name;

#ifdef WIN32
/* Change the current position to whatever is currently displayed at	*/
/* the given SCREEN coordinates.					*/
void set_position(int c, int l)
{
    line = l + dis_line;
    col = c + dis_col;
    fix_pos();
    move_cursor(col - dis_col, line - dis_line);
}
#endif /* WIN32 */

/* Perform the command associated with character c.  C may be an	*/
/* integer > 256 denoting a windows command, one of the above control	*/
/* characters, or another ASCII character to be used as either a 	*/
/* character to be inserted, a repeat count, or a search string, 	*/
/* depending on the current state.					*/
void do_command(int c)
{
    int i;
    int need_fix_pos;
    FILE * out;
    
    if ( c == '\r') c = '\n';
    if (locate_mode) {
        size_t new_pos;
          
        if (c == LOCATE) {
              locate_mode = 0;
              locate_string = CORD_EMPTY;
              return;
        }
        locate_string = CORD_cat_char(locate_string, (char)c);
        new_pos = CORD_str(current, file_pos - CORD_len(locate_string) + 1,
          		   locate_string);
        if (new_pos != CORD_NOT_FOUND) {
            need_redisplay = ALL;
            new_pos += CORD_len(locate_string);
            for (;;) {
              	file_pos = line_pos(line + 1, 0);
              	if (file_pos > new_pos) break;
              	line++;
            }
            col = new_pos - line_pos(line, 0);
            file_pos = new_pos;
            fix_cursor();
        } else {
            locate_string = CORD_substr(locate_string, 0,
              				CORD_len(locate_string) - 1);
            beep();
        }
        return;
    }
    if (c == REPEAT) {
      	repeat_count = BARE_PREFIX; return;
    } else if (c < 0x100 && isdigit(c)){
        if (repeat_count == BARE_PREFIX) {
          repeat_count = c - '0'; return;
        } else if (repeat_count != NO_PREFIX) {
          repeat_count = 10 * repeat_count + c - '0'; return;
        }
    }
    if (repeat_count == NO_PREFIX) repeat_count = 1;
    if (repeat_count == BARE_PREFIX && (c == UP || c == DOWN)) {
      	repeat_count = LINES - dis_granularity;
    }
    if (repeat_count == BARE_PREFIX) repeat_count = 8;
    need_fix_pos = 0;
    for (i = 0; i < repeat_count; i++) {
        switch(c) {
          case LOCATE:
            locate_mode = 1;
            break;
          case TOP:
            line = col = file_pos = 0;
            break;
     	  case UP:
     	    if (line != 0) {
     	        line--;
     	        need_fix_pos = 1;
     	    }
     	    break;
     	  case DOWN:
     	    line++;
     	    need_fix_pos = 1;
     	    break;
     	  case LEFT:
     	    if (col != 0) {
     	        col--; file_pos--;
     	    }
     	    break;
     	  case RIGHT:
     	    if (CORD_fetch(current, file_pos) == '\n') break;
     	    col++; file_pos++;
     	    break;
     	  case UNDO:
     	    del_hist();
     	    need_redisplay = ALL; need_fix_pos = 1;
     	    break;
     	  case BS:
     	    if (col == 0) {
     	        beep();
     	        break;
     	    }
     	    col--; file_pos--;
     	    /* fall through: */
     	  case DEL:
     	    if (file_pos == current_len-1) break;
     	    	/* Can't delete trailing newline */
     	    if (CORD_fetch(current, file_pos) == '\n') {
     	        need_redisplay = ALL; need_fix_pos = 1;
     	    } else {
     	        need_redisplay = line - dis_line;
     	    }
     	    add_hist(CORD_cat(
     	    		CORD_substr(current, 0, file_pos),
     	    		CORD_substr(current, file_pos+1, current_len)));
     	    invalidate_map(line);
     	    break;
     	  case WRITE:
	    {
  		CORD name = CORD_cat(CORD_from_char_star(arg_file_name),
				     ".new");

    	        if ((out = fopen(CORD_to_const_char_star(name), "wb")) == NULL
  	            || CORD_put(current, out) == EOF) {
        	    de_error("Write failed\n");
        	    need_redisplay = ALL;
                } else {
                    fclose(out);
                }
	    }
            break;
     	  default:
     	    {
     	        CORD left_part = CORD_substr(current, 0, file_pos);
     	        CORD right_part = CORD_substr(current, file_pos, current_len);
     	        
     	        add_hist(CORD_cat(CORD_cat_char(left_part, (char)c),
     	        		  right_part));
     	        invalidate_map(line);
     	        if (c == '\n') {
     	            col = 0; line++; file_pos++;
     	            need_redisplay = ALL;
     	        } else {
     	            col++; file_pos++;
     	            need_redisplay = line - dis_line;
     	    	}
     	        break;
     	    }
        }
    }
    if (need_fix_pos) fix_pos();
    fix_cursor();
    repeat_count = NO_PREFIX;
}

/* OS independent initialization */

void generic_init(void)
{
    FILE * f;
    CORD initial;
    
    if ((f = fopen(arg_file_name, "rb")) == NULL) {
     	initial = "\n";
    } else {
        initial = CORD_from_file(f);
        if (initial == CORD_EMPTY
            || CORD_fetch(initial, CORD_len(initial)-1) != '\n') {
            initial = CORD_cat(initial, "\n");
        }
    }
    add_map(0,0);
    add_hist(initial);
    now -> map = current_map;
    now -> previous = now;  /* Can't back up further: beginning of the world */
    need_redisplay = ALL;
    fix_cursor();
}

#ifndef WIN32

main(argc, argv)
int argc;
char ** argv;
{
    int c;

#if defined(MACINTOSH)
	console_options.title = "\pDumb Editor";
	cshow(stdout);
	argc = ccommand(&argv);
#endif
    GC_INIT();
    
    if (argc != 2) goto usage;
    arg_file_name = argv[1];
    setvbuf(stdout, GC_MALLOC_ATOMIC(8192), _IOFBF, 8192);
    initscr();
    noecho(); nonl(); cbreak();
    generic_init();
    while ((c = getchar()) != QUIT) {
		if (c == EOF) break;
	    do_command(c);
    }
done:
    move(LINES-1, 0);
    clrtoeol();
    refresh();
    nl();
    echo();
    endwin();
    exit(0);
usage:
    fprintf(stderr, "Usage: %s file\n", argv[0]);
    fprintf(stderr, "Cursor keys: ^B(left) ^F(right) ^P(up) ^N(down)\n");
    fprintf(stderr, "Undo: ^U    Write to <file>.new: ^W");
    fprintf(stderr, "Quit:^D  Repeat count: ^R[n]\n");
    fprintf(stderr, "Top: ^T   Locate (search, find): ^L text ^L\n");
    exit(1);
}

#endif  /* !WIN32 */
