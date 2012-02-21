lib_LTLIBRARIES = libhagraph.la
libhagraph_la_SOURCES = libhagraph.c libhagraph_data.c configfile.c
libhagraphinclude_HEADERS = libhagraph.h libhagraph_data.h configfile.h
libhagraphincludedir=$(includedir)/libhagraph-0.2/libhagraph/

LIBS = @gtk_LIBS@ @cairo_LIBS@
AM_CFLAGS = @CFLAGS@ @gtk_CFLAGS@ @cairo_CFLAGS@ -I/usr/include/postgresql
