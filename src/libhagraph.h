#ifndef __HAGRAPH_H__
#define __HAGRAPH_H__

#include <gtk/gtk.h>
#include "libhagraph_data.h"

extern void drawGraphGtk(GtkWidget *widget, struct _graph_data *graph);
extern void drawGraphPng(char *filename, struct _graph_data *graph, int width, int height);

#endif

