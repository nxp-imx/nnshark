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

#ifndef __GST_DDR_USAGE_COMPUTE_H__
#define __GST_DDR_USAGE_COMPUTE_H__

#include <gst/gst.h>

G_BEGIN_DECLS
#define DDR_MEAS_MAX  16
#define DDR_STRING_MAX  16
/* Returns a reference to the array the contains the ddr usage computed */
#define DDR_USAGE_ARRAY(ddrusage_struct)  (ddrusage_struct->load)
#define DDR_EVENT_NAME_ARRAY(ddrusage_struct)  (ddrusage_struct->names)
/* Returns how many element contains the ddr_usage array
 * This value also represents the number of ddrs in the system */
#define DDR_USAGE_ARRAY_LENGTH(ddrusage_struct)  (ddrusage_struct->meas_num)

typedef struct
{
  gint meas_num;
  gint meas_count;
  gfloat load[DDR_MEAS_MAX];
  gchar event_name[DDR_MEAS_MAX][DDR_STRING_MAX];
  gchar *names[DDR_MEAS_MAX];
} GstDDRUsage;

gint gst_ddr_usage_get_nmeas (void);

void gst_ddr_usage_init (GstDDRUsage * usage);

void gst_ddr_usage_finalize(void);

void gst_ddr_usage_compute (GstDDRUsage * usage);

G_END_DECLS
#endif //__GST_DDR_USAGE_COMPUTE_H__
