bin_PROGRAMS = kinect_bleeper

if GTK
kinect_bleeper_SOURCES = kinect_bleeper.c monitor.h monitor.c
else
kinect_bleeper_SOURCES = kinect_bleeper.c
endif

kinect_bleeper_CFLAGS = @GTK_CFLAGS@
kinect_bleeper_LDFLAGS = @GTK_LDFLAGS@

MAINTAINERCLEANFILES = Makefile.in

