/*
 * kinect_bleeper GUI monitor
 *
 * Copyright (c) 2012 coolparadox.com
 *
 * This code is licenced according to the GNU General Public Licence version 3.
 * A file COPYING is distributed with the sources of this project containing
 * the full text of the license. Optionally, the full terms of the license 
 * can also be downloaded from http://www.gnu.org/licenses/gpl.html .
 *
 */

#include "monitor.h"

void *monitor_thread(void *thread_data) {

#define data ((struct monitor_data *) thread_data)

	GtkWidget *window;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_widget_show(window);
	gtk_main();

	g_mutex_lock(&data->lock);
	*data->running = 0;
	g_mutex_unlock(&data->lock);

	return 0;

#undef data

}

