#include <ncurses.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <assert.h>

#include "visualizeutil.h"
#include "gstliveprofiler.h"
#include "gstliveunit.h"
#include "gstctf.h"

void initialize (void);
void print_pad (gpointer key, gpointer value, gpointer user_data);

void
milsleep (int ms)
{
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep (&ts, NULL);
}

// for saving initial timestamp;
long initial_tv_sec;
long initial_tv_usec;

// NCurses location
int row_current = 0;
int col_current = 0;
int row_offset = 0;             //for scrolling

// Draw Location
int row_pad_sink = 0;
int row_pad_src = 0;
int arrow_width = 0;

// Draw all location
int element_row = 0;
int element_col = 0;
int element_width = 0;
int element_height = 0;

// Draw location
#define DRAW_START_ROW 1
#define DRAW_START_COL 110
#define DRAW_WIDTH 80
#define DRAW_HEIGHT 20
#define DRAW_ELE_WIDTH 30
#define DRAW_ARROW_WIDTH 20

// NCurses color scheme
#define INVERT_PAIR	1
#define TITLE_PAIR	2
#define SELECT_ELEMENT_PAIR 3
#define SELECT_PAD_PAIR 4
#define SELECT_PEER_PAD_PAIR 5

// View Mode
#define PAD_SELECTION 1
#define ELEMENT_SELECTION 0

#define NONE_SELECTED 0
#define PEER_SELECTED 1
#define SELF_SELECTED 2

GList *elementIterator = NULL;
GList *padIterator = NULL;
gchar *pairPad = NULL;
gchar *pairElement = NULL;

// For log
extern int metadata_writed;
int *cpu_log = NULL;
LogUnit *element_log = NULL;
int log_base_row = 0;

// Iterator for Hashtable
void
print_pad (gpointer key, gpointer value, gpointer user_data)
{
  gchar *name = (gchar *) key;
  PadUnit *data = (PadUnit *) value;
  gint8 *selected = (gint8 *) user_data;
  GstPad *pair;

  if (padIterator && *selected == SELF_SELECTED
      && strcmp (key, padIterator->data) == 0) {
    attron (A_BOLD);
    attron (COLOR_PAIR (SELECT_PAD_PAIR));
    mvprintw (row_offset + row_current, 4, "%s", name);
    attroff (A_BOLD);
    attroff (COLOR_PAIR (SELECT_PAD_PAIR));

    pair = gst_pad_get_peer (data->element);
    pairElement = GST_OBJECT_NAME (GST_OBJECT_PARENT (pair));
    pairPad = GST_OBJECT_NAME (pair);
  } else if (padIterator && *selected == PEER_SELECTED
      && strcmp (key, pairPad) == 0) {
    attron (A_BOLD);
    attron (COLOR_PAIR (SELECT_PEER_PAD_PAIR));
    mvprintw (row_offset + row_current, 4, "%s", name);
    attroff (A_BOLD);
    attroff (COLOR_PAIR (SELECT_PEER_PAD_PAIR));
  } else {
    mvprintw (row_offset + row_current, 4, "%s", name);
  }

  // check value for each element is changed
  if (g_getenv ("LOG_ENABLED") && element_log) {
    char element_log_text[100];
    int new_datarate = (int) (data->datarate * 100);
    int changed =
        new_datarate - element_log[row_current - log_base_row].bufrate;

    if (changed != 0) {
      sprintf (element_log_text, "p %d %d", row_current - log_base_row,
          changed);
      do_print_log ("log", element_log_text);
      element_log[row_current - log_base_row].bufrate = new_datarate;
    }
  }

  mvprintw (row_offset + row_current, ELEMENT_NAME_MAX * 4 + 2,
      "%20.2f", data->datarate);
  row_current++;
}

