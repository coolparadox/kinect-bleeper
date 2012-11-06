/*
 * kinect_bleeper audio synthesis
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
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "common.h"
#include "bleep.h"

#define bleep_data ((struct bleep_data *) thread_data)
#define lock(x) pthread_mutex_lock(&x->lock)
#define unlock(x) pthread_mutex_unlock(&x->lock)

void *bleep_thread(void *thread_data) {

	double x, y, z;
	char play_command[256];

	while (1) {

		/* Gather information on depth position to be broadcasted.
		 * x, y, z are assumed to be normalized. */
		lock(bleep_data);
		x = bleep_data->x;
		y = bleep_data->y;
		z = bleep_data->z;
		unlock(bleep_data);

		/* Sleep for some time according to the target proximity. */
		usleep(lrint(interpolate(z, 500000, 5000000)));

		/* Notify stdout to bleep once. */
		double freq = interpolate(y, 400, 4000);
		//printf("%.0f %.2f %.2f\n",
		sprintf(play_command, "play -q -n synth 0.1 sine %.0f sine %.0f "
			"mixer %.2f,0,0,%.2f\n", freq, freq, (double) 1 - x, x);
		system(play_command);

	}

	return;

}

