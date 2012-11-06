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

#include "common.h"
#include "monitor.h"

int stride;
unsigned char *surface_buffer;

void *depth_draw(GtkWidget *wd, cairo_t *cr, void *monitor_data);

#define data ((struct monitor_data *) monitor_data)
#define lock(data) pthread_mutex_lock(&data->lock)
#define unlock(data) pthread_mutex_unlock(&data->lock)

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

struct rgb {

	double red;
	double green;
	double blue;

};

void color_for_depth(struct rgb *rgb, double depth, double min_depth, double max_depth);

void *depth_draw(GtkWidget *wd, cairo_t *cr, void *monitor_data) {

	size_t i, j, k;

	/* Update the Cairo surface representing the depth information. */
	lock(data);
	for (j = 0; j < data->freenect_frame_height ; j++) {

		k = j * (stride >> 2); // surface buffer pixel column index;
		for (i = 0; i < data->freenect_frame_width ; i++) {

			/* Convert depth value to 8-bit gray level. */

#define depth(i,j) data->depth[(j) * data->freenect_frame_width + (i)]
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
			((uint32_t *) surface_buffer)[k++] = (gray_value << 8 |
						gray_value) << 8 | gray_value;

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

	/* Mark the nearest position. */
	struct rgb rgb;
	color_for_depth(&rgb, data->nearest_depth, data->min_depth, data->max_depth);
	cairo_set_source_rgb(cr, rgb.red, rgb.green, rgb.blue);
	cairo_set_line_width(cr, 10);
	cairo_arc(cr, data->nearest_coord[0], data->nearest_coord[1], 10, 0, 2 * M_PI);
	cairo_stroke(cr);

	unlock(data);
	return;

}

#undef unlock
#undef lock
#undef data

struct rgb blue_rgb = { 0, 0, 1 };
struct rgb green_rgb = { 0, 1, 0 };
struct rgb yellow_rgb = { 1, 1, 0 };
struct rgb orange_rgb = { 1, 0.5, 0 };
struct rgb red_rgb = { 1, 0, 0 };

void interpolate_rgb(struct rgb *target, struct rgb *rgb_min, struct rgb *rgb_max,
		double value, double value_min, double value_max) {

	double norm = normalize(value, value_min, value_max);
	target->red = interpolate(norm, rgb_min->red, rgb_max->red);
	target->green = interpolate(norm, rgb_min->green, rgb_max->green);
	target->blue = interpolate(norm, rgb_min->blue, rgb_max->blue);

}

void color_for_depth(struct rgb *rgb, double depth, double min_depth, double max_depth) {

#define BLUE_DEPTH (min_depth + (max_depth - min_depth) * 1.00)
#define GREEN_DEPTH (min_depth + (max_depth - min_depth) * 0.75)
#define YELLOW_DEPTH (min_depth + (max_depth - min_depth) * 0.50)
#define ORANGE_DEPTH (min_depth + (max_depth - min_depth) * 0.25)
#define RED_DEPTH (min_depth + (max_depth - min_depth) * 0.00)

	if (depth > BLUE_DEPTH)
		memcpy(rgb, &blue_rgb, sizeof(struct rgb));
	else if (depth > GREEN_DEPTH)
		interpolate_rgb(rgb, &blue_rgb, &green_rgb, depth, BLUE_DEPTH, GREEN_DEPTH);
	else if (depth > YELLOW_DEPTH)
		interpolate_rgb(rgb, &green_rgb, &yellow_rgb, depth, GREEN_DEPTH, YELLOW_DEPTH);
	else if (depth > ORANGE_DEPTH)
		interpolate_rgb(rgb, &yellow_rgb, &orange_rgb, depth, YELLOW_DEPTH, ORANGE_DEPTH);
	else if (depth > RED_DEPTH)
		interpolate_rgb(rgb, &orange_rgb, &red_rgb, depth, ORANGE_DEPTH, RED_DEPTH);
	else
		memcpy(rgb, &red_rgb, sizeof(struct rgb));

}

