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
/* Boehm, February 6, 1995 12:29 pm PST */

/*
 * The MS Windows specific part of de.  
 * This started as the generic Windows application template
 * made available by Rob Haack (rhaack@polaris.unm.edu), but
 * significant parts didn't survive to the final version.
 *
 * This was written by a nonexpert windows programmer.
 */


#include "windows.h"
#include "gc.h"
#include "cord.h"
#include "de_cmds.h"
#include "de_win.h"

int LINES = 0;
int COLS = 0;

char       szAppName[]     = "DE";
char       FullAppName[]   = "Demonstration Editor";

HWND        hwnd;

void de_error(char *s)
{
    MessageBox( hwnd, (LPSTR) s,
                (LPSTR) FullAppName,
                MB_ICONINFORMATION | MB_OK );
    InvalidateRect(hwnd, NULL, TRUE);
}

int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPSTR command_line, int nCmdShow)
{
   MSG         msg;
   WNDCLASS    wndclass;
   HANDLE      hAccel;

#  ifdef THREAD_LOCAL_ALLOC
     GC_INIT();  /* Required if GC is built with THREAD_LOCAL_ALLOC 	*/
     		 /* Always safe, but this is used as a GC test.		*/
#  endif

   if (!hPrevInstance)
   {
      wndclass.style          = CS_HREDRAW | CS_VREDRAW;
      wndclass.lpfnWndProc    = WndProc;
      wndclass.cbClsExtra     = 0;
      wndclass.cbWndExtra     = DLGWINDOWEXTRA;
      wndclass.hInstance      = hInstance;
      wndclass.hIcon          = LoadIcon (hInstance, szAppName);
      wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW);
      wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
      wndclass.lpszMenuName   = "DE";
      wndclass.lpszClassName  = szAppName;

      if (RegisterClass (&wndclass) == 0) {
          char buf[50];
   	
   	  sprintf(buf, "RegisterClass: error code: 0x%X", GetLastError());
   	  de_error(buf);
   	  return(0);
      }
   }
   
   /* Empirically, the command line does not include the command name ...
   if (command_line != 0) {
       while (isspace(*command_line)) command_line++;
       while (*command_line != 0 && !isspace(*command_line)) command_line++;
       while (isspace(*command_line)) command_line++;
   } */
   
   if (command_line == 0 || *command_line == 0) {
        de_error("File name argument required");
        return( 0 );
   } else {
        char *p = command_line;
        
        while (*p != 0 && !isspace(*p)) p++;
   	arg_file_name = CORD_to_char_star(
   			    CORD_substr(command_line, 0, p - command_line));
   }

   hwnd = CreateWindow (szAppName,
   			FullAppName,
   			WS_OVERLAPPEDWINDOW | WS_CAPTION, /* Window style */
   			CW_USEDEFAULT, 0, /* default pos. */
   			CW_USEDEFAULT, 0, /* default width, height */
   			NULL,	/* No parent */
   			NULL, 	/* Window class menu */
   			hInstance, NULL);
   if (hwnd == NULL) {
   	char buf[50];
   	
   	sprintf(buf, "CreateWindow: error code: 0x%X", GetLastError());
   	de_error(buf);
   	return(0);
   }

   ShowWindow (hwnd, nCmdShow);

   hAccel = LoadAccelerators( hInstance, szAppName );
   
   while (GetMessage (&msg, NULL, 0, 0))
   {
      if( !TranslateAccelerator( hwnd, hAccel, &msg ) )
      {
         TranslateMessage (&msg);
         DispatchMessage (&msg);
      }
   }
   return msg.wParam;
}

/* Return the argument with all control characters replaced by blanks.	*/
char * plain_chars(char * text, size_t len)
{
    char * result = GC_MALLOC_ATOMIC(len + 1);
    register size_t i;
    
    for (i = 0; i < len; i++) {
       if (iscntrl(text[i])) {
           result[i] = ' ';
       } else {
           result[i] = text[i];
       }
    }
    result[len] = '\0';
    return(result);
}

/* Return the argument with all non-control-characters replaced by 	*/
/* blank, and all control characters c replaced by c + 32.		*/
char * control_chars(char * text, size_t len)
{
    char * result = GC_MALLOC_ATOMIC(len + 1);
    register size_t i;
    
    for (i = 0; i < len; i++) {
       if (iscntrl(text[i])) {
           result[i] = text[i] + 0x40;
       } else {
           result[i] = ' ';
       }
    }
    result[len] = '\0';
    return(result);
}

int char_width;
int char_height;

void get_line_rect(int line, int win_width, RECT * rectp)
{
    rectp -> top = line * char_height;
    rectp -> bottom = rectp->top + char_height;
    rectp -> left = 0;
    rectp -> right = win_width;
}

int caret_visible = 0;	/* Caret is currently visible.	*/

int screen_was_painted = 0;/* Screen has been painted at least once.	*/

void update_cursor(void);

