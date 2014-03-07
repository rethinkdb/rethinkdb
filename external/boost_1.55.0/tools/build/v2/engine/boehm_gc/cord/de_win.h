/*
 * Copyright (c) 1994 by Xerox Corporation.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */
/* Boehm, May 19, 1994 2:25 pm PDT */

/* cord.h, de_cmds.h, and windows.h should be included before this. */


# define OTHER_FLAG	0x100
# define EDIT_CMD_FLAG	0x200
# define REPEAT_FLAG	0x400

# define CHAR_CMD(i) ((i) & 0xff)

/* MENU: DE */
#define IDM_FILESAVE		(EDIT_CMD_FLAG + WRITE)
#define IDM_FILEEXIT		(OTHER_FLAG + 1)
#define IDM_HELPABOUT		(OTHER_FLAG + 2)
#define IDM_HELPCONTENTS	(OTHER_FLAG + 3)

#define IDM_EDITPDOWN		(REPEAT_FLAG + EDIT_CMD_FLAG + DOWN)
#define IDM_EDITPUP		(REPEAT_FLAG + EDIT_CMD_FLAG + UP)
#define IDM_EDITUNDO		(EDIT_CMD_FLAG + UNDO)
#define IDM_EDITLOCATE		(EDIT_CMD_FLAG + LOCATE)
#define IDM_EDITDOWN		(EDIT_CMD_FLAG + DOWN)
#define IDM_EDITUP		(EDIT_CMD_FLAG + UP)
#define IDM_EDITLEFT		(EDIT_CMD_FLAG + LEFT)
#define IDM_EDITRIGHT		(EDIT_CMD_FLAG + RIGHT)
#define IDM_EDITBS		(EDIT_CMD_FLAG + BS)
#define IDM_EDITDEL		(EDIT_CMD_FLAG + DEL)
#define IDM_EDITREPEAT		(EDIT_CMD_FLAG + REPEAT)
#define IDM_EDITTOP		(EDIT_CMD_FLAG + TOP)




/* Windows UI stuff	*/

LRESULT CALLBACK WndProc (HWND hwnd, UINT message,
			  UINT wParam, LONG lParam);

LRESULT CALLBACK AboutBox( HWND hDlg, UINT message,
			   UINT wParam, LONG lParam );


/* Screen dimensions.  Maintained by de_win.c.	*/
extern int LINES;
extern int COLS;

/* File being edited.	*/
extern char * arg_file_name;

/* Current display position in file.  Maintained by de.c	*/
extern int dis_line;
extern int dis_col;

/* Current cursor position in file.				*/
extern int line;
extern int col;

/*
 *  Calls from de_win.c to de.c
 */
  
CORD retrieve_screen_line(int i);
			/* Get the contents of i'th screen line.	*/
			/* Relies on COLS.				*/

void set_position(int x, int y);
			/* Set column, row.  Upper left of window = (0,0). */

void do_command(int);
			/* Execute an editor command.			*/
			/* Agument is a command character or one	*/
			/* of the IDM_ commands.			*/

void generic_init(void);
			/* OS independent initialization */


/*
 * Calls from de.c to de_win.c
 */
 
void move_cursor(int column, int line);
			/* Physically move the cursor on the display,	*/
			/* so that it appears at			*/
			/* (column, line).				*/

void invalidate_line(int line);
			/* Invalidate line i on the screen.	*/

void de_error(char *s);
			/* Display error message.	*/