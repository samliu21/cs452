
#ifndef _terminal_h_
#define _terminal_h_

void terminal_clear();
void terminal_set_cursor(int r, int c);
void terminal_set_cursor_origin();
void terminal_clear_line();
void terminal_save_cursor();
void terminal_restore_cursor();

#endif