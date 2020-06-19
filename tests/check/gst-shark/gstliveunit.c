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
#include "glib.h"
#include "gstliveunit.h"

GST_START_TEST (test_packet_new)
{
  Packet *p;
  p = packet_new (7);
  ck_assert (p->cpu_num == 7);
}

GST_END_TEST;

GST_START_TEST (test_is_filter)
{
  GstElement *element;
  GError e;
  element = gst_element_factory_make ("identity", "asdfa");
  ck_assert (is_filter (element));

}

GST_END_TEST;

GST_START_TEST (test_is_filter_false)
{
  GstElement *element;
  GError e;
  element = gst_element_factory_make ("fakesrc", "asdfa");
  ck_assert (!is_filter (element));

}

GST_END_TEST;
GST_START_TEST (test_avg_unit)
{
  AvgUnit *a = avg_unit_new ();
  ck_assert (a->value == 0 && a->num == 0 && a->avg == 0);

  avg_update_value (a, 7);
  ck_assert (a->value == 7);
}

GST_END_TEST;

GST_START_TEST (test_avg_unit_update)
{
  AvgUnit *a = avg_unit_new ();
  ck_assert (a->value == 0 && a->num == 0 && a->avg == 0);

  avg_update_value (a, 7);
  ck_assert (a->avg == 7);

  avg_update_value (a, 5);
  ck_assert (a->avg == 6);

  avg_update_value (a, 3);
  ck_assert (a->avg == 5);
}

GST_END_TEST;

GST_START_TEST (test_element_unit)
{
  GstElement *element;
  ElementUnit *unit;
  gchar *name;
  element = gst_element_factory_make ("fakesrc", "asdfa");
  unit = element_unit_new (element);
  g_object_get (G_OBJECT (unit->element), "name", &name, NULL);
  ck_assert (unit->element == element);
  assert_equals_string (name, "asdfa");
  ck_assert (unit->time == 0 && unit->queue_level == 0
      && unit->max_queue_level == 0);
  ck_assert (!unit->is_filter && !unit->is_queue);
  g_free (name);
  ck_assert (element_unit_free (unit));
}

GST_END_TEST;

GST_START_TEST (test_queue)
{
  GstElement *element;
  ElementUnit *unit;
  gchar *name;
  element = gst_element_factory_make ("queue", "asdfa");
  unit = element_unit_new (element);
  g_object_get (G_OBJECT (element), "name", &name, NULL);
  ck_assert (unit->element == element);
  assert_equals_string (name, "asdfa");
  ck_assert (unit->time == 0 && unit->queue_level == 0
      && unit->max_queue_level == 0);
  ck_assert (unit->is_queue);
  g_free (name);
  ck_assert (element_unit_free (unit));
}

GST_END_TEST;

GST_START_TEST (test_pad_unit)
{
  GstElement *element;
  ElementUnit *unit;
  PadUnit *pad;

  element = gst_element_factory_make ("fakesrc", "name");
  unit = element_unit_new (element);
  pad = pad_unit_new (unit);
  assert_equals_pointer (pad->element, unit);
  ck_assert (pad->time == 0 && pad->datarate == 0 && pad->num == 0);

}

GST_END_TEST;



static Suite *
gst_live_unit_suite (void)
{
  Suite *s = suite_create ("GstLiveUnit");
  TCase *tc = tcase_create ("/tracers/graphics/liveunit");

  suite_add_tcase (s, tc);
  tcase_add_test (tc, test_packet_new);
  tcase_add_test (tc, test_is_filter);
  tcase_add_test (tc, test_is_filter_false);
  tcase_add_test (tc, test_avg_unit);
  tcase_add_test (tc, test_avg_unit_update);
  tcase_add_test (tc, test_element_unit);
  tcase_add_test (tc, test_queue);
  tcase_add_test (tc, test_pad_unit);

  return s;
}

GST_CHECK_MAIN (gst_live_unit)
