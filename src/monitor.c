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
#include <math.h>

#include "monitor.h"

void *depth_draw(GtkWidget *wd, cairo_t *cr, void *monitor_data);

#define data ((struct monitor_data *) monitor_data)
#define lock(data) g_mutex_lock(&data->lock)
#define unlock(data) g_mutex_unlock(&data->lock)

void *monitor_thread(void *monitor_data) {

	/* Initialize monitor window. */
	lock(data);
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	GtkWidget *depth_widget = gtk_drawing_area_new();
	data->depth_widget = depth_widget;
	gtk_widget_set_size_request(depth_widget, data->freenect_frame_width,
						data->freenect_frame_height);
	gtk_container_add(GTK_CONTAINER(window), depth_widget);
	g_signal_connect(G_OBJECT(depth_widget), "draw",
				G_CALLBACK(depth_draw), monitor_data);
	gtk_widget_show(depth_widget);
	gtk_widget_show(window);
	unlock(data);

	/* Await for events. */
	gtk_main();

	/* Signal main thread for termination. */
	lock(data);
	*data->running = 0;
	unlock(data);

	return 0;

}

#define MAX_DEPTH 4

void *depth_draw(GtkWidget *wd, cairo_t *cr, void *monitor_data) {

	size_t i, j, k;

	/* Remember current image resources to be replaced. */
	cairo_surface_t *current_surface = cairo_get_target(cr);
	unsigned char *current_buffer = cairo_image_surface_get_data(current_surface);

	/* Build a new Cairo surface representing the depth information. */
	lock(data);
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24,
						data->freenect_frame_width);
	unsigned char *new_buffer = malloc(stride * data->freenect_frame_height);
	for (j = 0; j < data->freenect_frame_height ; j++ ) {

		k = j * stride; // surface buffer column index;
		for (i = 0; i < data->freenect_frame_width ; i++ ) {

			/* Convert depth value to 8-bit gray level. */
			double depth = data->depth[j * data->freenect_frame_width + i];
			if (depth < 0) depth = 0;
			if (depth > MAX_DEPTH) {
				depth = MAX_DEPTH;
			}
			long int gray_level = lrint(depth * (double) 0xFF / MAX_DEPTH);
			if (gray_level > 0xFF) {
				fprintf(stderr, "alert: pixel value overflow\n");
				gray_level = 0xFF;
			}
			uint8_t pixel_value = 0xFF - (uint8_t) gray_level;
			if (pixel_value >= 0xFF) pixel_value = 0;

			/* Fill buffer for this RGB pixel. */
			new_buffer[k++] = pixel_value; // blue component
			new_buffer[k++] = pixel_value; // green component
			new_buffer[k++] = pixel_value; // red component
			new_buffer[k++] = (uint8_t) 0; //unused

		}
	}
	cairo_surface_t *new_surface = cairo_image_surface_create_for_data(
						new_buffer, CAIRO_FORMAT_RGB24,
						data->freenect_frame_width,
						data->freenect_frame_height,
						stride);
	unlock(data);

	/* Replace the widget's Cairo surface, and release old buffers. */
	cairo_set_source_surface(cr, new_surface, 0, 0);
	if (!current_surface) {
		cairo_surface_finish(current_surface);
		free(current_buffer);
	}
	cairo_paint(cr);
	return;

}


#undef unlock
#undef lock
#undef data

