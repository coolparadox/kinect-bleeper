/*
 * kinect_bleeper common functions
 *
 * Copyright (c) 2012 coolparadox.com
 *
 * This code is licenced according to the GNU General Public Licence version 3.
 * A file COPYING is distributed with the sources of this project containing
 * the full text of the license. Optionally, the full terms of the license 
 * can also be downloaded from http://www.gnu.org/licenses/gpl.html .
 *
 */

#include "common.h"

double normalize(double value, double min, double max) {

#define A ((double) 1 / (max - min))
#define B (min / (min - max))

	return(value * A + B);

#undef B
#undef A

}

double interpolate(double norm, double min, double max) {

#define A (max - min)
#define B min

	return(norm * A + B);

#undef B
#undef A

}

