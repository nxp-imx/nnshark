/*
 * Copyright (C) 2018 RidgeRun Engineering <manuel.leiva@ridgerun.com>
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
 * SECTION:gstgpuusage
 * @short_description: log gpu usage stats
 *
 * A tracing module that take gpuusage() snapshots and logs them.
 */

#include <glib/gstdio.h>
#include <unistd.h>
#include <string.h>

#include "gstgpuusage.h"
#include "gstgpuusagecompute.h"
#include "gstctf.h"

GST_DEBUG_CATEGORY_STATIC (gst_gpu_usage_debug);
#define GST_CAT_DEFAULT gst_gpu_usage_debug

struct _GstGPUUsageTracer
{
  GstPeriodicTracer parent;
  GstGPUUsage gpu_usage;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_gpu_usage_debug, "gpuusage", 0, "gpuusage tracer");

G_DEFINE_TYPE_WITH_CODE (GstGPUUsageTracer, gst_gpu_usage_tracer,
    GST_TYPE_PERIODIC_TRACER, _do_init);

static GstTracerRecord *tr_gpuusage;

static const gchar gpuusage_metadata_event_header[] = "\
event {\n\
    name = \"gpuusage\";\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n";

static const gchar gpuusage_metadata_event_footer[] = "\
    };\n\
};\n\
\n";

static const gchar floating_point_event_field[] =
    "        floating_point { exp_dig = %lu; mant_dig = %d; byte_order = le; align = 8; } _gpu%d;\n";

static void gpuusage_dummy_bin_add_post (GObject * obj, GstClockTime ts,
    GstBin * bin, GstElement * element, gboolean result);
static gboolean gpu_usage_thread_func (GstPeriodicTracer * tracer);
static void create_metadata_event (GstPeriodicTracer * tracer);
static void reset_counters (GstPeriodicTracer * tracer);

static void
gpuusage_dummy_bin_add_post (GObject * obj, GstClockTime ts,
    GstBin * bin, GstElement * element, gboolean result)
{
  return;
}

static void
reset_counters (GstPeriodicTracer * tracer)
{
  GstGPUUsageTracer *self;

  self = GST_GPU_USAGE_TRACER (tracer);

  gst_gpu_usage_init (&(self->gpu_usage));
}

static gboolean
gpu_usage_thread_func (GstPeriodicTracer * tracer)
{
  GstGPUUsageTracer *self;
  GstGPUUsage *gpu_usage;
  gfloat *gpu_load;
  gint gpu_id;
  gint gpu_load_len;

  self = GST_GPU_USAGE_TRACER (tracer);

  gpu_usage = &self->gpu_usage;

  gpu_load = GPU_USAGE_ARRAY (gpu_usage);
  gpu_load_len = GPU_USAGE_ARRAY_LENGTH (gpu_usage);

  gst_gpu_usage_compute (gpu_usage);

  for (gpu_id = 0; gpu_id < gpu_load_len; ++gpu_id) {
    gst_tracer_record_log (tr_gpuusage, gpu_id, gpu_load[gpu_id]);
  }
  do_print_float_array_event (GPUUSAGE_EVENT_ID, gpu_load_len, gpu_load);

  return TRUE;
}

static void
create_metadata_event (GstPeriodicTracer * tracer)
{
  GstGPUUsageTracer *self;
  gint gpu_num;
  gchar *mem;
  gchar *mem_start;
  gchar *event_header;
  gsize str_size;
  gsize mem_size;
  gint msg_id;
  gint number_of_bytes;

  self = GST_GPU_USAGE_TRACER (tracer);
  gpu_num = self->gpu_usage.gpu_num;

  event_header =
      g_strdup_printf (gpuusage_metadata_event_header, GPUUSAGE_EVENT_ID, 0);

  str_size = strlen (event_header);

  /* Compute event description size
   * size = header + fields + footer
   */
  mem_size =
      str_size + gpu_num * sizeof (floating_point_event_field) +
      sizeof (gpuusage_metadata_event_footer);
  mem_start = g_malloc (mem_size);
  mem = mem_start;

  /* Add event header */
  mem += g_strlcpy (mem, event_header, mem_size);
  mem_size -= str_size;

  /* Add event fields */
  for (msg_id = 0; msg_id < gpu_num; ++msg_id) {
    /* floating point field definition:
     * http://diamon.org/ctf/#spec4.1.7
     */
    number_of_bytes = g_snprintf (mem,
        mem_size,
        floating_point_event_field,
        (unsigned long) (sizeof (gfloat) * CHAR_BIT - FLT_MANT_DIG),
        FLT_MANT_DIG, msg_id);

    mem += number_of_bytes;
    mem_size -= number_of_bytes;
  }

  /* Add event footer */
  g_strlcpy (mem, gpuusage_metadata_event_footer, mem_size);

  /* Add event in metadata file */
  add_metadata_event_struct (mem_start);
  /* Free allocated memory */
  g_free (mem_start);
  g_free (event_header);
}

static void
gst_gpu_usage_tracer_class_init (GstGPUUsageTracerClass * klass)
{
  GstPeriodicTracerClass *tracer_class;

  tracer_class = GST_PERIODIC_TRACER_CLASS (klass);

  tracer_class->timer_callback = GST_DEBUG_FUNCPTR (gpu_usage_thread_func);
  tracer_class->reset = GST_DEBUG_FUNCPTR (reset_counters);
  tracer_class->write_header = GST_DEBUG_FUNCPTR (create_metadata_event);

  tr_gpuusage = gst_tracer_record_new ("gpuusage.class",
      "number", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_UINT,
          "description", G_TYPE_STRING, "Core number",
          "flags", GST_TYPE_TRACER_VALUE_FLAGS,
          GST_TRACER_VALUE_FLAGS_AGGREGATED, "min", G_TYPE_UINT, 0, "max",
          G_TYPE_UINT, GPU_NUM_MAX, NULL), "load", GST_TYPE_STRUCTURE,
      gst_structure_new ("value", "type", G_TYPE_GTYPE, G_TYPE_DOUBLE,
          "description", G_TYPE_STRING, "Core load percentage [%]", "flags",
          GST_TYPE_TRACER_VALUE_FLAGS, GST_TRACER_VALUE_FLAGS_AGGREGATED, "min",
          G_TYPE_DOUBLE, 0.0f, "max", G_TYPE_DOUBLE, 100.0f, NULL), NULL);
}

static void
gst_gpu_usage_tracer_init (GstGPUUsageTracer * self)
{
  GstGPUUsage *gpu_usage;

  gpu_usage = &self->gpu_usage;
  gst_gpu_usage_init (gpu_usage);
  gpu_usage->gpu_array_sel = FALSE;

  /* Register a dummy hook so that the tracer remains alive */
  gst_tracing_register_hook (GST_TRACER (self), "bin-add-post",
      G_CALLBACK (gpuusage_dummy_bin_add_post));
}
