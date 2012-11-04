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

#include <stdint.h>
#include <gtk/gtk.h>

struct monitor_data {

	GMutex lock;
	volatile sig_atomic_t *running;
	int freenect_frame_width;
	int freenect_frame_height;
	int freenect_bits_per_depth;
	double *depth;
	double min_depth, max_depth;
	GtkWidget *depth_widget;

};

void *monitor_thread(void *thread_data);

