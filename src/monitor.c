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

#include <stdlib.h>

#include "monitor.h"

void *monitor_thread(void *thread_data) {

#define data ((struct monitor_data *) thread_data)
#define get_lock(data) g_mutex_lock(&data->lock)
#define release_lock(data) g_mutex_unlock(&data->lock)

	/* Initialize monitor window. */
	get_lock(data);
	GtkWidget *window;
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	int stride;
	stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24,
						data->freenect_frame_width);
	unsigned char *surface_buffer;
	surface_buffer = malloc(stride * data->freenect_frame_height);
	cairo_surface_t *depth_surface;
	depth_surface = cairo_image_surface_create_for_data(surface_buffer,
						CAIRO_FORMAT_RGB24,
						data->freenect_frame_width,
						data->freenect_frame_height,
						stride);
	size_t i;
	for (i = 0; i < stride * data->freenect_frame_height ; ) {
		surface_buffer[i++] = 0x00; // blue
		surface_buffer[i++] = 0x00; // green
		surface_buffer[i++] = 0x00; // red
		surface_buffer[i++] = 0x00; // unused
	}
	GdkPixbuf *depth_pixbuf;
	depth_pixbuf = gdk_pixbuf_get_from_surface(depth_surface, 0, 0,
						data->freenect_frame_width,
						data->freenect_frame_height);
	GtkWidget *depth_image;
	depth_image = gtk_image_new_from_pixbuf(depth_pixbuf);
	gtk_container_add(GTK_CONTAINER(window), depth_image);
	gtk_widget_show(GTK_WIDGET(depth_image));
	gtk_widget_show(window);
	release_lock(data);

	/* Await for events. */
	gtk_main();

	/* Signal for main thread termination. */
	get_lock(data);
	*data->running = 0;
	release_lock(data);

	return 0;

#undef release_lock
#undef get_lock
#undef data

}

