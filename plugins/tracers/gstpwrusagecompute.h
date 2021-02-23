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

#ifndef __GST_PWR_USAGE_COMPUTE_H__
#define __GST_PWR_USAGE_COMPUTE_H__

#include <gst/gst.h>
#include <gio/gio.h>

G_BEGIN_DECLS
#define PWR_MEAS_MAX  16
#define PWR_STRING_MAX  16
#define PWR_NR_MEAS     16
/* Returns a reference to the array the contains the pwr usage computed */
#define PWR_USAGE_ARRAY(pwrusage_struct)  (pwrusage_struct->load)
#define PWR_EVENT_NAME_ARRAY(pwrusage_struct)  (pwrusage_struct->names)
/* Returns how many element contains the pwr_usage array
 * This value also represents the number of pwrs in the system */
#define PWR_USAGE_ARRAY_LENGTH(pwrusage_struct)  (pwrusage_struct->meas_num)

typedef struct
{
  gint meas_num;
  gint meas_count;
  gfloat load[PWR_MEAS_MAX];
  gfloat meas[PWR_MEAS_MAX][PWR_NR_MEAS];
  gchar event_name[PWR_MEAS_MAX][PWR_STRING_MAX];
  gchar *names[PWR_MEAS_MAX];
} GstPWRUsage;

gint gst_pwr_usage_get_nmeas (void);

void gst_pwr_usage_init (GstPWRUsage * usage);

void gst_pwr_usage_finalize(void);

void gst_pwr_usage_compute (GstPWRUsage * usage);

G_END_DECLS
#endif //__GST_PWR_USAGE_COMPUTE_H__