//print each element
void
print_element (gpointer key, gpointer value, gpointer user_data)
{
  gchar *name = (gchar *) key;
  ElementUnit *data = (ElementUnit *) value;
  gint8 *selected = g_malloc (sizeof (gboolean *));

  attron (A_BOLD);
  if (elementIterator && strcmp (key, elementIterator->data) == 0) {
    attron (COLOR_PAIR (SELECT_ELEMENT_PAIR));
    mvprintw (row_offset + row_current, 0, "%s", name);
    attroff (COLOR_PAIR (SELECT_ELEMENT_PAIR));
    *selected = SELF_SELECTED;
  } else if (pairElement && strcmp (key, pairElement) == 0) {
    mvprintw (row_offset + row_current, 0, "%s", name);
    *selected = PEER_SELECTED;
  } else {
    mvprintw (row_offset + row_current, 0, "%s", name);
    *selected = NONE_SELECTED;
  }
  attroff (A_BOLD);

  // check value for each element is changed
  if (g_getenv ("LOG_ENABLED") && element_log) {

    char element_log_text[100];
    char *buf = &element_log_text[0];
    if (element_log[row_current - log_base_row].proctime !=
        data->proctime->value) {
      buf +=
          sprintf (buf, "%ld ",
          data->proctime->value - element_log[row_current -
              log_base_row].proctime);
      element_log[row_current - log_base_row].proctime = data->proctime->value;
    } else
      buf += sprintf (buf, ". ");

    if (element_log[row_current - log_base_row].queue_level !=
        data->queue_level) {
      buf += sprintf (buf, "%d ", data->queue_level);
      element_log[row_current - log_base_row].queue_level = data->queue_level;
    } else
      buf += sprintf (buf, ". ");

    if (element_log[row_current - log_base_row].max_queue_level !=
        data->max_queue_level) {
      buf += sprintf (buf, "%d", data->max_queue_level);
      element_log[row_current - log_base_row].max_queue_level =
          data->max_queue_level;
    } else
      buf += sprintf (buf, ".");

    // mvprintw (row_offset + row_current, ELEMENT_NAME_MAX,
    //   "%10d %10d", row_current, data->elem_idx);

    if (strcmp (element_log_text, ". . .")) {
      char changed_data[100];
      sprintf (changed_data, "%d %s", row_current - log_base_row,
          element_log_text);

      do_print_log ("log", changed_data);
    }
  }

  mvprintw (row_offset + row_current, ELEMENT_NAME_MAX,
      "%20ld %20.3f %17d/%2d",
      data->proctime->value,
      data->proctime->avg, data->queue_level, data->max_queue_level);
  row_current++;

  g_hash_table_foreach (data->pad, (GHFunc) print_pad, selected);

  g_free (selected);
}

void
initialize (void)
{
  row_current = 0;
  col_current = 0;
  row_offset = 0;               //for scrolling
  initscr ();
  raw ();
  start_color ();
  init_pair (INVERT_PAIR, COLOR_BLACK, COLOR_WHITE);
  init_pair (TITLE_PAIR, COLOR_BLUE, COLOR_BLACK);
  init_pair (SELECT_ELEMENT_PAIR, COLOR_RED, COLOR_BLACK);
  init_pair (SELECT_PAD_PAIR, COLOR_YELLOW, COLOR_BLACK);
  init_pair (SELECT_PEER_PAD_PAIR, COLOR_CYAN, COLOR_BLACK);
  keypad (stdscr, TRUE);
  curs_set (0);
  noecho ();
}

void
print_line_absolute (int *row, int *col)
{
  for (int i = 0; i < 5; i++) {
    mvprintw (*row, *col + COL_SCALE * i, "---------------------");
  }

  (*row)++;
}

void
print_line (int *row, int *col)
{
  for (int i = 0; i < 5; i++) {
    mvprintw (row_offset + *row, *col + COL_SCALE * i, "---------------------");
  }

  (*row)++;
}

int
box_char (int x)
{
  switch (x) {
    case 1:
      return ACS_LLCORNER;
    case 2:
      return ACS_BTEE;
    case 3:
      return ACS_LRCORNER;
    case 4:
      return ACS_LTEE;
    case 5:
      return ACS_PLUS;
    case 6:
      return ACS_RTEE;
    case 7:
      return ACS_ULCORNER;
    case 8:
      return ACS_TTEE;
    case 9:
      return ACS_URCORNER;
    case 10:
      return ACS_HLINE;         // Horizontal Line      '-'
    case 11:
      return ACS_VLINE;         // Vertical Line        '|'
  }
  return 0;
}

void
draw_arrow (int rows, int cols, int length)
{
  for (int i = cols; i < cols + length; ++i) {
    mvaddch (rows, i, '-');
  }
  mvaddch (rows, cols + length, '>');
  return;
}

void
draw_box (int rows, int cols, int height, int width)
{
  /* TODO */
  //Draw Box whose left-top pos is (rows, cols) with height and width

  // for(int w= cols+1; w<cols+width; w++) {
  //      for(int h = rows+1 ; h<rows+height; h++) {
  //              mvaddch(h, w, ' ');
  //      }
  // }

  for (int w = cols + 1; w < cols + width; w++) {
    mvaddch (rows, w, box_char (10));
    mvaddch (rows + height, w, box_char (10));
  }
  for (int h = rows + 1; h < rows + height; h++) {
    mvaddch (h, cols, box_char (11));
    mvaddch (h, cols + width, box_char (11));
  }
  mvaddch (rows, cols, box_char (7));
  mvaddch (rows, cols + width, box_char (9));
  mvaddch (rows + height, cols, box_char (1));
  mvaddch (rows + height, cols + width, box_char (3));
  return;
}

