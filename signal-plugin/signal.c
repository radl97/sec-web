/*  $Id$
 *
 *  Copyright (C) 2003 Choe Hwanjin(krisna@kldp.org)
 *  Copyright (c) 2006 Remco den Breeje <remco@sx.mine.nu>
 *  Copyright (c) 2008 Diego Ongaro <ongardie@gmail.com>
 *  Copyright (c) 2016 Landry Breuil <landry@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "signal.h"

#define DATETIME_MAX_STRLEN 256

/**
 *  Convert a GTimeVal to milliseconds.
 *  Fractions of a millisecond are truncated.
 *  With a 32 bit word size and some values of t.tv_sec,
 *  multiplication by 1000 could overflow a glong,
 *  so the value is cast to a gint64.
 */
static inline gint64 signal_gtimeval_to_ms(const GTimeVal t)
{
  return ((gint64) t.tv_sec * 1000) + ((gint64) t.tv_usec / 1000);
}

static void signal_update_time_font(t_signal *signal)
{
  GtkCssProvider *css_provider;
  gchar * css;
  PangoFontDescription *font;
  font = pango_font_description_from_string(signal->time_font);
  if (G_LIKELY (font))
  {
    css = g_strdup_printf("label { font-family: %s; font-size: %dpx; font-style: %s; font-weight: %s }",
                          pango_font_description_get_family (font),
                          pango_font_description_get_size (font) / PANGO_SCALE,
                          (pango_font_description_get_style(font) == PANGO_STYLE_ITALIC ||
                           pango_font_description_get_style(font) == PANGO_STYLE_OBLIQUE) ? "italic" : "normal",
                          (pango_font_description_get_weight(font) >= PANGO_WEIGHT_BOLD) ? "bold" : "normal");
    pango_font_description_free (font);
  }
  else
    css = g_strdup_printf("label { font: %s; }",
                          signal->time_font);
    /* Setup Gtk style */
    DBG("css: %s",css);
    css_provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (css_provider, css, strlen(css), NULL);
    gtk_style_context_add_provider (
        GTK_STYLE_CONTEXT (gtk_widget_get_style_context (GTK_WIDGET (signal->time_label))),
        GTK_STYLE_PROVIDER (css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_free(css);
}

/*
 * Get date/time string
 */
gchar * signal_do_utf8strftime(const struct tm *tm)
{
  gchar buf[DATETIME_MAX_STRLEN];
  gchar *utf8str = NULL;

  FILE* pipe = popen("sudo wifistat", "r");
  if (!pipe) 
    return g_strdup(_("Error..."));
  int len = fread(buf, 1, sizeof(buf[0])*DATETIME_MAX_STRLEN-1, pipe);
  pclose(pipe);

  if (len < 0)
    return g_strdup(_("Invalid format"));
  buf[len] = 0;

  utf8str = g_locale_to_utf8(buf, -1, NULL, NULL, NULL);
  if(utf8str == NULL)
    return g_strdup(_("Error"));

  return utf8str;
}

/*
 * set date and time labels
 */
gboolean signal_update(t_signal *signal)
{
  GTimeVal timeval;
  gchar *utf8str;
  struct tm *current;
  guint wake_interval;  /* milliseconds to next update */

  DBG("wake");

  /* stop timer */
  if (signal->timeout_id)
  {
    g_source_remove(signal->timeout_id);
  }

  g_get_current_time(&timeval);
  current = localtime((time_t *)&timeval.tv_sec);

  if (GTK_IS_LABEL(signal->time_label))
  {
    utf8str = signal_do_utf8strftime(current);
    gtk_label_set_text(GTK_LABEL(signal->time_label), utf8str);
    g_free(utf8str);
  }

  /* Compute the time to the next update and start the timer. */
  wake_interval = signal->update_interval;
  signal->timeout_id = g_timeout_add(wake_interval, (GSourceFunc) signal_update, signal);

  return TRUE;
}

static void signal_set_update_interval(t_signal *signal)
{
  /* a custom date format could specify seconds */

  /* 1000 ms in 1 second */
  signal->update_interval = 10000;
}

/*
 * set layout after doing some checks
 */
void signal_apply_layout(t_signal *signal)
{

  /* hide labels based on layout-selection */
  gtk_widget_show(GTK_WIDGET(signal->time_label));

  signal_set_update_interval(signal);
}

/*
 * set the date and time font type
 */
void signal_apply_font(t_signal *signal,
    const gchar *time_font_name)
{
  if (time_font_name != NULL)
  {
    g_free(signal->time_font);
    signal->time_font = g_strdup(time_font_name);
    signal_update_time_font(signal);
  }
}

/*
 * set the date and time format
 */
void signal_apply_format(t_signal *signal)
{
  if (signal == NULL)
    return;

  signal_set_update_interval(signal);
}

/*
 * Function only called by the signal handler.
 */
static int signal_set_size(XfcePanelPlugin *plugin,
    gint size,
    t_signal *signal)
{
  GtkOrientation orientation;

  orientation = xfce_panel_plugin_get_orientation (plugin);

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
  else
    gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);
  return TRUE;
}

/*
 * Read the settings from the config file
 */
static void signal_read_rc_file(XfcePanelPlugin *plugin, t_signal *dt)
{
  gchar *file;
  XfceRc *rc = NULL;
  const gchar *time_font;

  /* load defaults */
  time_font = "Bitstream Vera Sans 8";

  /* open file */
  if((file = xfce_panel_plugin_lookup_rc_file(plugin)) != NULL)
  {
    rc = xfce_rc_simple_open(file, TRUE);
    g_free(file);

    if(rc != NULL)
    {
      time_font   = xfce_rc_read_entry(rc, "time_font", time_font);
    }
  }

  time_font   = g_strdup(time_font);

  if(rc != NULL)
    xfce_rc_close(rc);

  /* set values in dt struct */
  signal_apply_layout(dt);
  signal_apply_font(dt, time_font);
}

/*
 * write the settings to the config file
 */
void signal_write_rc_file(XfcePanelPlugin *plugin, t_signal *dt)
{
  char *file;
  XfceRc *rc;

  if(!(file = xfce_panel_plugin_save_location(plugin, TRUE)))
    return;

  rc = xfce_rc_simple_open(file, FALSE);
  g_free(file);

  if(rc != NULL)
  {
    xfce_rc_write_entry(rc, "time_font", dt->time_font);

    xfce_rc_close(rc);
  }

}

/*
 * change widgets orientation when the panel orientation changes
 */
static void signal_set_mode(XfcePanelPlugin *plugin, XfcePanelPluginMode mode, t_signal *signal)
{
  GtkOrientation orientation = (mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL) ?
    GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
  if (orientation == GTK_ORIENTATION_VERTICAL)
  {
    gtk_label_set_angle(GTK_LABEL(signal->time_label), -90);
  }
  else
  {
    gtk_label_set_angle(GTK_LABEL(signal->time_label), 0);
  }
}

/*
 * create the gtk-part of the signal plugin
 */
static void signal_create_widget(t_signal * signal)
{
  GtkOrientation orientation;
  orientation = xfce_panel_plugin_get_orientation(signal->plugin);

  /* create time and date lines */
  signal->time_label = gtk_label_new("ASDFSDFSDFSDFS");
  gtk_label_set_justify(GTK_LABEL(signal->time_label), GTK_JUSTIFY_CENTER);

  /* set orientation according to the panel orientation */
  signal_set_mode(signal->plugin, orientation, signal);
}

/*
 * create signal plugin
 */
static t_signal * signal_new(XfcePanelPlugin *plugin)
{
  t_signal * signal;

  DBG("Starting signals panel plugin");

  /* alloc and clear mem */
  signal = panel_slice_new0 (t_signal);

  /* store plugin reference */
  signal->plugin = plugin;

  /* call widget-create function */
  signal_create_widget(signal);

  /* load settings (default values if non-av) */
  signal_read_rc_file(plugin, signal);

  /* set date and time labels */
  signal_update(signal);

  return signal;
}

/*
 * frees the signal struct
 */
static void signal_free(XfcePanelPlugin *plugin, t_signal *signal)
{
  /* stop timeouts */
  if (signal->timeout_id != 0)
    g_source_remove(signal->timeout_id);

  /* destroy widget */
  gtk_widget_destroy(signal->time_label);

  /* cleanup */
  g_free(signal->time_font);

  panel_slice_free(t_signal, signal);
}

/*
 * Construct the plugin
 */
static void signal_construct(XfcePanelPlugin *plugin)
{
  /* create signal plugin */
  t_signal * signal = signal_new(plugin);

  /* add plugin to panel */
  gtk_container_add(GTK_CONTAINER(plugin), signal->time_label);
  xfce_panel_plugin_add_action_widget(plugin, signal->time_label);

  /* connect plugin signals to functions */
  g_signal_connect(plugin, "save",
      G_CALLBACK(signal_write_rc_file), signal);
  g_signal_connect(plugin, "free-data",
      G_CALLBACK(signal_free), signal);
  g_signal_connect(plugin, "size-changed",
      G_CALLBACK(signal_set_size), signal);
  g_signal_connect(plugin, "mode-changed", G_CALLBACK(signal_set_mode), signal);
  xfce_panel_plugin_menu_show_configure(plugin);
}


XFCE_PANEL_PLUGIN_REGISTER(signal_construct);