INT_PTR CALLBACK AboutBoxCallback( HWND hDlg, UINT message,
                           WPARAM wParam, LPARAM lParam )
{
   switch( message )
   {
      case WM_INITDIALOG:
           SetFocus( GetDlgItem( hDlg, IDOK ) );
           break;

      case WM_COMMAND:
           switch( wParam )
           {
              case IDOK:
                   EndDialog( hDlg, TRUE );
                   break;
           }
           break;

      case WM_CLOSE:
           EndDialog( hDlg, TRUE );
           return TRUE;

   }
   return FALSE;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message,
                          WPARAM wParam, LPARAM lParam)
{
   static HANDLE  hInstance;
   HDC dc;
   PAINTSTRUCT ps;
   RECT client_area;
   RECT this_line;
   RECT dummy;
   TEXTMETRIC tm;
   register int i;
   int id;

   switch (message)
   {
      case WM_CREATE:
           hInstance = ( (LPCREATESTRUCT) lParam)->hInstance;
           dc = GetDC(hwnd);
           SelectObject(dc, GetStockObject(SYSTEM_FIXED_FONT));
           GetTextMetrics(dc, &tm);
           ReleaseDC(hwnd, dc);
           char_width = tm.tmAveCharWidth;
           char_height = tm.tmHeight + tm.tmExternalLeading;
           GetClientRect(hwnd, &client_area);
      	   COLS = (client_area.right - client_area.left)/char_width;
      	   LINES = (client_area.bottom - client_area.top)/char_height;
      	   generic_init();
           return(0);

      case WM_CHAR:
      	   if (wParam == QUIT) {
      	       SendMessage( hwnd, WM_CLOSE, 0, 0L );
      	   } else {
      	       do_command((int)wParam);
      	   }
      	   return(0);
      
      case WM_SETFOCUS:
      	   CreateCaret(hwnd, NULL, char_width, char_height);
      	   ShowCaret(hwnd);
      	   caret_visible = 1;
      	   update_cursor();
      	   return(0);
      	   
      case WM_KILLFOCUS:
      	   HideCaret(hwnd);
      	   DestroyCaret();
      	   caret_visible = 0;
      	   return(0);
      	   
      case WM_LBUTTONUP:
      	   {
      	       unsigned xpos = LOWORD(lParam);	/* From left	*/
      	       unsigned ypos = HIWORD(lParam);	/* from top */
      	       
      	       set_position( xpos/char_width, ypos/char_height );
      	       return(0);
      	   }
      	   
      case WM_COMMAND:
      	   id = LOWORD(wParam);
      	   if (id & EDIT_CMD_FLAG) {
               if (id & REPEAT_FLAG) do_command(REPEAT);
               do_command(CHAR_CMD(id));
               return( 0 );
           } else {
             switch(id) {
               case IDM_FILEEXIT:
                  SendMessage( hwnd, WM_CLOSE, 0, 0L );
                  return( 0 );

               case IDM_HELPABOUT:
                  if( DialogBox( hInstance, "ABOUTBOX",
                                 hwnd, AboutBoxCallback ) )
                     InvalidateRect( hwnd, NULL, TRUE );
                  return( 0 );
	       case IDM_HELPCONTENTS:
	     	  de_error(
	     	       "Cursor keys: ^B(left) ^F(right) ^P(up) ^N(down)\n"
	     	       "Undo: ^U    Write: ^W   Quit:^D  Repeat count: ^R[n]\n"
	     	       "Top: ^T   Locate (search, find): ^L text ^L\n");
	     	  return( 0 );
	     }
	   }
           break;

      case WM_CLOSE:
           DestroyWindow( hwnd );
           return 0;

      case WM_DESTROY:
           PostQuitMessage (0);
	   GC_win32_free_heap();
           return 0;
      
      case WM_PAINT:
      	   dc = BeginPaint(hwnd, &ps);
      	   GetClientRect(hwnd, &client_area);
      	   COLS = (client_area.right - client_area.left)/char_width;
      	   LINES = (client_area.bottom - client_area.top)/char_height;
      	   SelectObject(dc, GetStockObject(SYSTEM_FIXED_FONT));
      	   for (i = 0; i < LINES; i++) {
      	       get_line_rect(i, client_area.right, &this_line);
      	       if (IntersectRect(&dummy, &this_line, &ps.rcPaint)) {
      	           CORD raw_line = retrieve_screen_line(i);
      	           size_t len = CORD_len(raw_line);
      	           char * text = CORD_to_char_star(raw_line);
      	           		/* May contain embedded NULLs	*/
      	           char * plain = plain_chars(text, len);
      	           char * blanks = CORD_to_char_star(CORD_chars(' ',
      	           				                COLS - len));
      	           char * control = control_chars(text, len);
#		   define RED RGB(255,0,0)
      	           
      	           SetBkMode(dc, OPAQUE);
      	           SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
      	           
      	           TextOut(dc, this_line.left, this_line.top,
      	           	   plain, (int)len);
      	           TextOut(dc, this_line.left + (int)len * char_width,
		   	   this_line.top,
      	           	   blanks, (int)(COLS - len));
      	           SetBkMode(dc, TRANSPARENT);
      	           SetTextColor(dc, RED);
      	           TextOut(dc, this_line.left, this_line.top,
      	           	   control, (int)strlen(control));
      	       }
      	   }
      	   EndPaint(hwnd, &ps);
      	   screen_was_painted = 1;
      	   return 0;
   }
   return DefWindowProc (hwnd, message, wParam, lParam);
}

int last_col;
int last_line;

void move_cursor(int c, int l)
{
    last_col = c;
    last_line = l;
    
    if (caret_visible) update_cursor();
}

void update_cursor(void)
{
    SetCaretPos(last_col * char_width, last_line * char_height);
    ShowCaret(hwnd);
}

void invalidate_line(int i)
{
    RECT line;
    
    if (!screen_was_painted) return;
    	/* Invalidating a rectangle before painting seems result in a	*/
    	/* major performance problem.					*/
    get_line_rect(i, COLS*char_width, &line);
    InvalidateRect(hwnd, &line, FALSE);
}

