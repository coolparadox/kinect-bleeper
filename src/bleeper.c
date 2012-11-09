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
#include <pthread.h>

#include <libfreenect/libfreenect.h>
#include <ao/ao.h>

#include "common.h"
#include "bleep.h"

#define KINECT_DEPTH_FRAME_WIDTH 640
#define KINECT_DEPTH_FRAME_HEIGHT 480
#define KINECT_DEPTH_BITS_PER_DEPTH 11

#define DEPTH_MATRIX_SIZE (KINECT_DEPTH_FRAME_WIDTH * KINECT_DEPTH_FRAME_HEIGHT)

/* Dynamic operation range in meters. */
#define MIN_DEPTH 0.45
#define MAX_DEPTH 10

#define MAX_DEPTH_DEFAULT 2.5

/* EMA bars for pixel noise reduction. */
#define MIN_SMOOTH 1
#define MAX_SMOOTH 50

#define SMOOTH_DEFAULT 5

/* Size of pixelization grid. */
#define MIN_CELL_SIZE 1
#define MAX_CELL_SIZE 120

#define CELL_SIZE_DEFAULT 8

#define STR(X) #X
#define XSTR(X) STR(X)

struct bleep_data bleep_data;

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
	{ "cell", 'c', "CELL_SIZE", 0, "Cell size of pixelization grid ["
						XSTR(CELL_SIZE_DEFAULT) "]." },
	{ "mirror", 'm', 0, 0, "Mirror x axis." },
#ifdef GTK
	{ "monitor", 'w', 0, 0, "Start the monitor GUI." },
#endif // GTK
        { 0 }

};

struct arguments {

