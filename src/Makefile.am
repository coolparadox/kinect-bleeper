bin_PROGRAMS = kinect_bleeper

if GTK
kinect_bleeper_SOURCES = bleeper.c bleep.c monitor.c common.c
else
kinect_bleeper_SOURCES = bleeper.c bleep.c common.c
endif

kinect_bleeper_CFLAGS = -pthread @GTK_CFLAGS@
kinect_bleeper_LDFLAGS = -pthread -lm @GTK_LDFLAGS@

MAINTAINERCLEANFILES = Makefile.in

