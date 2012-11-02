/*
 * kinect_bleeper - obstacle avoidance using sounds and openkinect
 *
 * Copyright (c) 2012 coolparadox.com
 *
 * This code is licenced according to the GNU General Public Licence version 3.
 * A file COPYING is distributed with the sources of this project containing
 * the full text of the license. Optionally, the full terms of the license 
 * can also be downloaded from http://www.gnu.org/licenses/gpl.html .
 *
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <argp.h>
#include <signal.h>
#include <math.h>

#include <libfreenect/libfreenect.h>

#define KINETIC_DEPTH_FRAME_WIDTH 640
#define KINETIC_DEPTH_FRAME_HEIGHT 480
#define KINETIC_DEPTH_BITS_PER_DEPTH 11

#define KINETIC_DEPTH_FRAME_SIZE (sizeof(uint16_t) * KINETIC_DEPTH_FRAME_WIDTH * KINETIC_DEPTH_FRAME_HEIGHT)
#define DEPTH_MATRIX_SIZE (sizeof(double) * KINETIC_DEPTH_FRAME_WIDTH * KINETIC_DEPTH_FRAME_HEIGHT)

#ifdef GTK

#include <gtk/gtk.h>
#include "monitor.h"

struct monitor_data monitor_data;

#endif // GTK

volatile sig_atomic_t running = 1;

/*
 * Command line argument parsing functions.
 */

const char *argp_program_version = PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = "kinect_bleeper -- obstacle avoidance using sounds and "
			"openkinect\vThis routine in not yet implemented.\n";

static char args_doc[] = "";

static struct argp_option options[] = {

        { 0 }

};

struct arguments {

	char *dummy;

};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {

        struct arguments *arguments = state->input;
        switch (key) {
		case ARGP_KEY_ARG:
			switch (state->arg_num) {
				default:
					argp_usage (state);
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return (0);
}

static struct argp argp = { options, parse_opt, args_doc, doc };

void process_depth(freenect_device *dev, void *depth, uint32_t timestamp);

void signal_cleanup(int num) {

	running = 0;
	fprintf(stderr, "\ncleaning up...\n");
	signal(SIGINT, signal_cleanup);

}

/*
 * The portal.
 */

int main (int argc, char **argv) {

	struct arguments arguments;

	freenect_context *ctx;
	freenect_device *dev;

#ifdef GTK
	/* Initialize gtk library. */
	gtk_init (&argc, &argv);
#endif // GTK

	/* Get CLI arguments. */
        memset (&arguments, 0, sizeof (struct arguments));
        argp_parse (&argp, argc, argv, 0, 0, &arguments);

	/* Cleanup on interruption. */
	signal(SIGINT, signal_cleanup);

	/* Initialize kinetic device. */
	if (freenect_init(&ctx, NULL)) {
		fprintf(stderr, "error: cannot initialize freenect context.\n");
		return (1);
	}
	freenect_select_subdevices(ctx, FREENECT_DEVICE_CAMERA);
	if (freenect_open_device(ctx, &dev, 0)) {
		fprintf(stderr, "error: cannot open kinect device.\n");
		return (1);
	}
	if (freenect_set_depth_mode(dev, freenect_find_depth_mode(
			FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT))) {
		fprintf(stderr, "error: cannot set depth mode.\n");
		goto shutdown;
	}
	freenect_set_depth_callback(dev, process_depth);
	if (freenect_start_depth(dev)) {
		fprintf(stderr, "error: cannot start depth stream.\n");
		goto shutdown;
	}

#ifdef GTK

	/* Start the GUI monitor. */
	g_mutex_init(&monitor_data.lock);
	monitor_data.running = &running;
	monitor_data.freenect_frame_width = KINETIC_DEPTH_FRAME_WIDTH;
	monitor_data.freenect_frame_height = KINETIC_DEPTH_FRAME_HEIGHT;
	monitor_data.freenect_bits_per_depth = KINETIC_DEPTH_BITS_PER_DEPTH;
	monitor_data.depth = malloc(DEPTH_MATRIX_SIZE);
	memset(monitor_data.depth, 0, DEPTH_MATRIX_SIZE);
	monitor_data.depth_widget = NULL;
	g_thread_new("monitor", monitor_thread, &monitor_data);

#endif // GTK

	/* Process frame events. */
	while (running && freenect_process_events(ctx) >= 0);

shutdown:

	/* Shutdown kinetic device. */
	if (freenect_stop_depth(dev)) {
		fprintf(stderr, "warning: cannot stop depth stream.\n");
	}
	if (freenect_close_device(dev)) {
		fprintf(stderr, "warning: cannot close freenect device.\n");
	}
	if (freenect_shutdown(ctx)) {
		fprintf(stderr, "warning: cannot shutdown freenect context.\n");
	}
	
}

void process_depth(freenect_device *dev, void *depth, uint32_t timestamp) {

	size_t i, j;

	/* Convert raw disparity and 2D offset values to real world distance information.
	See http://openkinect.org/wiki/Imaging_Information#Depth_Camera */

#define WIDTH KINETIC_DEPTH_FRAME_WIDTH
#define HEIGHT KINETIC_DEPTH_FRAME_HEIGHT

	static double x[WIDTH][HEIGHT];
	static double y[WIDTH][HEIGHT];
	static double z[WIDTH][HEIGHT];

	for (i = 0; i < WIDTH; i++)
		for (j = 0; j < HEIGHT; j++) {

#define minDistance -10
#define scaleFactor 0.0021
#define disparity(i, j) ((uint16_t *) depth)[i * HEIGHT + j]

			z[i][j] = tan(disparity(i, j) / 2842.5 + 1.1863)
							* 0.1236 - 0.037;
			x[i][j] = ((double) i - WIDTH / 2) * (z[i][j] + minDistance)
						* scaleFactor * (WIDTH / HEIGHT);
			y[i][j] = ((double) j - HEIGHT / 2) * (z[i][j] + minDistance)
								* scaleFactor;

#undef disparity
#undef scaleFactor
#undef minDistance
#undef HEIGHT
#undef WIDTH

		}

#ifdef GTK

	/* Notify GUI monitor about depth information update. */
	if (g_mutex_trylock(&monitor_data.lock)) {
		memcpy(monitor_data.depth, z, DEPTH_MATRIX_SIZE);
		g_mutex_unlock(&monitor_data.lock);
		if (monitor_data.depth_widget)
			gtk_widget_queue_draw(monitor_data.depth_widget);
	}

#endif // GTK

	printf ("%u\n", timestamp);

}

