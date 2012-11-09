/*
 * kinect_bleeper audio synthesis include file
 *
 * Copyright (c) 2012 coolparadox.com
 *
 * This code is licenced according to the GNU General Public Licence version 3.
 * A file COPYING is distributed with the sources of this project containing
 * the full text of the license. Optionally, the full terms of the license 
 * can also be downloaded from http://www.gnu.org/licenses/gpl.html .
 *
 */

#include <pthread.h>
#include <ao/ao.h>

struct bleep_data {

	pthread_mutex_t lock;
	ao_sample_format audio_format;
	double x_norm, y_norm, z_norm;

};

void *bleep_thread(void *thread_data);

