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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <glib/gstdio.h>
#include "gstgpuusage.h"
#include "gstgpuusagecompute.h"

#include <gpuperfcnt/gpuperfcnt.h>
#include <gpuperfcnt/gpuperfcnt_vivante.h>
#include <gpuperfcnt/gpuperfcnt_log.h>

#include <unistd.h>
#include <string.h>

#define GPU_NAME_MAX_SIZE 8
#define S(arg) XS(arg)
#define XS(arg) #arg
/**
 * \brief: helper macro for iterating over clients list
 */
#define list_for_each(client, head) 	\
	for (client = head; client != NULL; client = client->next)

struct GstGPUUsage_state
{
  struct perf_device *dev;
  gint gpu_num;
  gint gpu_type[GPU_NUM_MAX];
  char *gpu_names[GPU_NUM_MAX];
  uint32_t idle_cycles[GPU_NUM_MAX];
  uint32_t cycles[GPU_NUM_MAX];
};

static struct GstGPUUsage_state state;

static void
_gst_gpu_usage_init_perf_if_required (void)
{
  struct perf_device *gpu_dev = state.dev;
  if (gpu_dev == NULL) {
    struct perf_hw_info *hw_info_iter;
    struct perf_hw_info hw_info = { };

    gpu_dev = perf_init (&vivante_ops);
    g_return_if_fail (gpu_dev);
    state.dev = gpu_dev;

    if (perf_open (VIV_HW_3D, gpu_dev) < 0) {
      GST_WARNING ("Failed to open gpu perf counters");
      perf_exit (gpu_dev);
      return;
    }
    perf_get_hw_info (&hw_info, gpu_dev);
    state.gpu_num = 0;

    list_for_each (hw_info_iter, hw_info.head) {
      gchar name[GPU_NAME_MAX_SIZE];
      /* Identify different GPU type */
      enum perf_core_type c_type =
          perf_get_core_type (hw_info_iter->id, gpu_dev);
      state.gpu_type[state.gpu_num] = c_type;
      g_snprintf (name, GPU_NAME_MAX_SIZE, "GC%x", hw_info_iter->model);
      state.gpu_names[state.gpu_num] = g_strndup (name, GPU_NAME_MAX_SIZE);
      state.idle_cycles[state.gpu_num] = 0;
      state.cycles[state.gpu_num] = 0;
      state.gpu_num++;
    }

    perf_free_hw_info (&hw_info, gpu_dev);
  }
}

int
gst_gpu_usage_get_ngpus (void)
{
  _gst_gpu_usage_init_perf_if_required ();

  return state.gpu_num;
}

void
gst_gpu_usage_init (GstGPUUsage * gpu_usage)
{
  g_return_if_fail (gpu_usage);

  _gst_gpu_usage_init_perf_if_required ();

  memset (gpu_usage, 0, sizeof (GstGPUUsage));
  gpu_usage->gpu_array_sel = FALSE;

  gpu_usage->gpu_num = state.gpu_num;
  memcpy (gpu_usage->gpu_names, state.gpu_names,
      sizeof (gchar *) * state.gpu_num);
}

void
gst_gpu_usage_compute (GstGPUUsage * gpu_usage)
{
  gfloat *gpu_load;
  gint gpu_num;
  gint gpu_id;

  struct perf_device *gpu_dev = state.dev;

#define GC_TOTAL_IDLE_CYCLES            0x0000007c
#define GC_TOTAL_CYCLES                 0x00000078
  uint32_t idle_reg_addr = GC_TOTAL_IDLE_CYCLES;
  uint32_t cycle_reg_addr = GC_TOTAL_CYCLES;
  uint32_t *idle_cycles = &state.idle_cycles[0];
  uint32_t *cycles = &state.cycles[0];
  gint ret;

  g_return_if_fail (gpu_dev);
  g_return_if_fail (gpu_usage);

  gpu_load = gpu_usage->gpu_load;
  gpu_num = gpu_usage->gpu_num;

  for (gpu_id = 0; gpu_id < gpu_num; ++gpu_id) {
    perf_read_register (gpu_id, idle_reg_addr, &idle_cycles[gpu_id], gpu_dev);
    perf_read_register (gpu_id, cycle_reg_addr, &cycles[gpu_id], gpu_dev);
#if 0
    fprintf (stderr, "GPU %d, Idle: %u Cycles: %d\n",
        gpu_id, idle_cycles[gpu_id], cycles[gpu_id]);
#endif
  }

  /* Compute the utilization for each core */
  for (gpu_id = 0; gpu_id < gpu_num; ++gpu_id) {
    gint _idle_cycles = idle_cycles[gpu_id];
    gint _cycles = cycles[gpu_id];
    if (_idle_cycles > _cycles)
      _idle_cycles = _cycles;
    gpu_load[gpu_id] =
        100.0 * ((float) (_cycles - _idle_cycles)) / (float) _cycles;
  }

  for (gpu_id = 0; gpu_id < gpu_num; ++gpu_id) {
    perf_write_register (gpu_id, cycle_reg_addr, 0, gpu_dev);
    perf_write_register (gpu_id, idle_reg_addr, 0, gpu_dev);
  }
  (void) ret;

}
