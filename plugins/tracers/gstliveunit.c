#include "gstliveunit.h"
#include "glib.h"
#include <stdio.h>

void
avg_update_value (AvgUnit * unit, guint64 value)
{
  unit->value = value;
  unit->avg += (value - unit->avg) / ++(unit->num);
}

void
time_push_value (TimeUnit * unit, guint64 ts)
{
  guint64 *pts = g_malloc (sizeof (guint64));
  *pts = ts;
  g_queue_push_head (unit->time_log, pts);
}

guint64
time_pop_value (TimeUnit * unit, guint64 ts)
{
  guint64 *pts;
  if (g_queue_get_length (unit->time_log) > 0) {
    pts = (guint64 *) g_queue_pop_tail (unit->time_log);
    return ts - *pts;
  }
  return 0;
}

AvgUnit *
avg_unit_new (void)
{
  AvgUnit *a = g_malloc0 (sizeof (AvgUnit));
  a->num = 0;
  a->avg = 0;
  return a;
}

TimeUnit *
time_unit_new (void)
{
  TimeUnit *t = g_malloc0 (sizeof (TimeUnit));
  t->time_log = g_queue_new ();
  return t;
}

ElementUnit *
element_unit_new (void)
{
  ElementUnit *e = g_malloc0 (sizeof (ElementUnit));
  e->proctime = avg_unit_new ();
  e->time_log = time_unit_new ();       // On testing!
  e->time = 0;
  return e;
}

PadUnit *
pad_unit_new (void)
{
  PadUnit *p = g_malloc0 (sizeof (PadUnit));
  p->time = 0;
  p->time_log = g_queue_new ();
  p->buffer_size = avg_unit_new ();
  p->datarate = 0;
  p->num = 0;
  return p;
}

PadUnit *
pad_unit_peer (GHashTable * elements, PadUnit * target)
{
  GstPad *peer = gst_pad_get_peer ((GstPad *) target->element);
  ElementUnit *peerElementUnit;
  PadUnit *peerPadUnit;

  peerElementUnit = g_hash_table_lookup (elements,
      GST_OBJECT_NAME (GST_OBJECT_PARENT (peer)));
  peerPadUnit = g_hash_table_lookup (peerElementUnit->pad,
      GST_OBJECT_NAME (peer));

  return peerPadUnit;
}

ElementUnit *
pad_unit_parent (GHashTable * elements, PadUnit * target)
{
  return g_hash_table_lookup (elements,
      GST_OBJECT_NAME (GST_OBJECT_PARENT (target->element)));
}
