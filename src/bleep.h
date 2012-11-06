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

struct bleep_data {

	pthread_mutex_t lock;
	double x, y, z;

};

void *bleep_thread(void *thread_data);

