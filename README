kinect-bleeper
==============

Obstacle avoidance using sounds and openkinect

Usage: kinect_bleeper [OPTION...] 

  -c, --cell=CELL_SIZE       Cell size of pixelization grid [8].
  -f, --fps                  Show average frames per second acquired from
                             device.
  -m, --mirror               Mirror x axis.
  -s, --smooth=SAMPLES       Number of samples for exponential moving average
                             noise reduction [5].
  -w, --monitor              Start the monitor GUI.
  -x, --maxdepth=MAX_DEPTH   Maximum depth in meters [2.5].
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

This routine is a proof-of-concept on using the Microsoft Kinect(tm) sensor as
an assistive technology device for the visually impaired. By parsing Kinect's
depth information stream, this routine detects the nearest object and then
express its relative position in the field of view using sounds.

The horizontal position of the nearest object is coded by the left-right audio
balance of speakers, the vertical position is coded by the tone of bleeps, and
the distance to the nearest object is coded by the repetition rate of bleeps.

Tested in indoor environments only. Engineering efforts are required to address
hardware bulk issues before a setup suited for practical daily usage can be
achieved.

