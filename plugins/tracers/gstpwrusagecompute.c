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
#include "gstpwrusagecompute.h"

#include <stdint.h>
#include <unistd.h>
#include <string.h>

#define SOC_NAME_MAX_LENGTH 8
static gchar soc_name[SOC_NAME_MAX_LENGTH];

enum soc_id
{
  IMX8MP,
};

struct perf_pwr_type
{
  const char *name;
  float value;
};

static struct perf_pwr_type perf_pwr_imx8mp[] = {
  {"VDD_ARM", 0.0f},
  {"NVCC_DRAM_1V1", 0.0f},
  {"VSYS_5V", 0.0f},
  {"VDD_SOC", 0.0f},
  {"LPD4_VDDQ", 0.0f},
  {"LPD4_VDD2", 0.0f},
  {"LPD4_VDD1", 0.0f},
  {0}
};

static struct perf_pwr_type *perf_pwr_socs[] = {
  [IMX8MP] = perf_pwr_imx8mp,
};

static struct perf_pwr_type *perf_pwr_soc = NULL;

static GThread *pwr_meas_thread = NULL;
static GCancellable *pwr_meas_cancellable = NULL;



static void
gtop_set_perf_pwr_soc (void)
{
  FILE *file;
  char buf[1024];
  if (strlen (soc_name))
    return;

  file = fopen ("/sys/devices/soc0/soc_id", "r");
  if (file == NULL)
    return;

  if ((fgets (buf, 1024, file)) != NULL) {
    if (!strncmp (buf, "i.MX8MP", SOC_NAME_MAX_LENGTH - 1)) {
      perf_pwr_soc = perf_pwr_socs[IMX8MP];
    }
    strncpy (soc_name, buf, SOC_NAME_MAX_LENGTH);
  }

}

#ifdef PWR_MEAS_AVERAGE
static gfloat
get_average_meas (gfloat * meas)
{
  gint j, cnt = 0;
  gfloat average = 0.0f;
  for (j = 0; j < PWR_NR_MEAS; j++) {
    if (meas[j] != 0.0f) {
      average += meas[j];
      cnt++;
    }
  }

  if (cnt == 0)
    return 0.0f;
  else
    return average / cnt;
}
#endif


static gpointer
pwr_meas_thread_func (gpointer data)
{
  GstPWRUsage *usage = data;
  GError *error = NULL;
  GInputStream *in_stream;
  GOutputStream *out_stream;
  gchar incoming_buff[1024] = { 0 };
  const gchar *buffer = "data request";
  guint16 server_port = 65432;
  const gchar *server_ip;

  GSocketClient *client = g_socket_client_new ();

  server_ip = g_getenv ("GST_TRACERS_PWR_SERVER_IP");
  if (server_ip == NULL)
    server_ip = "127.0.0.1";

  GSocketConnection *connection = g_socket_client_connect_to_host (client,
      server_ip,
      server_port,
      NULL,
      &error);

  if (error) {
    g_warning ("%s", error->message);
    g_error_free (error);
    goto exit_thread;
  }

  in_stream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
  out_stream = g_io_stream_get_output_stream (G_IO_STREAM (connection));

  pwr_meas_cancellable = g_cancellable_new ();

  while (1) {
    struct perf_pwr_type *evt = &perf_pwr_soc[0];
    const gchar *delim = ";";
    gchar **items = NULL, **tmp;
    gint cnt, i;
    g_output_stream_write (out_stream,
        buffer, strlen (buffer) + 1, NULL, &error);

    if (error)
      g_error ("%s", error->message);

    g_input_stream_read (in_stream,
        incoming_buff, sizeof (incoming_buff), pwr_meas_cancellable, &error);

    if (error) {
      if (error->code == G_IO_ERROR_CANCELLED) {
        g_error_free (error);
        break;
      }
      g_error ("%s", error->message);
    }
    g_strstrip (incoming_buff);
    items = g_strsplit (incoming_buff, delim, -1);
    cnt = usage->meas_count++ % PWR_NR_MEAS;
    for (evt = &perf_pwr_soc[0], i = 0; (evt->name != NULL); evt++, i++) {
      gint meas_name_len = strlen (evt->name);
      for (tmp = items; *tmp; tmp++) {
        if (g_ascii_strncasecmp (evt->name, *tmp, meas_name_len) == 0) {
          gchar *avalue = *tmp;
          avalue += meas_name_len + 1;  /* measName:float */
          usage->meas[i][cnt] = (float) atof (avalue);
#ifdef PWR_MEAS_AVERAGE
          evt->value = get_average_meas (usage->meas[i]);
#else
          evt->value = usage->meas[i][cnt];
#endif
          break;
        }
      }
    }
    g_strfreev (items);
    g_usleep (G_USEC_PER_SEC);
  }

exit_thread:
  g_thread_exit (NULL);
  return (gpointer) NULL;
}

gint
gst_pwr_usage_get_nmeas (void)
{
  struct perf_pwr_type *evt;
  gint i = 0;
  gtop_set_perf_pwr_soc ();

  evt = &perf_pwr_soc[0];

  for (i = 0; (evt->name != NULL); i++) {
    evt++;
  }

  return i;
}

void
gst_pwr_usage_init (GstPWRUsage * usage)
{
  struct perf_pwr_type *evt;
  gint i = 0;

  g_return_if_fail (usage);

  if (pwr_meas_thread > 0)
    return;                     /* TODO: Reset perf ? */

  memset (usage, 0, sizeof (GstPWRUsage));

  gtop_set_perf_pwr_soc ();

  evt = &perf_pwr_soc[0];
  g_return_if_fail (evt);

  for (i = 0; (evt->name != NULL); i++, evt++) {
    char *meas_name;
    /* Read */
    meas_name = usage->event_name[i];
    usage->names[i] = meas_name;
    snprintf (meas_name, PWR_STRING_MAX, "%s", evt->name);

    usage->meas_num++;
  }

  pwr_meas_thread = g_thread_new ("PwrMeas Thread",
      pwr_meas_thread_func, usage);
}

void
gst_pwr_usage_finalize (void)
{
  /* Trigger the cancel object */
  if (pwr_meas_cancellable)
    g_cancellable_cancel (pwr_meas_cancellable);

  if (pwr_meas_thread) {
    g_thread_unref (pwr_meas_thread);
    pwr_meas_thread = NULL;
  }

  pwr_meas_cancellable = NULL;
}

void
gst_pwr_usage_compute (GstPWRUsage * usage)
{
  gint i;
  struct perf_pwr_type *evt;

  g_return_if_fail (usage);

  gtop_set_perf_pwr_soc ();
  evt = &perf_pwr_soc[0];

  for (i = 0; (evt->name != NULL); i++, evt++) {
    usage->load[i] = evt->value;
  }
}
