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
#include <string.h>
#include <ao/ao.h>

#include "common.h"
#include "bleep.h"

#define SLEEP_MIN 0.2
#define SLEEP_MAX 2
#define BLEEP_SECOND_FRACTION 10

#define FREQ_MIN 400
#define FREQ_MAX 4000

#define bleep_data ((struct bleep_data *) thread_data)
#define lock(x) pthread_mutex_lock(&x->lock)
#define unlock(x) pthread_mutex_unlock(&x->lock)
#define audio_format (bleep_data->audio_format)

void *bleep_thread(void *thread_data) {

	double x, y, z;
	size_t i;
	unsigned int sleep_min_usec, sleep_max_usec;
	char *audio_buf;
	size_t audio_bufsize;
	int audio_driver;
	ao_sample_format sample_format;
	ao_device *audio_device;

	sleep_min_usec = lrint((double) SLEEP_MIN * 1000000);
	sleep_max_usec = lrint((double) SLEEP_MAX * 1000000);
	audio_bufsize = audio_format.bits/8 * audio_format.channels *
				audio_format.rate / BLEEP_SECOND_FRACTION;
	audio_buf = calloc(audio_bufsize, sizeof(char));
	memcpy(&sample_format, &audio_format, sizeof(ao_sample_format));
	audio_driver = ao_default_driver_id();
	while (1) {

		/* Sleep for some time according to the target proximity. */
		lock(bleep_data);
		z = bleep_data->z_norm;
		unlock(bleep_data);
		usleep(lrint(interpolate(z, sleep_min_usec, sleep_max_usec)));

		/* Time to bleep once. */
		lock(bleep_data);
		x = bleep_data->x_norm;
		y = bleep_data->y_norm;
		unlock(bleep_data);
		for (i = 0; i < audio_format.rate / BLEEP_SECOND_FRACTION; i++) {

			/* Synthesize a tone with frequency as a function of the
			 * vertical target's coordinate. */
			double freq = interpolate(y, FREQ_MIN, FREQ_MAX);
			int sample = lrint((double) 0.75 * 32768.0 * sin(2 *
				M_PI * freq * ((double) i / audio_format.rate)));

			/* Add the same sound to the left and right channels,
			 * balancing according to the horizontal target's coordinate. */
			int left = lrint((double) sample * ((double) 1 - x));
			int right = lrint((double) sample * x);
			audio_buf[4*i] = left & 0xff;
			audio_buf[4*i+1] = (left >> 8) & 0xff;
			audio_buf[4*i+2] = right & 0xff;
			audio_buf[4*i+3] = (right >> 8) & 0xff;

		}
		audio_device = ao_open_live(audio_driver, &audio_format, NULL);
		if (audio_device) {
			ao_play(audio_device, audio_buf, audio_bufsize);
			ao_close(audio_device);
		}
		else {
			fprintf(stderr, "error: cannot open audio device.\n");
		}

	}

}

