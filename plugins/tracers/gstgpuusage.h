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

#ifndef __GST_GPU_USAGE_TRACER_H__
#define __GST_GPU_USAGE_TRACER_H__

#include "gstperiodictracer.h"

G_BEGIN_DECLS

#define GST_TYPE_GPU_USAGE_TRACER (gst_gpu_usage_tracer_get_type())
G_DECLARE_FINAL_TYPE (GstGPUUsageTracer, gst_gpu_usage_tracer, GST, GPU_USAGE_TRACER, GstPeriodicTracer)

G_END_DECLS
#endif /* __GST_GPU_USAGE_TRACER_H__ */
