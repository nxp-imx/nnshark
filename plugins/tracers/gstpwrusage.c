/*
 * Copyright 2021 NXP
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
/**
 * SECTION:gstpwrusage
 * @short_description: log power domain statitistics
 *
 */

#include "gstpwrusage.h"
#include "gstpwrusagecompute.h"
#include "gstctf.h"

GST_DEBUG_CATEGORY_STATIC (gst_pwr_usage_debug);
#define GST_CAT_DEFAULT gst_pwr_usage_debug

struct _GstPWRUsageTracer
{
  GstPeriodicTracer parent;
  GstPWRUsage pwr_usage;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_pwr_usage_debug, "pwrusage", 0, "pwrusage tracer");

G_DEFINE_TYPE_WITH_CODE (GstPWRUsageTracer, gst_pwr_usage_tracer,
    GST_TYPE_PERIODIC_TRACER, _do_init);

static GstTracerRecord *tr_pwrusage;

static const gchar pwrusage_metadata_event_header[] = "\
event {\n\
    name = \"pwrusage\";\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n";

static const gchar pwrusage_metadata_event_footer[] = "\
    };\n\
};\n\
\n";

static const gchar floating_point_event_field[] =
    "        floating_point { exp_dig = %lu; mant_dig = %d; byte_order = le; align = 8; } _%s;\n";


static void
pwrusage_dummy_bin_add_post (GObject * obj, GstClockTime ts,
    GstBin * bin, GstElement * element, gboolean result)
{
  return;
}

static void
reset_counters (GstPeriodicTracer * tracer)
{
  GstPWRUsageTracer *self;

  self = GST_PWR_USAGE_TRACER (tracer);

  gst_pwr_usage_init (&(self->pwr_usage));
}

static gboolean
pwr_usage_thread_func (GstPeriodicTracer * tracer)
{
  GstPWRUsageTracer *self;
  GstPWRUsage *pwr_usage;
  gfloat *pwr_meas;
  gchar **pwr_names;
  gint pwr_id;
  gint pwr_meas_len;

  self = GST_PWR_USAGE_TRACER (tracer);

  pwr_usage = &self->pwr_usage;

  pwr_meas = PWR_USAGE_ARRAY (pwr_usage);
  pwr_meas_len = PWR_USAGE_ARRAY_LENGTH (pwr_usage);
  pwr_names = PWR_EVENT_NAME_ARRAY (pwr_usage);

  gst_pwr_usage_compute (pwr_usage);

  for (pwr_id = 0; pwr_id < pwr_meas_len; ++pwr_id) {
    gst_tracer_record_log (tr_pwrusage, pwr_names[pwr_id], pwr_meas[pwr_id]);
  }

  do_print_pwrusage_event (PWRUSAGE_EVENT_ID, pwr_meas_len, pwr_meas);

  return TRUE;

}

static void
create_metadata_event (GstPeriodicTracer * tracer)
{
  GstPWRUsageTracer *self;
  GstPWRUsage *usage;
  gint written, i;
  gsize mem_size = 4096;

  gchar *mem, *mem_start;

  self = GST_PWR_USAGE_TRACER (tracer);
  usage = &self->pwr_usage;

  mem_start = g_malloc (mem_size);
  mem = mem_start;

  written =
      g_snprintf (mem, mem_size, pwrusage_metadata_event_header,
      PWRUSAGE_EVENT_ID, 0);
  mem += written;
  mem_size -= written;

  for (i = 0; i < usage->meas_num; i += 1) {
    const char *name = usage->names[i];
    written = g_snprintf (mem, mem_size, floating_point_event_field,
        (unsigned long) (sizeof (gfloat) * CHAR_BIT - FLT_MANT_DIG),
        FLT_MANT_DIG, name);
    mem += written;
    mem_size -= written;
  }

  g_strlcpy (mem, pwrusage_metadata_event_footer, mem_size);

  add_metadata_event_struct (mem_start);
  g_free (mem_start);
}

static void
gst_pwr_usage_tracer_class_init (GstPWRUsageTracerClass * klass)
{
  GstPeriodicTracerClass *tracer_class;

  tracer_class = GST_PERIODIC_TRACER_CLASS (klass);

  tracer_class->timer_callback = GST_DEBUG_FUNCPTR (pwr_usage_thread_func);
  tracer_class->reset = GST_DEBUG_FUNCPTR (reset_counters);
  tracer_class->write_header = GST_DEBUG_FUNCPTR (create_metadata_event);

  tr_pwrusage = gst_tracer_record_new ("pwrusage.class",
      "name", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "description", G_TYPE_STRING, "PWR domain name",
          NULL),
      "value", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_FLOAT,
          "description", G_TYPE_STRING, "Power measurement (mw)",
          "min", G_TYPE_FLOAT, 0.0f,
          "max", G_TYPE_FLOAT, G_MAXFLOAT, NULL), NULL);
}

static void
gst_pwr_usage_tracer_init (GstPWRUsageTracer * self)
{
  GstPWRUsage *pwr_usage = &self->pwr_usage;

  gst_pwr_usage_init (pwr_usage);

  /* Register a dummy hook so that the tracer remains alive */
  gst_tracing_register_hook (GST_TRACER (self), "bin-add-post",
      G_CALLBACK (pwrusage_dummy_bin_add_post));
}