void
draw_pad (gpointer key, gpointer value, gpointer user_data)
{
  int src_pad_info_col = element_col + element_width + 1;
  int sink_pad_info_col = element_col - arrow_width - 1;
  PadUnit *data = (PadUnit *) value;;

  if (key == NULL || value == NULL)
    return;

  if (padIterator && !strcmp (key, padIterator->data)) {
    attron (A_BOLD);
    attron (COLOR_PAIR (SELECT_PAD_PAIR));
  }
  if (gst_pad_get_direction (data->element) == GST_PAD_SRC) {
    draw_arrow (row_pad_src, src_pad_info_col, arrow_width);
    mvprintw (row_pad_src + 1, src_pad_info_col, "to %s",
        GST_OBJECT_NAME (GST_OBJECT_PARENT (gst_pad_get_peer (data->element))));
    mvprintw (row_pad_src + 2, src_pad_info_col, "bufrate: %10.2f",
        data->datarate);
    mvprintw (row_pad_src + 3, src_pad_info_col, "bufsize: %10ld",
        data->buffer_size->value);
    row_pad_src += 4;
  } else {
    draw_arrow (row_pad_sink, sink_pad_info_col, arrow_width);
    mvprintw (row_pad_sink + 1, sink_pad_info_col, "from %s",
        GST_OBJECT_NAME (GST_OBJECT_PARENT (gst_pad_get_peer (data->element))));
    mvprintw (row_pad_sink + 2, sink_pad_info_col, "bufrate: %10.2f",
        data->datarate);
    mvprintw (row_pad_sink + 3, sink_pad_info_col, "bufsize: %10ld",
        data->buffer_size->value);
    row_pad_sink += 4;
  }
  if (padIterator && !strcmp (key, padIterator->data)) {
    attroff (A_BOLD);
    attroff (COLOR_PAIR (SELECT_PAD_PAIR));
  }

  return;
}

void
draw_element (gpointer key, gpointer value, gpointer user_data)
{
  int element_info_row = element_row + element_height / 2;
  int element_info_col = element_col + element_width / 2 - 10;
  gchar *name = (gchar *) key;
  ElementUnit *data = (ElementUnit *) value;

  if (key == NULL || value == NULL)
    return;

  draw_box (element_row, element_col, element_height, element_width);
  attron (COLOR_PAIR (SELECT_ELEMENT_PAIR));
  attron (A_BOLD);
  mvprintw (element_info_row - 3, element_info_col, "%s", name);
  attroff (COLOR_PAIR (SELECT_ELEMENT_PAIR));
  attroff (A_BOLD);

  mvprintw (element_info_row, element_info_col, "%s: %ld", "Proctime",
      data->proctime->value);
  mvprintw (element_info_row + 1, element_info_col, "%s: %f", "Average",
      data->proctime->avg);
  mvprintw (element_info_row + 2, element_info_col, "%s: %d/%d",
      "Queue_level", data->queue_level, data->max_queue_level);

  row_pad_src = element_row + 1;
  row_pad_sink = element_row + 1;
  g_hash_table_foreach (data->pad, (GHFunc) draw_pad, NULL);

  return;
}

