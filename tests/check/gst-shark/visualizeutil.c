#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <gst/check/gstcheck.h>
#include <string.h>

#include <stdio.h>
#include "visualizeutil.h"
#include "gstliveunit.h"
#include "gstliveprofiler.h"

extern int mvaddch_count;
extern int mvprintw_count;
extern int getch_count;
extern GList *elementIterator;
extern GList *padIterator;
extern int attron_count;
extern int element_row;
extern int element_col;
extern int element_width;
extern int element_height;
extern int arrow_width;
extern int row_current;
extern int print_yloc;
extern int print_xloc;
extern int row_pad_src;
extern int row_pad_sink;
extern gchar *pairPad;
extern gchar *pairElement;
extern LogUnit *element_log;
extern gchar *pairElement;
extern Packet *packet;

GST_START_TEST (test_draw_box)
{
  mvaddch_count = 0;
  draw_box (3, 6, 5, 6);
  ck_assert (mvaddch_count == 22);

}

GST_END_TEST;

GST_START_TEST (test_draw_arrow)
{
  mvaddch_count = 0;
  draw_arrow (0, 0, 10);
  ck_assert (mvaddch_count == 11);
}

GST_END_TEST;

GST_START_TEST (test_draw_element_null)
{
  mvaddch_count = 0;
  mvprintw_count = 0;
  draw_element (NULL, NULL, NULL);
  ck_assert (mvprintw_count == 0);
  ck_assert (mvaddch_count == 0);
}

GST_END_TEST;

GST_START_TEST (test_draw_element)
{
  mvaddch_count = 0;
  mvprintw_count = 0;
  ElementUnit *element;
  GError *e = NULL;

  element_row = 3;
  element_col = 6;
  element_height = 5;
  element_width = 6;

  element = element_unit_new (gst_parse_launch ("fakesrc ! fakesink", &e));
  element->pad = g_hash_table_new (g_str_hash, g_str_equal);
  element->proctime = avg_unit_new ();
  elementIterator = g_list_append (elementIterator, "asdf");
  elementIterator = g_list_first (elementIterator);
  draw_element (elementIterator->data, element, NULL);
  ck_assert (mvprintw_count == 4);
  ck_assert (mvaddch_count == 22);
}

GST_END_TEST;

GST_START_TEST (test_print_element)
{
  mvprintw_count = 0;
  attron_count = 0;
  ElementUnit *element;
  GError *e = NULL;
  element_log = (LogUnit *) malloc (sizeof (LogUnit) * 100);
  g_setenv ("LOG_ENABLED", "TRUE", TRUE);

  element = element_unit_new (gst_parse_launch ("fakesrc ! fakesink", &e));
  element->pad = g_hash_table_new (g_str_hash, g_str_equal);
  element->proctime = avg_unit_new ();
  elementIterator = g_list_append (elementIterator, "asdf");
  elementIterator = g_list_first (elementIterator);
  print_element (elementIterator->data, element, NULL);
  ck_assert (mvprintw_count == 2);
  ck_assert (attron_count == 2);

  mvprintw_count = 0;
  attron_count = 0;
  elementIterator = g_list_first (elementIterator);
  print_element ("diff", element, NULL);
  ck_assert (mvprintw_count == 2);
  ck_assert (attron_count == 1);

  mvprintw_count = 0;
  attron_count = 0;
  pairElement = "pair";
  elementIterator = g_list_first (elementIterator);
  print_element ("pair", element, NULL);
  ck_assert (mvprintw_count == 2);
  ck_assert (attron_count == 1);
}

GST_END_TEST;

GST_START_TEST (test_draw_pad_null)
{
  mvaddch_count = 0;
  mvprintw_count = 0;
  draw_pad (NULL, NULL, NULL);
  ck_assert (mvprintw_count == 0);
  ck_assert (mvaddch_count == 0);
}

GST_END_TEST;

