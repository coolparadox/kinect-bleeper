/* kinect_bleeper - obstacle avoidance using sounds and openkinect
 * by Rafael Lorandi <coolparadox@gmail.com>
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <argp.h>

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

/*
 * The portal.
 */

int main (int argc, char **argv) {

	struct arguments arguments;

	/* Get CLI arguments. */
        memset (&arguments, 0, sizeof (struct arguments));
        argp_parse (&argp, argc, argv, 0, 0, &arguments);

	/* Sanity check CLI arguments. */

	/* Under construction. */
	fprintf (stderr, "error: this routine is not yet implemented.\n");
	return (1);

}

