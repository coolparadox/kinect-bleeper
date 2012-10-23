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

#include <libfreenect/libfreenect.h>

volatile sig_atomic_t running = 1;

/*
 * Command line argument parsing functions.
 */

const char *argp_program_version = PACKAGE_VERSION;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = "kinect_bleeper -- obstacle avoidance using sounds and openkinect\v"
	"This routine in not yet implemented.\n";

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
	if (freenect_set_depth_mode(dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT))) {
		fprintf(stderr, "error: cannot set depth mode.\n");
		goto shutdown;
	}
	freenect_set_depth_callback(dev, process_depth);
	if (freenect_start_depth(dev)) {
		fprintf(stderr, "error: cannot start depth stream.\n");
		goto shutdown;
	}

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

	/* FIXME: implement depth stream processing here. */
	printf ("%u\n", timestamp);

}