GST_START_TEST (test_draw_pad)
{
  mvaddch_count = 0;
  mvprintw_count = 0;
  attron_count = 0;
  arrow_width = 10;
  element_col = 21;
  row_pad_src = 21;
  row_pad_sink = 35;

  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! fakesink", &e);
  update_pipeline_init ((GstPipeline *) pipe);
  padIterator = g_list_append (padIterator, "src");

  ElementUnit *unit = g_hash_table_lookup (packet->elements, "fakesrc0");
  PadUnit *pad = g_hash_table_lookup (unit->pad, "src");
  padIterator = g_list_first (padIterator);
  draw_pad ("src", pad, NULL);
  ck_assert (mvaddch_count == 11);
  ck_assert (mvprintw_count == 3);
  ck_assert (attron_count == 2);
  ck_assert (print_yloc == 22);
  ck_assert (print_xloc == 22);
  ck_assert (row_pad_src == 25);

  mvaddch_count = 0;
  mvprintw_count = 0;
  attron_count = 0;
  unit = g_hash_table_lookup (packet->elements, "fakesink0");
  pad = g_hash_table_lookup (unit->pad, "sink");
  padIterator = g_list_append (padIterator, "sink");
  padIterator = g_list_first (padIterator);
  padIterator = g_list_next (padIterator);
  draw_pad ("sink", pad, NULL);
  ck_assert (mvaddch_count == 11);
  ck_assert (mvprintw_count == 3);
  ck_assert (attron_count == 2);
  ck_assert (print_yloc == 36 && print_xloc == 10);
  ck_assert (row_pad_sink == 39);

  mvaddch_count = 0;
  mvprintw_count = 0;
  attron_count = 0;
  padIterator = g_list_first (padIterator);
  draw_pad ("diff", pad, NULL);
  ck_assert (mvaddch_count == 11);
  ck_assert (mvprintw_count == 3);
  ck_assert (attron_count == 0);
}

GST_END_TEST;

GST_START_TEST (test_print_pad)
{
  mvaddch_count = 0;
  mvprintw_count = 0;
  attron_count = 0;
  arrow_width = 10;
  element_col = 21;
  row_pad_src = 21;
  row_pad_sink = 35;
  gint8 selected = 2;
  row_current = 14;

  element_log = (LogUnit *) malloc (sizeof (LogUnit) * 100);
  g_setenv ("LOG_ENABLED", "TRUE", TRUE);

  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! fakesink", &e);
  update_pipeline_init ((GstPipeline *) pipe);
  padIterator = g_list_append (padIterator, "src");

  ElementUnit *unit = g_hash_table_lookup (packet->elements, "fakesrc0");
  PadUnit *pad = g_hash_table_lookup (unit->pad, "src");
  padIterator = g_list_first (padIterator);
  print_pad ("src", pad, &selected);
  ck_assert (mvprintw_count == 2);
  ck_assert (attron_count == 2);
  assert_equals_string (pairPad, "sink");
  assert_equals_string (pairElement, "fakesink0");

  mvaddch_count = 0;
  mvprintw_count = 0;
  attron_count = 0;
  selected = 1;
  unit = g_hash_table_lookup (packet->elements, "fakesink0");
  pad = g_hash_table_lookup (unit->pad, "sink");
  padIterator = g_list_append (padIterator, "sink");
  padIterator = g_list_first (padIterator);
  padIterator = g_list_next (padIterator);
  print_pad ("sink", pad, &selected);
  ck_assert (mvprintw_count == 2);
  ck_assert (attron_count == 2);

  mvaddch_count = 0;
  mvprintw_count = 0;
  attron_count = 0;
  selected = 0;
  padIterator = g_list_first (padIterator);
  padIterator = g_list_next (padIterator);
  print_pad ("src", pad, &selected);
  ck_assert (mvprintw_count == 2);
  ck_assert (attron_count == 0);
}

GST_END_TEST;

GST_START_TEST (test_curses_loop)
{
  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! fakesink", &e);
  update_pipeline_init ((GstPipeline *) pipe);
  getch_count = 0;
  curses_loop (packet);
  ck_assert (mvaddch_count > 0);
}

GST_END_TEST;



static Suite *
visualizeutil_suite (void)
{
  Suite *s = suite_create ("VisualizeUtil");
  TCase *tc = tcase_create ("/tracers/graphics/visualizeutl");

  suite_add_tcase (s, tc);
  tcase_add_test (tc, test_draw_box);
  tcase_add_test (tc, test_draw_arrow);
  tcase_add_test (tc, test_draw_element_null);
  tcase_add_test (tc, test_draw_element);
  tcase_add_test (tc, test_print_element);
  tcase_add_test (tc, test_draw_pad_null);
  tcase_add_test (tc, test_curses_loop);
  tcase_add_test (tc, test_draw_pad);
  tcase_add_test (tc, test_print_pad);

  return s;
}

GST_CHECK_MAIN (visualizeutil)
