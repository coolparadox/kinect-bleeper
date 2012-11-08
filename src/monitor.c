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
#include <stdio.h>

#include "common.h"
#include "monitor.h"

int stride;
unsigned char *surface_buffer;

void *depth_draw(GtkWidget *wd, cairo_t *cr, void *monitor_data);

#define data ((struct monitor_data *) monitor_data)
#define lock(data) pthread_mutex_lock(&data->lock)
#define unlock(data) pthread_mutex_unlock(&data->lock)

#define STRSIZE 1024
#define PADDING 5

void *monitor_thread(void *monitor_data) {

	char str[STRSIZE];

	lock(data);

	/* Initialize surface buffer. */
	stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24,
						data->freenect_frame_width);
	surface_buffer = malloc(stride * data->freenect_frame_height);
	memset(surface_buffer, 0, stride * data->freenect_frame_height);

	/* Initialize monitor window. */
	gdk_threads_enter();
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Kinect Bleeper Monitor");
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	GtkWidget *top_box;
	top_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(window), top_box);
	GtkWidget *box;
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(top_box), box, TRUE, TRUE, PADDING);
	GtkWidget *wd;
	wd = gtk_drawing_area_new();
	data->depth_widget = wd;
	gtk_widget_set_size_request(wd, data->freenect_frame_width,
						data->freenect_frame_height);
	g_signal_connect(G_OBJECT(wd), "draw",
				G_CALLBACK(depth_draw), monitor_data);
	gtk_box_pack_start(GTK_BOX(box), wd, FALSE, FALSE, PADDING);
	GtkTextBuffer *textbuf = gtk_text_buffer_new(NULL);
	snprintf(str, STRSIZE, " * OPERATING PARAMETERS *\n\n");
	gtk_text_buffer_insert_at_cursor(textbuf, str, -1);
	snprintf(str, STRSIZE, "min depth: %.2f meters\n", data->min_depth);
	gtk_text_buffer_insert_at_cursor(textbuf, str, -1);
	snprintf(str, STRSIZE, "max depth: %.2f meters\n", data->max_depth);
	gtk_text_buffer_insert_at_cursor(textbuf, str, -1);
	snprintf(str, STRSIZE, "grid size: %lu x %lu pixels\n",
				data->freenect_frame_width / data->cell_size,
				data->freenect_frame_height / data->cell_size);
	gtk_text_buffer_insert_at_cursor(textbuf, str, -1);
	snprintf(str, STRSIZE, "smoothness: %u samples\n", data->smooth);
	gtk_text_buffer_insert_at_cursor(textbuf, str, -1);
	snprintf(str, STRSIZE, "\n\n==========\n\n");
	gtk_text_buffer_insert_at_cursor(textbuf, str, -1);
	snprintf(str, STRSIZE, "kinect-bleeper project\n");
	gtk_text_buffer_insert_at_cursor(textbuf, str, -1);
	snprintf(str, STRSIZE, "http://coolparadox.github.com\n");
	gtk_text_buffer_insert_at_cursor(textbuf, str, -1);
	wd = gtk_text_view_new_with_buffer(textbuf);
	gtk_widget_set_size_request(wd, 250, data->freenect_frame_height);
	gtk_text_view_set_editable((GtkTextView *) wd, FALSE);
	gtk_text_view_set_cursor_visible((GtkTextView *) wd, FALSE);
	gtk_text_view_set_left_margin((GtkTextView *) wd, 5);
	gtk_box_pack_start(GTK_BOX(box), wd, TRUE, TRUE, 0);
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start(GTK_BOX(top_box), box, TRUE, TRUE, PADDING);
	wd = gtk_button_new_with_label("Quit");
	g_signal_connect(G_OBJECT(wd), "clicked", G_CALLBACK(gtk_main_quit), NULL);
	gtk_box_pack_start(GTK_BOX(box), wd, TRUE, TRUE, PADDING);
	gtk_widget_show_all(window);
	gdk_threads_leave();

	/* Wait for events. */
	unlock(data);
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	/* Signal main thread for termination. */
	lock(data);
	*data->running = 0;
	unlock(data);

	return 0;

}

#undef PADDING
#undef STRSIZE

struct rgb {

	double red;
	double green;
	double blue;

};

void color_for_depth(struct rgb *rgb, double depth, double min_depth, double max_depth);

void *depth_draw(GtkWidget *wd, cairo_t *cr, void *monitor_data) {

	size_t i, j, k;
	double *depth;

	lock(data);
	size_t frame_height = data->freenect_frame_height;
	size_t frame_width = data->freenect_frame_width;
	double min_depth = data->min_depth;
	double max_depth = data->max_depth;
	size_t nearest_i = data->nearest_coord[0];
	size_t nearest_j = data->nearest_coord[1];
	double nearest_depth = data->nearest_depth;
#define DEPTH_SIZE (sizeof(double) * frame_height * frame_width)
	depth = malloc(DEPTH_SIZE);
	memcpy(depth, data->depth, DEPTH_SIZE);
#undef DEPTH_SIZE
	unlock(data);

	/* Update the Cairo surface representing the depth information. */
	for (j = 0; j < frame_height ; j++) {

		k = j * (stride >> 2); // surface buffer pixel column index;
		for (i = 0; i < frame_width ; i++) {

			/* Convert depth value to 8-bit gray level. */

#define DEPTH(i,j) depth[(j) * frame_width + (i)]
#define A ((double) 0xFF / (max_depth - min_depth))
#define B ((double) 0xFF * min_depth / (min_depth - max_depth))
#define gray_depth(i,j) (DEPTH(i,j) * A + B)

			long int gray_level = lrint(gray_depth(i, j));

#undef gray_depth
#undef B
#undef A
#undef DEPTH

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
	free(depth);
	cairo_surface_t *new_surface = cairo_image_surface_create_for_data(
						surface_buffer, CAIRO_FORMAT_RGB24,
						frame_width,
						frame_height,
						stride);

	/* Paint the new surface data in the wodget. */
	cairo_set_source_surface(cr, new_surface, 0, 0);
	cairo_paint(cr);

	/* Mark the nearest position. */
	struct rgb rgb;
	color_for_depth(&rgb, nearest_depth, min_depth, max_depth);
	cairo_set_source_rgb(cr, rgb.red, rgb.green, rgb.blue);
	cairo_set_line_width(cr, 10);
	cairo_arc(cr, nearest_i, nearest_j, 10, 0, 2 * M_PI);
	cairo_stroke(cr);

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

