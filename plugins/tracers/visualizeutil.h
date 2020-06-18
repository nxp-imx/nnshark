#include <gst/gst.h>
#include "gstliveunit.h"

void milsleep (int ms);

void *curses_loop (void *arg);
void print_line_absolute (int *row, int *col);
void print_line (int *row, int *col);
int box_char (int x);
void draw_arrow (int rows, int cols, int length);
void draw_box (int rows, int cols, int height, int width);
void draw_pad (gpointer key, gpointer value, gpointer user_data);
void draw_element (gpointer key, gpointer value, gpointer user_data);
void draw_all (ElementUnit * element, int start_row, int start_col,
    int width, int height, int ele_width, int arr_width);
void print_data (int key_in, Packet * packet);

G_BEGIN_DECLS
#define COL_SCALE 21
#define TIMESCALE 400
#define ELEMENT_NAME_MAX 20
    G_END_DECLS
