/* GstShark - A Front End for GstTracer
 * Copyright 2021 NXP
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
/**
 * SECTION:gstddrusage
 * @short_description: log ddr usage stats
 *
 * A tracing module that take ddrusage() snapshots and logs them.
 */


#include "gstddrusage.h"
#include "gstddrusagecompute.h"
#include "gstctf.h"


GST_DEBUG_CATEGORY_STATIC (gst_ddr_usage_debug);
#define GST_CAT_DEFAULT gst_ddr_usage_debug

struct _GstDDRUsageTracer
{
  GstPeriodicTracer parent;
  GstDDRUsage ddr_usage;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_ddr_usage_debug, "ddrusage", 0, "ddrusage tracer");

G_DEFINE_TYPE_WITH_CODE (GstDDRUsageTracer, gst_ddr_usage_tracer,
    GST_TYPE_PERIODIC_TRACER, _do_init);


static GstTracerRecord *tr_ddrusage;

static const gchar ddrusage_metadata_event_header[] = "\
event {\n\
    name = \"ddrusage\";\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n";

static const gchar ddrusage_metadata_event_footer[] = "\
    };\n\
};\n\
\n";

static const gchar floating_point_event_field[] =
    "        floating_point { exp_dig = %lu; mant_dig = %d; byte_order = le; align = 8; } _%s;\n";

static void
ddrusage_dummy_bin_add_post (GObject * obj, GstClockTime ts,
    GstBin * bin, GstElement * element, gboolean result)
{
  return;
}

static gboolean
ddr_usage_thread_func (GstPeriodicTracer * tracer)
{
  GstDDRUsageTracer *self;
  GstDDRUsage *ddr_usage;

  gfloat *ddr_load;
  gint ddr_load_len, i;
  gchar **ddr_names;

  self = GST_DDR_USAGE_TRACER (tracer);

  ddr_usage = &self->ddr_usage;

  ddr_load = DDR_USAGE_ARRAY (ddr_usage);
  ddr_load_len = DDR_USAGE_ARRAY_LENGTH (ddr_usage);
  ddr_names = DDR_EVENT_NAME_ARRAY (ddr_usage);

  gst_ddr_usage_compute (ddr_usage);

  for (i = 0; i < ddr_load_len; i += 2) {
    char name[DDR_MEAS_MAX];
    gsize written = g_strlcpy (name, ddr_names[i], DDR_MEAS_MAX);
    name[written - 3] = '\0';
    gst_tracer_record_log (tr_ddrusage, name, ddr_load[i], ddr_load[i + 1]);
  }

  do_print_ddrusage_event (DDRUSAGE_EVENT_ID, ddr_load_len, ddr_load);

  return TRUE;
}


static void
reset_counters (GstPeriodicTracer * tracer)
{
  GstDDRUsageTracer *self;

  self = GST_DDR_USAGE_TRACER (tracer);

  gst_ddr_usage_init (&(self->ddr_usage));
}

static void
create_metadata_event (GstPeriodicTracer * tracer)
{
  GstDDRUsageTracer *self;
  GstDDRUsage *usage;
  gint written, i;
  gsize mem_size = 4096;

  gchar *mem, *mem_start;

  self = GST_DDR_USAGE_TRACER (tracer);
  usage = &self->ddr_usage;

  mem_start = g_malloc (mem_size);
  mem = mem_start;

  written =
      g_snprintf (mem, mem_size, ddrusage_metadata_event_header,
      DDRUSAGE_EVENT_ID, 0);
  mem += written;
  mem_size -= written;

  for (i = 0; i < usage->meas_num; i += 2) {
    const char *rd_name = usage->names[i];
    const char *wr_name = usage->names[i + 1];
    written = g_snprintf (mem, mem_size, floating_point_event_field,
        (unsigned long) (sizeof (gfloat) * CHAR_BIT - FLT_MANT_DIG),
        FLT_MANT_DIG, rd_name);
    mem += written;
    mem_size -= written;
    written = g_snprintf (mem, mem_size, floating_point_event_field,
        (unsigned long) (sizeof (gfloat) * CHAR_BIT - FLT_MANT_DIG),
        FLT_MANT_DIG, wr_name);
    mem += written;
    mem_size -= written;

  }

  g_strlcpy (mem, ddrusage_metadata_event_footer, mem_size);

  add_metadata_event_struct (mem_start);
  g_free (mem_start);
}

static void
gst_ddr_usage_tracer_class_init (GstDDRUsageTracerClass * klass)
{
  GstPeriodicTracerClass *tracer_class;

  tracer_class = GST_PERIODIC_TRACER_CLASS (klass);

  tracer_class->timer_callback = GST_DEBUG_FUNCPTR (ddr_usage_thread_func);
  tracer_class->reset = GST_DEBUG_FUNCPTR (reset_counters);
  tracer_class->write_header = GST_DEBUG_FUNCPTR (create_metadata_event);

  tr_ddrusage = gst_tracer_record_new ("ddrusage.class",
      "name", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "description", G_TYPE_STRING, "DDR initiator name",
          NULL),
      "read", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_FLOAT,
          "description", G_TYPE_STRING, "Read bandwidth",
          "min", G_TYPE_FLOAT, 0.0f,
          "max", G_TYPE_FLOAT, G_MAXFLOAT,
          NULL),
      "write", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_FLOAT,
          "description", G_TYPE_STRING, "Write bandwidth",
          "min", G_TYPE_FLOAT, 0.0f,
          "max", G_TYPE_FLOAT, G_MAXFLOAT, NULL), NULL);
}

static void
gst_ddr_usage_tracer_init (GstDDRUsageTracer * self)
{
  GstDDRUsage *ddr_usage = &self->ddr_usage;

  gst_ddr_usage_init (ddr_usage);

  /* Register a dummy hook so that the tracer remains alive */
  gst_tracing_register_hook (GST_TRACER (self), "bin-add-post",
      G_CALLBACK (ddrusage_dummy_bin_add_post));

}
