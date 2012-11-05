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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <argp.h>
#include <signal.h>
#include <math.h>

#include <libfreenect/libfreenect.h>

#define KINETIC_DEPTH_FRAME_WIDTH 640
#define KINETIC_DEPTH_FRAME_HEIGHT 480
#define KINETIC_DEPTH_BITS_PER_DEPTH 11

#define DEPTH_MATRIX_SIZE (sizeof(double) * KINETIC_DEPTH_FRAME_WIDTH * KINETIC_DEPTH_FRAME_HEIGHT)

/* Dynamic operation range in meters. */
#define MIN_DEPTH 0.45
#define MAX_DEPTH 10

#define MAX_DEPTH_DEFAULT 2.5

/* EMA bars for pixel noise reduction. */
#define MIN_SMOOTH 1
#define MAX_SMOOTH 100

#define SMOOTH_DEFAULT 10

/* Size of pixelization grid. */
#define MIN_GRID_SIZE 1
#define MAX_GRID_SIZE 160

#define GRID_SIZE_DEFAULT 10

#define STR(X) #X
#define XSTR(X) STR(X)

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
	{ "fps", 'f', 0, 0, "Show average frames per second acquired from "
								"device." },
	{ "maxdepth", 'x', "MAX_DEPTH", 0, "Maximum depth in meters ["
						XSTR(MAX_DEPTH_DEFAULT) "]." },
	{ "smooth", 's', "SAMPLES", 0, "Number of samples for exponential "
		"moving average noise reduction [" XSTR(SMOOTH_DEFAULT) "]." },
	{ "grid", 'g', "GRID_SIZE", 0, "Size of pixelization grid ["
						XSTR(GRID_SIZE_DEFAULT) "]." },
#ifdef GTK
	{ "monitor", 'm', 0, 0, "Start the monitor GUI." },
#endif // GTK
        { 0 }

};

struct arguments {

