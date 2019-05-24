/*  $Id$
 *
 * Based on datetime-plugin
 */

#ifndef SIGNAL_H
#define SIGNAL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* local includes */
#include <time.h>
#include <string.h>

/* xfce includes */
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-panel-convenience.h>

/* enums */
enum {
  DATE = 0,
  TIME
};

typedef struct {
  XfcePanelPlugin * plugin;
  GtkWidget *time_label;
  guint update_interval;  /* time between updates in milliseconds */
  guint timeout_id;

  /* settings */
  gchar *time_font;

  /* option widgets */
} t_signal;

gboolean
signal_update(t_signal *signal);

gchar *
signal_do_utf8strftime(
    const struct tm *tm);

void
signal_apply_font(t_signal *signal,
    const gchar *time_font_name);

void
signal_apply_format(t_signal *signal);

void
signal_write_rc_file(XfcePanelPlugin *plugin,
    t_signal *dt);

#endif /* signal.h */

