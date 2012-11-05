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
#include <string.h>

#include "monitor.h"

int stride;
unsigned char *surface_buffer;

void *depth_draw(GtkWidget *wd, cairo_t *cr, void *monitor_data);

#define data ((struct monitor_data *) monitor_data)
#define lock(data) g_mutex_lock(&data->lock)
#define unlock(data) g_mutex_unlock(&data->lock)

void *monitor_thread(void *monitor_data) {

	lock(data);

	/* Initialize surface buffer. */
	stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24,
						data->freenect_frame_width);
	surface_buffer = malloc(stride * data->freenect_frame_height);
	memset(surface_buffer, 0, stride * data->freenect_frame_height);

	/* Initialize monitor window. */
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

	/* Await for events. */
	unlock(data);
	gtk_main();

	/* Signal main thread for termination. */
	lock(data);
	*data->running = 0;
	unlock(data);

	return 0;

}

void *depth_draw(GtkWidget *wd, cairo_t *cr, void *monitor_data) {

	size_t i, j, k;

	/* Update the Cairo surface representing the depth information. */
	lock(data);
	for (j = 0; j < data->freenect_frame_height ; j++ ) {

		k = j * (stride >> 2); // surface buffer pixel column index;
		for (i = 0; i < data->freenect_frame_width ; i++ ) {

			/* Convert depth value to 8-bit gray level. */

#define depth(i,j) data->depth[j * data->freenect_frame_width + i]
#define min data->min_depth
#define max data->max_depth
#define A ((double) 0xFF / (max - min))
#define B ((double) 0xFF * min / (min - max))
#define gray_depth(i,j) (depth(i,j) * A + B)

			long int gray_level = lrint(gray_depth(i, j));

#undef gray_depth
#undef B
#undef A
#undef max
#undef min
#undef depth

			if (gray_level > 0xFF) {
				fprintf(stderr, "alert: pixel value overflow!\n");
				gray_level = 0xFF;
			}

			/* Represent the 8-bit gray level in a 32-bit RGB pixel
			 * where the brighter, the closer. */
			uint32_t gray_value = (uint32_t) 0xFF - gray_level;
			((uint32_t *) surface_buffer)[k++] =
						(gray_value << 8 |
						gray_value) << 8 |
						gray_value;

		}
	}
	cairo_surface_t *new_surface = cairo_image_surface_create_for_data(
						surface_buffer, CAIRO_FORMAT_RGB24,
						data->freenect_frame_width,
						data->freenect_frame_height,
						stride);

	/* Paint the new surface data in the wodget. */
	cairo_set_source_surface(cr, new_surface, 0, 0);
	cairo_paint(cr);

	unlock(data);
	return;

}


#undef unlock
#undef lock
#undef data