	int fps;
	char *max_depth;
	char *smooth;
	char *cell_size;
	int mirror;

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
		case 'c':
			arguments->cell_size = arg;
			break;
		case 'm':
			arguments->mirror = 1;
			break;
#ifdef GTK
		case 'w':
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
int mirror;
int monitor;
double max_depth;
unsigned int smooth;
double smooth_factor;
size_t cell_size;
double z[KINECT_DEPTH_FRAME_WIDTH * KINECT_DEPTH_FRAME_HEIGHT];

int main (int argc, char **argv) {

	size_t i, j;
	struct arguments arguments;
	freenect_context *ctx;
	freenect_device *dev;
	pthread_t thread_bleep;
#ifdef GTK
	pthread_t thread_monitor;
#endif // GTK

	/* Parse CLI arguments. */
	memset(&arguments, 0, sizeof(struct arguments));
        argp_parse (&argp, argc, argv, 0, 0, &arguments);
	fps = arguments.fps;
	mirror = arguments.mirror;
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
	smooth_factor = (double) 2.0 / ((double) smooth + 1);
	if (arguments.cell_size) {

		char *tail;
		cell_size = strtoul(arguments.cell_size, &tail, 10);
		if (*tail) {
			fprintf(stderr, "error: cannot understand '%s' as an "
				"integer decimal number.\n", arguments.cell_size);
			return(1);
		}
		if (cell_size < MIN_CELL_SIZE) {
			fprintf(stderr, "error: CELL_SIZE must be no lesser than "
						"%s.\n", XSTR(MIN_CELL_SIZE));
			return(1);
		}
		if (cell_size > MAX_CELL_SIZE) {
			fprintf(stderr, "error: CELL_SIZE must be no greater than "
						"%s.\n", XSTR(MAX_CELL_SIZE));
			return(1);
		}

	}
	else
		cell_size = CELL_SIZE_DEFAULT;

	/* Hello world. */
	fprintf(stderr, "==========\n");
	fprintf(stderr, "%s\n", PACKAGE_STRING);
	fprintf(stderr, "\n");
	fprintf(stderr, "%s project\n", PACKAGE_NAME);
	fprintf(stderr, "%s\n", PACKAGE_URL);
	fprintf(stderr, "==========\n");
	fprintf(stderr, "dynamic range: %.2lf - %.2lf meters\n", MIN_DEPTH, max_depth);
	fprintf(stderr, "cell size: %lu (%lux%lu grid)\n", cell_size,
				KINECT_DEPTH_FRAME_WIDTH / cell_size,
				KINECT_DEPTH_FRAME_HEIGHT / cell_size);
	fprintf(stderr, "samples for smoothing: %u\n", smooth);

	/* Cleanup on interruption. */
	signal(SIGINT, signal_cleanup);

	/* Initialize kinect device. */
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
	memset(z, 0, sizeof(double) * KINECT_DEPTH_FRAME_WIDTH *
						KINECT_DEPTH_FRAME_HEIGHT);
	freenect_set_depth_callback(dev, process_depth);
	if (freenect_start_depth(dev)) {
		fprintf(stderr, "error: cannot start depth stream.\n");
		goto shutdown;
	}
	fprintf(stderr, "kinect device initialized.\n");

	/* Initialize audio thread. */
	pthread_mutex_init(&bleep_data.lock, NULL);
	bleep_data.x_norm = 0.5;
	bleep_data.y_norm = 0.5;
	bleep_data.z_norm = 1;
	ao_initialize();
	memset(&bleep_data.audio_format, 0, sizeof(ao_sample_format));
	bleep_data.audio_format.bits = 16;
	bleep_data.audio_format.channels = 2;
	bleep_data.audio_format.rate = 44100;
	bleep_data.audio_format.byte_format = AO_FMT_LITTLE;
	pthread_create(&thread_bleep, NULL, &bleep_thread, &bleep_data);
	fprintf(stderr, "audio device initialized.\n");

#ifdef GTK

	/* Initialize GUI monitor. */
	gdk_threads_init();
	gtk_init (&argc, &argv);
	monitor = arguments.monitor;
	if (monitor) {

		/* Start the GUI monitor thread. */
		pthread_mutex_init(&monitor_data.lock, NULL);
		monitor_data.running = &running;
		monitor_data.depth = malloc(sizeof(double) * DEPTH_MATRIX_SIZE);
		for (i = 0; i < DEPTH_MATRIX_SIZE; i++)
			monitor_data.depth[i] = max_depth;
		monitor_data.freenect_frame_width = KINECT_DEPTH_FRAME_WIDTH;
		monitor_data.freenect_frame_height = KINECT_DEPTH_FRAME_HEIGHT;
		monitor_data.freenect_bits_per_depth = KINECT_DEPTH_BITS_PER_DEPTH;
		monitor_data.min_depth = MIN_DEPTH;
		monitor_data.max_depth = max_depth;
		monitor_data.nearest_coord[0] = 0;
		monitor_data.nearest_coord[1] = 0;
		monitor_data.nearest_depth = max_depth;
		monitor_data.depth_widget = NULL;
		monitor_data.smooth = smooth;
		monitor_data.cell_size = cell_size;
		pthread_create(&thread_monitor, NULL, &monitor_thread, &monitor_data);
		fprintf(stderr, "gui monitor started.\n");

	}

#endif // GTK

	/* Process frame events. */
	fprintf(stderr, "processing depth stream...\n");
	while (running && freenect_process_events(ctx) >= 0);
	fprintf(stderr, "shutting down...\n");

shutdown:

	/* Shutdown audio resources. */
	ao_shutdown();

	/* Shutdown kinect device. */
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
	nearest_i = KINECT_DEPTH_FRAME_WIDTH / cell_size / 2;
	nearest_j = KINECT_DEPTH_FRAME_HEIGHT / cell_size / 2;
	nearest_z = max_depth;
	for (j = 0; j < KINECT_DEPTH_FRAME_HEIGHT / cell_size; j++)
	for (i = 0; i < KINECT_DEPTH_FRAME_WIDTH / cell_size; i++) {

		size_t is, js;
		double z_new;
		double z_grid = 0;
		double z_smooth;

		/* Account the depth value for the current grid cell. */
		for (js = 0; js < cell_size; js++)
		for (is = 0; is < cell_size; is++) {

			/* Convert raw disparity values to real world distance
			 * information.  See
			 * http://openkinect.org/wiki/Imaging_Information#Depth_Camera */

#define MIRROR(x) (mirror ? (KINECT_DEPTH_FRAME_WIDTH - (x) - 1) : x)
#define BUFPOS(x,y) ((y) * KINECT_DEPTH_FRAME_WIDTH + (x))
#define DISPARITY(x,y) ((uint16_t *) depth)[BUFPOS(MIRROR(x),y)]

			z_new = tan(DISPARITY(i * cell_size + is, j * cell_size + js)
					/ 2842.5 + 1.1863) * 0.1236 - 0.037;
			if (z_new > max_depth) z_new = max_depth;
			if (z_new < MIN_DEPTH) z_new = max_depth;
			z_grid += z_new;

		}
		z_grid /= pow(cell_size, 2);
		if (z_grid > max_depth) z_grid = max_depth;

		/* Smooth the value of the current grid cell. */
		if (z_grid >= MIN_DEPTH) {
			double z_old = z[BUFPOS(i * cell_size, j * cell_size)];
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

		/* Replicate the current grid cell to the depth matrix. */
		for (is = 0; is < cell_size; is++)
		for (js = 0; js < cell_size; js++)
			z[BUFPOS(i * cell_size + is, j * cell_size + js)] =
									z_smooth;

#undef DISPARITY
#undef BUFPOS
#undef MIRROR

	}

	if (pthread_mutex_trylock(&bleep_data.lock)) {

		bleep_data.x_norm = normalize(nearest_i, 0,
					KINECT_DEPTH_FRAME_WIDTH / cell_size);
		bleep_data.y_norm = normalize(nearest_j, 0,
					KINECT_DEPTH_FRAME_HEIGHT / cell_size);
		bleep_data.z_norm = normalize(nearest_z, MIN_DEPTH, max_depth);
		pthread_mutex_unlock(&bleep_data.lock);

	}

#ifdef GTK

	if (monitor) {

		/* Notify GUI monitor about depth information update. */
		if (pthread_mutex_trylock(&monitor_data.lock)) {
			memcpy(monitor_data.depth, z, sizeof(double) *
							DEPTH_MATRIX_SIZE);
			monitor_data.nearest_coord[0] = nearest_i * cell_size
							+ (cell_size / 2);
			monitor_data.nearest_coord[1] = nearest_j * cell_size
							+ (cell_size / 2);
			monitor_data.nearest_depth = nearest_z;
			pthread_mutex_unlock(&monitor_data.lock);
			gdk_threads_enter();
			gtk_widget_queue_draw(monitor_data.depth_widget);
			gdk_threads_leave();
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