void
print_data (int key_in, Packet * packet)
{
  time_t tmp_t = time (NULL);
  struct tm tm = *localtime (&tmp_t);
  int cpu_counter;
  int gpu_counter;
  int ddr_counter;
  int pwr_counter;
  int row_current_tmp;
  // draw
  clear ();
  mvprintw (row_offset + row_current, 36, "key");       //for debug
  mvprintw (row_offset + row_current, 40, "%08d", key_in);      //for debug
  attron (A_BOLD);
  mvprintw (row_offset + row_current, 63, "%4d-%02d-%02d %2d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);     //time indicator
  attron (COLOR_PAIR (INVERT_PAIR));
  mvprintw (row_offset + row_current++, col_current,
      "Press 'q' or 'Q' to quit");
  attroff (A_BOLD);
  attroff (COLOR_PAIR (INVERT_PAIR));
  print_line (&row_current, &col_current);

  if (g_getenv ("LOG_ENABLED")) {
    do_print_log ("log", "t");
  }
  //CPU Usage
  //
#define _CPU_HEADER_OFFSET 0
#define _GPU_HEADER_OFFSET 16
#define _DDR_HEADER_OFFSET 32
#define _PWR_HEADER_OFFSET 64

#define _CPU_STATS_OFFSET (_CPU_HEADER_OFFSET + 8)
#define _GPU_STATS_OFFSET (_GPU_HEADER_OFFSET + 8)
#define _DDR_STATS_OFFSET (_DDR_HEADER_OFFSET + 16)
#define _PWR_STATS_OFFSET (_PWR_HEADER_OFFSET + 16)

  attron (A_BOLD);
  attron (COLOR_PAIR (TITLE_PAIR));
  mvprintw (row_offset + row_current, col_current + _CPU_HEADER_OFFSET,
      "CPU Usage");
  mvprintw (row_offset + row_current, col_current + _GPU_HEADER_OFFSET,
      "GPU Usage");
  mvprintw (row_offset + row_current, col_current + _DDR_HEADER_OFFSET,
      "DDR Usage");
  mvprintw (row_offset + row_current++, col_current + _PWR_HEADER_OFFSET,
      "PWR Measurement");
  attroff (A_BOLD);
  attroff (COLOR_PAIR (TITLE_PAIR));

  row_current_tmp = row_current;
  cpu_counter = 0;
  gpu_counter = 0;
  ddr_counter = 0;
  pwr_counter = 0;
  while (cpu_counter < packet->cpu_num) {
    attron (A_BOLD);
    mvprintw (row_offset + row_current_tmp, col_current, "CPU%2d", cpu_counter);
    attroff (A_BOLD);
    mvprintw (row_offset + row_current_tmp++, col_current + _CPU_STATS_OFFSET,
        "%3.1f%%", packet->cpu_load[cpu_counter]);
    cpu_counter++;
  }

  row_current_tmp = row_current;
  while (gpu_counter < packet->gpu_num) {
    attron (A_BOLD);
    mvprintw (row_offset + row_current_tmp, col_current + _GPU_HEADER_OFFSET,
        "%s", packet->gpu_name[gpu_counter]);
    attroff (A_BOLD);
    mvprintw (row_offset + row_current_tmp++, col_current + _GPU_STATS_OFFSET,
        "%3.1f%%", packet->gpu_load[gpu_counter]);
    gpu_counter++;
  }

  row_current_tmp = row_current;
  while (ddr_counter < packet->ddr_num) {
    attron (A_BOLD);
    mvprintw (row_offset + row_current_tmp, col_current + _DDR_HEADER_OFFSET,
        "%s", packet->ddr_name[ddr_counter]);
    attroff (A_BOLD);
    mvprintw (row_offset + row_current_tmp++, col_current + _DDR_STATS_OFFSET,
        "%8.2f MB/s", packet->ddr_load[ddr_counter]);
    ddr_counter++;
  }

  row_current_tmp = row_current;
  while (pwr_counter < packet->pwr_num) {
    attron (A_BOLD);
    mvprintw (row_offset + row_current_tmp, col_current + _PWR_HEADER_OFFSET,
        "%s", packet->pwr_name[pwr_counter]);
    attroff (A_BOLD);
    mvprintw (row_offset + row_current_tmp++, col_current + _PWR_STATS_OFFSET,
        "%8.2f mW", packet->pwr_value[pwr_counter]);
    pwr_counter++;
  }

  row_current += MAX (packet->pwr_num,
      MAX (packet->ddr_num, MAX (packet->gpu_num, packet->cpu_num)));

  if (g_getenv ("LOG_ENABLED") && cpu_log
      && cpu_log[0] != (int) (packet->cpu_load[0] * 10)) {
    char cpuusage_text[100];
    char *buf = &cpuusage_text[0];
    buf += sprintf (buf, "c ");
    for (cpu_counter = 0; cpu_counter < packet->cpu_num; cpu_counter++) {
      cpu_log[cpu_counter] = packet->cpu_load[cpu_counter] * 10;
      buf += sprintf (buf, "%d ", cpu_log[cpu_counter]);
    }
    do_print_log ("log", cpuusage_text);
  }

  print_line (&row_current, &col_current);

  // Proctime & Bufferrate
  attron (A_BOLD);
  attron (COLOR_PAIR (TITLE_PAIR));
  mvprintw (row_offset + row_current, 0, "ElementName");
  mvprintw (row_offset + row_current++, ELEMENT_NAME_MAX,
      "%20s %20s %20s %20s", "Proctime(ns)", "Avg_proctime(ns)",
      "queuelevel", "Bufferrate(bps)");
  attroff (COLOR_PAIR (TITLE_PAIR));
  attroff (A_BOLD);
}

void
draw_all (ElementUnit * element, int start_row, int start_col, int width,
    int height, int ele_width, int arr_width)
{
  assert (element_width + 2 * arrow_width < width);
  draw_box (start_row, start_col, height, width);
  element_row = start_row + 2;
  element_height = height - 4;
  element_col = start_col + width / 2 - element_width / 2;
  element_width = ele_width;
  arrow_width = arr_width;
  if (elementIterator)
    draw_element (elementIterator->data, element, NULL);

}

void *
curses_loop (void *arg)
{
  Packet *packet = (Packet *) arg;
  struct timeval startTime;     //for getting time
  int key_in;
  int iter = 0;
  gboolean selection_mode = ELEMENT_SELECTION;
  ElementUnit *element = NULL;
  GList *element_key = NULL;
  GList *pad_key = NULL;
  char *text;

  while (!packet->loaded)
    milsleep (1);

  initialize ();

  if (g_getenv ("LOG_ENABLED")) {
    log_base_row = packet->cpu_num + 5;
    gettimeofday (&startTime, NULL);
    text = (char *) g_malloc (30 * sizeof (char));
    sprintf (text, "%ld.%ld", startTime.tv_sec, startTime.tv_usec / 1000);
    do_print_log ("log_metadata", text);
    initial_tv_sec = startTime.tv_sec;
    initial_tv_usec = startTime.tv_usec;
  }

  while (1) {
    if (element_key == NULL) {
      element_key = g_hash_table_get_keys (packet->elements);
      elementIterator = g_list_last (element_key);
      if (elementIterator)
        element = g_hash_table_lookup (packet->elements, elementIterator->data);
    }

    if (element_log == NULL && metadata_writed) {
      cpu_log = (int *) malloc (sizeof (int) * packet->cpu_num);
      element_log = (LogUnit *) malloc (sizeof (LogUnit) * metadata_writed);
    }

    row_current = 0;
    col_current = 0;

    timeout (0);
    key_in = getch ();
    //key binding
    if (key_in == 'q' || key_in == 'Q')
      break;

    switch (key_in) {
      case 259:                //arrow right
        if (selection_mode == PAD_SELECTION) {
          if (g_list_next (padIterator)) {
            padIterator = g_list_next (padIterator);
          }
        } else if (selection_mode == ELEMENT_SELECTION) {
          if (g_list_next (elementIterator)) {
            elementIterator = g_list_next (elementIterator);
            if (elementIterator)
              element =
                  g_hash_table_lookup (packet->elements, elementIterator->data);
          }
        }

        break;
      case 258:                //arrow left
        if (selection_mode == PAD_SELECTION) {
          if (g_list_previous (padIterator)) {
            padIterator = g_list_previous (padIterator);
          }
        } else if (selection_mode == ELEMENT_SELECTION) {
          if (g_list_previous (elementIterator)) {
            elementIterator = g_list_previous (elementIterator);
            if (elementIterator)
              element =
                  g_hash_table_lookup (packet->elements, elementIterator->data);
          }
        }
        break;
      case 261:                //arrow right
        selection_mode = PAD_SELECTION;
        element = g_hash_table_lookup (packet->elements, elementIterator->data);
        pad_key = g_hash_table_get_keys (element->pad);
        padIterator = g_list_last (pad_key);
        break;
      case 260:                //arrow left
        selection_mode = ELEMENT_SELECTION;
        pad_key = NULL;
        padIterator = NULL;
        pairPad = NULL;
        pairElement = NULL;
        break;
      case 32:
        if (element_key && pad_key) {
          elementIterator = g_list_first (element_key);
          while (g_list_next (elementIterator)) {
            if (strcmp (elementIterator->data, pairElement) == 0)
              break;
            elementIterator = g_list_next (elementIterator);
          }
          element =
              g_hash_table_lookup (packet->elements, elementIterator->data);
          pad_key = g_hash_table_get_keys (element->pad);
          padIterator = g_list_first (pad_key);
          while (g_list_next (padIterator)) {
            if (strcmp (padIterator->data, pairPad) == 0)
              break;
            padIterator = g_list_next (padIterator);
          }
        }
        break;
      case '[':                //arrow up
        if (row_offset < 0)
          row_offset++;
        break;
      case ']':                //arrow down
        row_offset--;
        break;
      default:
        break;
    }

    print_data (key_in, packet);

    if (packet->loaded)
      g_hash_table_foreach (packet->elements, (GHFunc) print_element, NULL);

    draw_all (element, DRAW_START_ROW, DRAW_START_COL, DRAW_WIDTH,
        DRAW_HEIGHT, DRAW_ELE_WIDTH, DRAW_ARROW_WIDTH);

    iter++;
    refresh ();
    milsleep (TIMESCALE / 4);

  }
  endwin ();
  return NULL;
}
