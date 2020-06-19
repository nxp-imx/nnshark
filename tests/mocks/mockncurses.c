#ifdef LINKTIME
#include <ncurses.h>

int __real_waddch (WINDOW * win, const chtype ch);
int __real_noecho (void);
int __real_wattr_on (WINDOW * win, int attrs);
int __real_mvprintw (int y, int x, char *fmt, ...);
int __real_getch (void);
int __real_wgetch (WINDOW * win);
int __real_endwin (void);
int __real_wattr_off (WINDOW * win, attr_t attrs, void *opts);
int __real_wrefresh (WINDOW * win);
int __real_wclear (WINDOW * win);
int __real_wmove (WINDOW * win, int y, int x);
int __real_keypad (WINDOW * win, bool bf);
int __real_curs_set (int visibility);
WINDOW *__real_initscr (void);
int __real_raw (void);
int __real_start_color (void);
int __real_init_pair (short pair, short f, short b);

int mvaddch_count = 0;
int mvprintw_count = 0;
int attron_count = 0;
int getch_count = 0;
int print_yloc = 0;
int print_xloc = 0;

int
__wrap_waddch (WINDOW * win, const chtype ch)
{
  mvaddch_count++;
  return 0;
}

int
__wrap_noecho (void)
{
  return 0;
}

int
__wrap_wattr_on (WINDOW * win, int attrs)
{
  attron_count++;
  return 0;
}

int
__wrap_mvprintw (int y, int x, char *fmt, ...)
{
  if (mvprintw_count == 0) {
    print_yloc = y;
    print_xloc = x;
  }
  mvprintw_count++;
  return 0;
}

int
__wrap_getch (void)
{
  return 0;
}

int
__wrap_wgetch (WINDOW * win)
{
  if ((getch_count) == 0) {
    getch_count = 1;
    return 259;
  } else if ((getch_count) == 1) {
    getch_count = 2;
    return 258;
  } else if ((getch_count) == 2) {
    getch_count = 3;
    return 261;
  } else if ((getch_count) == 3) {
    getch_count = 4;
    return 260;
  } else if ((getch_count) == 4) {
    getch_count = 5;
    return 32;
  } else if ((getch_count) == 5) {
    getch_count = 6;
    return '[';
  } else if ((getch_count) == 6) {
    getch_count = 7;
    return ']';
  } else if ((getch_count) == 7) {
    getch_count = 8;
    return 0;
  } else
    return 'q';
}

int
__wrap_endwin (void)
{
  return 0;
}

int
__wrap_wattr_off (WINDOW * win, attr_t attrs, void *opts)
{
  return 0;
}

int
__wrap_wrefresh (WINDOW * win)
{
  return 0;
}

int
__wrap_wclear (WINDOW * win)
{
  return 0;
}

int
__wrap_wmove (WINDOW * win, int y, int x)
{
  return 0;
}

int
__wrap_keypad (WINDOW * win, bool bf)
{
  return 0;
}

int
__wrap_curs_set (int visibility)
{
  return 0;
}

WINDOW *
__wrap_initscr (void)
{
  return NULL;
}

int
__wrap_raw (void)
{
  return 0;
}

int
__wrap_start_color (void)
{
  return 0;
}

int
__wrap_init_pair (short pair, short f, short b)
{
  return 0;
}

#endif
