/* GstShark - A Front End for GstTracer
 * Copyright (C) 2016 RidgeRun Engineering <michael.gruner@ridgerun.com>
 *
 * This file is part of GstShark.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
//#include "gstliveunit.h"
#include <stdio.h>
#include <gst/gst.h>
#include <gst/check/gstcheck.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "glib.h"
#include "gstliveprofiler.h"
#include "gstliveunit.h"

extern Packet *packet;
extern int log_idx = 0;
extern int metadata_writed = 0;

void
check_pad (gpointer key, gpointer value, gpointer user_data)
{
  ElementUnit *unit = (ElementUnit *) value;
  gchar *name;
  g_object_get (G_OBJECT (unit->element), "name", &name, NULL);
}

GST_START_TEST (test_liveprofiler_init)
{
  gint cpu_num;
  ck_assert (!packet);
  cpu_num = sysconf (_SC_NPROCESSORS_ONLN);
  ck_assert (gst_liveprofiler_init ());
  ck_assert (packet->cpu_num == cpu_num);
  ck_assert (gst_liveprofiler_finalize ());
}

GST_END_TEST;

GST_START_TEST (test_update_cpuusage)
{
  gint cpu_num;
  gfloat *a;
  ck_assert (!packet);
  ck_assert (gst_liveprofiler_init ());
  cpu_num = packet->cpu_num;
  a = g_malloc (cpu_num * sizeof (gfloat));
  gfloat *cpu_load = packet->cpu_load;

  int i;
  for (i = 0; i < cpu_num; i++) {
    a[i] = (gfloat) i;
  }
  update_cpuusage_event (cpu_num, a);
  for (i = 0; i < cpu_num; i++) {
    assert_equals_float (a[i], (gfloat) i);
  }

  ck_assert (gst_liveprofiler_finalize ());
  g_free (a);
}

GST_END_TEST;

GST_START_TEST (test_add_children)
{
  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! fakesink", &e);
  add_children_recursively (pipe, packet->elements);
  ck_assert (g_hash_table_size (packet->elements) == 2);
  ElementUnit *unit = g_hash_table_lookup (packet->elements, "fakesrc0");
  ck_assert (unit);
  PadUnit *pad_src = g_hash_table_lookup (unit->pad, "src");
  ck_assert (pad_src);

  unit = g_hash_table_lookup (packet->elements, "fakesink0");
  ck_assert (unit);
  PadUnit *pad_sink = g_hash_table_lookup (unit->pad, "sink");
  ck_assert (pad_sink);
  ck_assert (pad_unit_peer (packet->elements, pad_sink) == pad_src);

  unit = pad_unit_parent (packet->elements, pad_sink);

  g_object_get (G_OBJECT (unit->element), "name", &name, NULL);

  assert_equals_string (name, "fakesink0");


  ck_assert (gst_liveprofiler_finalize ());

}

GST_END_TEST;

GST_START_TEST (test_update_proctime)
{
  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  ElementUnit *element1, *element2, *element3;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! identity ! fakesink", &e);
  add_children_recursively (pipe, packet->elements);

  element1 = g_hash_table_lookup (packet->elements, "fakesrc0");
  element2 = g_hash_table_lookup (packet->elements, "identity0");
  element3 = g_hash_table_lookup (packet->elements, "fakesink0");
  update_proctime (element1, element2, 60, 1);
  update_proctime (element2, element3, 70, 1);
  ck_assert (element2->proctime->avg == 10);
}

GST_END_TEST;

GST_START_TEST (test_update_datatrate)
{
  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! fakesink", &e);
  add_children_recursively (pipe, packet->elements);

  ElementUnit *unit = g_hash_table_lookup (packet->elements, "fakesrc0");
  PadUnit *pad_src = g_hash_table_lookup (unit->pad, "src");
  unit = g_hash_table_lookup (packet->elements, "fakesink0");
  PadUnit *pad_sink = g_hash_table_lookup (unit->pad, "sink");
  ck_assert (g_queue_get_length (pad_src->time_log) == 0);
  update_datatrate (pad_src, pad_sink, 0);
  ck_assert (g_queue_get_length (pad_src->time_log) == 1);
  ck_assert (pad_src->datarate == 0 && pad_sink->datarate == 0);

  update_datatrate (pad_src, pad_sink, 1);
  ck_assert (g_queue_get_length (pad_src->time_log) == 2);
  ck_assert (pad_src->datarate == 1e9 && pad_sink->datarate == 1e9);

  int i;
  for (i = 2; i <= 10; i++)
    update_datatrate (pad_src, pad_sink, i);

  ck_assert (g_queue_get_length (pad_src->time_log) == 11);
  update_datatrate (pad_src, pad_sink, 110);
  ck_assert (g_queue_get_length (pad_src->time_log) == 11);
  ck_assert (pad_src->datarate == 1e8 && pad_sink->datarate == 1e8);

}

GST_END_TEST;

GST_START_TEST (test_update_buffer_size)
{
  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! fakesink", &e);
  add_children_recursively (pipe, packet->elements);

  ElementUnit *unit = g_hash_table_lookup (packet->elements, "fakesrc0");
  PadUnit *pad_src = g_hash_table_lookup (unit->pad, "src");
  unit = g_hash_table_lookup (packet->elements, "fakesink0");
  PadUnit *pad_sink = g_hash_table_lookup (unit->pad, "sink");

  update_buffer_size (pad_src, pad_sink, 7);
  ck_assert (pad_src->buffer_size->avg == 7 && pad_sink->buffer_size->avg == 7);
  update_buffer_size (pad_src, pad_sink, 5);
  ck_assert (pad_src->buffer_size->avg == 6 && pad_sink->buffer_size->avg == 6);
}

GST_END_TEST;

GST_START_TEST (test_update_queue_level)
{
  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! queue ! fakesink", &e);
  add_children_recursively (pipe, packet->elements);
  ElementUnit *unit = g_hash_table_lookup (packet->elements, "fakesrc0");
  update_queue_level (unit);
  ck_assert (unit->queue_level == 0 && unit->max_queue_level == 0);

  unit = g_hash_table_lookup (packet->elements, "queue0");
  ck_assert (unit);
  update_queue_level (unit);
  ck_assert (unit->queue_level == 0 && unit->max_queue_level > 0);

}

GST_END_TEST;

GST_START_TEST (test_push_buffer_pre)
{
  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! fakesink", &e);
  add_children_recursively (pipe, packet->elements);
  element_push_buffer_pre ("fakesrc0", "src", 10, 10);
  element_push_buffer_pre ("fakesrc0", "src", 20, 10);
  ElementUnit *unit = g_hash_table_lookup (packet->elements, "fakesrc0");
  PadUnit *pad_src = g_hash_table_lookup (unit->pad, "src");
  printf ("COMMON:!!!! %f\n", pad_src->buffer_size->avg);
  ck_assert (pad_src->buffer_size->avg == 0);
}

GST_END_TEST;

GST_START_TEST (test_pipeline_init)
{
  GstElement *pipe;
  gchar *name;
  GError *e = NULL;
  gst_ctf_init ();

  ck_assert (gst_liveprofiler_init ());
  pipe = gst_parse_launch ("fakesrc ! fakesink", &e);
  update_pipeline_init (pipe);
  ck_assert (log_idx == metadata_writed);
  element_push_buffer_post ("A", "B", 0);
  element_push_buffer_list_pre ("A", "B", 0);
  element_push_buffer_list_post ("A", "B", 0);
  element_pull_range_pre ("A", "B", 0);
  element_pull_range_post ("A", "B", 0);
}

GST_END_TEST;

static Suite *
gst_live_profiler_suite (void)
{
  Suite *s = suite_create ("GstLiveProfiler");
  TCase *tc = tcase_create ("/tracers/graphics/liveprofiler");

  suite_add_tcase (s, tc);
  tcase_add_test (tc, test_liveprofiler_init);
  tcase_add_test (tc, test_update_cpuusage);
  tcase_add_test (tc, test_add_children);
  tcase_add_test (tc, test_update_proctime);
  tcase_add_test (tc, test_update_datatrate);
  tcase_add_test (tc, test_update_buffer_size);
  tcase_add_test (tc, test_update_queue_level);
  tcase_add_test (tc, test_push_buffer_pre);
  tcase_add_test (tc, test_pipeline_init);

  return s;
}

GST_CHECK_MAIN (gst_live_profiler)
