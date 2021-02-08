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

#ifndef __GST_GPU_USAGE_COMPUTE_H__
#define __GST_GPU_USAGE_COMPUTE_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define GPU_NUM_MAX  8
/* Returns a reference to the array the contains the gpu usage computed */
#define GPU_USAGE_ARRAY(gpuusage_struct)  (gpuusage_struct->gpu_load)
#define GPU_EVENT_NAME_ARRAY(gpuusage_struct)  (gpuusage_struct->gpu_names)
/* Returns how many element contains the gpu_usage array
 * This value also represents the number of gpus in the system */
#define GPU_USAGE_ARRAY_LENGTH(gpuusage_struct)  (gpuusage_struct->gpu_num)
    typedef struct
{
  gint gpu_num; /* GPU core number */
  gfloat gpu_load[GPU_NUM_MAX]; /* Scaled GPU load */

  char * gpu_names[GPU_NUM_MAX];
  gboolean gpu_array_sel;
} GstGPUUsage;

int gst_gpu_usage_get_ngpus(void);

void gst_gpu_usage_init (GstGPUUsage * usage);

void gst_gpu_usage_compute (GstGPUUsage * usage);

G_END_DECLS
#endif //__GST_GPU_USAGE_COMPUTE_H__