	int fps;
	char *max_depth;
	char *smooth;
	char *grid_size;

#ifdef GTK
	int monitor;
#endif // GTK


};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {

        struct arguments *arguments = state->input;
        switch (key) {
		case 'f':
			arguments->fps = 1;
			break;
		case 'x':
			arguments->max_depth = arg;
			break;
		case 's':
			arguments->smooth = arg;
			break;
		case 'g':
			arguments->grid_size = arg;
			break;
#ifdef GTK
		case 'm':
			arguments->monitor = 1;
			break;
#endif // GTK
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
	signal(SIGINT, signal_cleanup);

}

/*
 * The portal.
 */

int fps;
int monitor;
double max_depth;
unsigned long int smooth;
double smooth_factor;
size_t grid_size;
double z[KINETIC_DEPTH_FRAME_WIDTH * KINETIC_DEPTH_FRAME_HEIGHT];

int main (int argc, char **argv) {

	size_t i, j;

	struct arguments arguments;

	freenect_context *ctx;
	freenect_device *dev;

	/* Parse CLI arguments. */
	memset(&arguments, 0, sizeof(struct arguments));
        argp_parse (&argp, argc, argv, 0, 0, &arguments);
	fps = arguments.fps;
	if (arguments.max_depth) {

		char *tail;
		max_depth = strtod(arguments.max_depth, &tail);
		if (*tail) {
			fprintf(stderr, "error: cannot understand '%s' as a "
				"float decimal number.\n", arguments.max_depth);
			return(1);
		}
		if (max_depth < MIN_DEPTH) {
			fprintf(stderr, "error: MAX_DEPTH must be no lesser than "
							"%s.\n", XSTR(MIN_DEPTH));
			return(1);
		}
		if (max_depth > MAX_DEPTH) {
			fprintf(stderr, "error: MAX_DEPTH must be no greater than "
							"%s.\n", XSTR(MAX_DEPTH));
			return(1);
		}

	}
	else
		max_depth = MAX_DEPTH_DEFAULT;
	fprintf(stderr, "dynamic range: %.2lf - %.2lf meters\n", MIN_DEPTH, max_depth);
	if (arguments.smooth) {

		char *tail;
		smooth = strtoul(arguments.smooth, &tail, 10);
		if (*tail) {
			fprintf(stderr, "error: cannot understand '%s' as an "
				"integer decimal number.\n", arguments.smooth);
			return(1);
		}
		if (smooth < MIN_SMOOTH) {
			fprintf(stderr, "error: SAMPLES must be no lesser than "
							"%s.\n", XSTR(MIN_SMOOTH));
			return(1);
		}
		if (smooth > MAX_SMOOTH) {
			fprintf(stderr, "error: SAMPLES must be no greater than "
							"%s.\n", XSTR(MAX_SMOOTH));
			return(1);
		}

	}
	else
		smooth = SMOOTH_DEFAULT;
	fprintf(stderr, "samples for smoothing: %lu\n", smooth);
	smooth_factor = (double) 2.0 / ((double) smooth + 1);
	if (arguments.grid_size) {

		char *tail;
		grid_size = strtoul(arguments.grid_size, &tail, 10);
		if (*tail) {
			fprintf(stderr, "error: cannot understand '%s' as an "
				"integer decimal number.\n", arguments.grid_size);
			return(1);
		}
		if (grid_size < MIN_GRID_SIZE) {
			fprintf(stderr, "error: GRID_SIZE must be no lesser than "
						"%s.\n", XSTR(MIN_GRID_SIZE));
			return(1);
		}
		if (grid_size > MAX_GRID_SIZE) {
			fprintf(stderr, "error: GRID_SIZE must be no greater than "
						"%s.\n", XSTR(MAX_GRID_SIZE));
			return(1);
		}

	}
	else
		grid_size = GRID_SIZE_DEFAULT;
	fprintf(stderr, "pixelization grid size: %lu\n", grid_size);

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
	fprintf(stderr, "kinetic device found.\n");
	if (freenect_set_depth_mode(dev, freenect_find_depth_mode(
			FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT))) {
		fprintf(stderr, "error: cannot set depth mode.\n");
		goto shutdown;
	}
	memset(z, 0, sizeof(double) * KINETIC_DEPTH_FRAME_WIDTH * KINETIC_DEPTH_FRAME_HEIGHT);
	freenect_set_depth_callback(dev, process_depth);
	if (freenect_start_depth(dev)) {
		fprintf(stderr, "error: cannot start depth stream.\n");
		goto shutdown;
	}
	fprintf(stderr, "depth stream initialized\n");

#ifdef GTK

	/* Initialize GUI monitor. */
	gtk_init (&argc, &argv);
	monitor = arguments.monitor;
	if (monitor) {

		/* Start the GUI monitor thread. */
		g_mutex_init(&monitor_data.lock);
		monitor_data.running = &running;
		monitor_data.freenect_frame_width = KINETIC_DEPTH_FRAME_WIDTH;
		monitor_data.freenect_frame_height = KINETIC_DEPTH_FRAME_HEIGHT;
		monitor_data.freenect_bits_per_depth = KINETIC_DEPTH_BITS_PER_DEPTH;
		monitor_data.depth = malloc(DEPTH_MATRIX_SIZE);
		monitor_data.min_depth = MIN_DEPTH;
		monitor_data.max_depth = max_depth;
		monitor_data.nearest_coord[0] = 0;
		monitor_data.nearest_coord[1] = 0;
		monitor_data.nearest_depth = max_depth;
		memset(monitor_data.depth, 0, DEPTH_MATRIX_SIZE);
		monitor_data.depth_widget = NULL;
		g_thread_new("monitor", monitor_thread, &monitor_data);
		fprintf(stderr, "gui monitor started.\n");

	}

#endif // GTK

	/* Process frame events. */
	fprintf(stderr, "processing stream...\n");
	while (running && freenect_process_events(ctx) >= 0);
	fprintf(stderr, "shutting down...\n");

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

	size_t nearest_i, nearest_j;
	double nearest_z;

	/* Travel through the pixelated grid. */
	nearest_i = KINETIC_DEPTH_FRAME_WIDTH / grid_size / 2;
	nearest_j = KINETIC_DEPTH_FRAME_HEIGHT / grid_size / 2;
	nearest_z = max_depth;
	for (j = 0; j < KINETIC_DEPTH_FRAME_HEIGHT / grid_size; j++)
	for (i = 0; i < KINETIC_DEPTH_FRAME_WIDTH / grid_size; i++) {

		size_t is, js;
		double z_new;
		double z_grid = 0;
		double z_smooth;

		/* Account the depth value for the current grid cell. */
		for (js = 0; js < grid_size; js++)
		for (is = 0; is < grid_size; is++) {

			/* Convert raw disparity values to real world distance
			 * information.  See
			 * http://openkinect.org/wiki/Imaging_Information#Depth_Camera */

#define BUFPOS(x,y) ((y) * KINETIC_DEPTH_FRAME_WIDTH + (x))
#define DISPARITY(x,y) ((uint16_t *) depth)[BUFPOS(x,y)]

			z_new = tan(DISPARITY(i * grid_size + is, j * grid_size + js)
					/ 2842.5 + 1.1863) * 0.1236 - 0.037;
			if (z_new > max_depth) z_new = max_depth;
			if (z_new < MIN_DEPTH) z_new = max_depth;
			z_grid += z_new;

		}
		z_grid /= pow(grid_size, 2);
		if (z_grid > max_depth) z_grid = max_depth;

		/* Smooth the value of the current grid cell. */
		if (z_grid >= MIN_DEPTH) {
			double z_old = z[BUFPOS(i * grid_size, j * grid_size)];
			z_smooth = smooth_factor * z_grid +
						(1 - smooth_factor) * z_old;
			}
		else
			z_smooth = max_depth;

		/* Discover the nearest grid position. */
		if (z_smooth < nearest_z) {

			nearest_i = i;
			nearest_j = j;
			nearest_z = z_smooth;

		}

#ifdef GTK

		/* Replicate the current grid cell to the depth matrix. */
		for (is = 0; is < grid_size; is++)
		for (js = 0; js < grid_size; js++)
			z[BUFPOS(i * grid_size + is, j * grid_size + js)] = z_smooth;

#endif // GTK

#undef DISPARITY
#undef BUFPOS

	}

#ifdef GTK

	if (monitor) {

		/* Notify GUI monitor about depth information update. */
		if (g_mutex_trylock(&monitor_data.lock)) {
			memcpy(monitor_data.depth, z, DEPTH_MATRIX_SIZE);
			monitor_data.nearest_coord[0] = nearest_i * grid_size
							+ (grid_size / 2);
			monitor_data.nearest_coord[1] = nearest_j * grid_size
							+ (grid_size / 2);
			monitor_data.nearest_depth = nearest_z;
			g_mutex_unlock(&monitor_data.lock);
			if (monitor_data.depth_widget)
				gtk_widget_queue_draw(monitor_data.depth_widget);
		}

	}

#endif // GTK

	if (fps) {

		/* Account and show the average frames per seconds. */
		static size_t frame_count = 0;
		static unsigned int fps_same_second = 0;
		frame_count++;
		if (!(time(NULL) % 10)) {
			if (!fps_same_second) {
				fprintf(stderr, "FPS = %.1f\n",
						(double) frame_count / 10);
				frame_count = 0;
			}
			fps_same_second = 1;
		}
		else
			fps_same_second = 0;

	}

}

