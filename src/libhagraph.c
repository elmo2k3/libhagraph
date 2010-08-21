/*
 * Copyright (C) 2007-2010 Bjoern Biesenbach <bjoern@bjoern-b.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <cairo/cairo.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "libhagraph.h"
#include "graph_names.h"
#include "../version.h"

#define width_STD 800
#define height_STD 600

#define X1_SKIP 40
#define X2_SKIP 40
//#define Y1_SKIP 100
#define Y2_SKIP 40
#define X1_TO_TEXT 25
#define X1_TO_TEXT2 5
#define TICK_OFFSET 10

static int Y1_SKIP;
static int Y1_TO_TEXT;
static int Y1_TO_LEGEND;

#define WIDTH_FOR_ONE_HOUR ((double)(width-X1_SKIP-X2_SKIP)/24)
#define WIDTH_FOR_ONE_DAY_IN_WEEK ((double)(width-X1_SKIP-X2_SKIP)/7)
#define WIDTH_FOR_ONE_DAY_IN_MONTH ((double)(width-X1_SKIP-X2_SKIP)/31)
#define WIDTH_FOR_ONE_DAY_IN_YEAR ((double)(width-X1_SKIP-X2_SKIP)/365)
#define WIDTH_FOR_ONE_MONTH_IN_YEAR ((double)(width-X1_SKIP-X2_SKIP)/12)

static const int colors[][3] = {{255,0,0},
            {0,255,0},
            {0,0,255},
            {255,255,0},
            {0,0,0},
            {255,0,0},
            {255,0,0},
            {0,255,0},
            {0,0,255},
            {255,255,0},
            {0,0,0},
            {0,255,0},
            {0,0,255},
            {255,255,0},
            {0,0,0},
            {255,0,0},
            {255,0,0},
            {0,255,0},
            {0,0,255},
            {255,255,0},
            {0,0,0},
            {0,255,0},
            {0,0,255},
            {255,0,0}};

// map of names for module/sensor combinations

static void drawGraph(cairo_t *cr, struct _graph_data *graph, int width, int height);
static void drawXLegend(cairo_t *cr, char timebase, const char *title, int width, int height);
static void drawYLegend(cairo_t *cr, double temp_max, double temp_min, int width, int height);
static double transformX(time_t x, int timebase, int width);
static double transformY(double temperature, double max, double min, int height);

char *libhagraphVersion(void)
{
    return LIBHAGRAPH_VERSION;
}

void drawGraphGtk(GtkWidget *widget, struct _graph_data *graph)
{
    gint width,height;
    cairo_t *cr;
    
    GdkWindow *window = widget->window;
    gdk_window_get_geometry(window,0,0,&width,&height,0);
    cr = gdk_cairo_create(widget->window);
    
    drawGraph(cr, graph, width, height);
    
    cairo_destroy(cr);
}

void drawGraphPng(char *filename, struct _graph_data *graph, int width, int height)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create(surface);

    cairo_rectangle(cr, 0,0,width,height);
    cairo_stroke_preserve(cr);
    cairo_set_source_rgb(cr, 255,255,255);
    cairo_fill(cr);

    drawGraph(cr, graph, width, height);
    
    cairo_surface_write_to_png(surface, filename);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void drawGraph(cairo_t *cr, struct _graph_data *graph, int width, int height)
{
    int c,i;
    char min_max_avg[40];
    int text_x, text_y;
    int max_label_length = 0;
    cairo_text_extents_t extents;
    
    if(readGraphNameFile("/etc/hagraphs.conf"))
    {
        if(readGraphNameFile("hagraphs.conf"))
        {
            fprintf(stderr,"could not find hagraphs.conf");
            return;
        }
    }
    
    Y1_SKIP = 10 * graph->num_graphs + 50;
    Y1_TO_TEXT = Y1_SKIP - 20;
    Y1_TO_LEGEND = Y1_SKIP - 40;
    
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12.0);

    for(c=0; c < graph->num_graphs;c++)
    {
        cairo_text_extents(cr, getGraphName(graph->graphs[c].modul,graph->graphs[c].sensor), &extents);
        if((int)extents.width > max_label_length)
            max_label_length = (int)extents.width;
    }

    for(c=0;c < graph->num_graphs;c++)
    {
        cairo_set_line_width(cr, 2);
        cairo_set_source_rgb(cr, colors[c][0], colors[c][1], colors[c][2]);
        
        for(i=0; i < graph->graphs[c].num_points; i++)
        {
            if(!i)
                cairo_move_to(cr, transformX((time_t)graph->graphs[c].points[i].x, 
                        graph->view, width), 
                        transformY(graph->graphs[c].points[i].y, graph->max, 
                        graph->min, height));
            else
                cairo_line_to(cr, transformX((time_t)graph->graphs[c].points[i].x, 
                        graph->view, width), 
                        transformY(graph->graphs[c].points[i].y, 
                        graph->max, 
                        graph->min, height));
        }
        cairo_stroke(cr);
        
        sprintf(min_max_avg," Max: %2.02f Min: %2.02f Avg: %2.02f",
                graph->graphs[c].max,
                graph->graphs[c].min,
                graph->graphs[c].average);

        if(c%2) // 1 .. 3 .. 5 ..
        {
            text_x = X1_SKIP + (width/2);
            text_y = height - Y1_TO_LEGEND + (c-1)*10;
        }
        else // 0 .. 2 .. 4 ..
        {
            text_x = X1_SKIP;
            text_y = height - Y1_TO_LEGEND + c*10;
        }

        cairo_rectangle(cr, text_x, text_y, 10, 10);
        cairo_stroke_preserve(cr);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 0,0,0);
        cairo_move_to(cr, text_x + 15, text_y + 10);
        cairo_show_text(cr,getGraphName(graph->graphs[c].modul,graph->graphs[c].sensor));
        cairo_move_to(cr, text_x + max_label_length + 25, text_y + 10);
        cairo_show_text(cr,min_max_avg);
    }
    
    drawXLegend(cr, graph->view, graph->time_from, width, height);
    drawYLegend(cr, graph->max, graph->min, width, height);
}


/* 
 * X-Achse zeichnen
 * Möglichkeiten für timebase: TB_DAY, TB_WEEK, TB_MONTH, TB_YEAR
 * 
 */
static void drawXLegend(cairo_t *cr, char timebase, const char *title, int width, int height)
{
    int i,p;
    char time[200];
    double space;

    strcpy(time, title);

    cairo_set_line_width(cr, 2);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, X1_SKIP-5, height-Y1_SKIP);
    cairo_line_to(cr, width-X2_SKIP+5, height-Y1_SKIP);
    cairo_stroke(cr);

    switch(timebase)
    {
        case TB_DAY:    if(width<2000)
                {
                    space = (double)(WIDTH_FOR_ONE_HOUR*2);i=0,p=13;
                }
                else
                {
                    space = (double)(WIDTH_FOR_ONE_HOUR); i=0; p=25;
                }
                break;
        case TB_WEEK:   space = WIDTH_FOR_ONE_DAY_IN_WEEK; i=0; p=8; break;
        case TB_MONTH:  time[7] = '\0';
                space = WIDTH_FOR_ONE_DAY_IN_MONTH; i=0; p=32; break;
        case TB_YEAR:   time[4] = '\0';
                if(width<9999)
                {
                    space = WIDTH_FOR_ONE_MONTH_IN_YEAR; i=0; p=13;
                }
                else
                    space = WIDTH_FOR_ONE_DAY_IN_YEAR; i=0; p=366; 
                break;
    }
    
    cairo_move_to(cr, width/2,14);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 14.0);
    cairo_show_text(cr, time);
        
    for(;i<p;i++)
    {
        cairo_set_line_width(cr, 1);
        cairo_move_to(cr, (double)i*space+X1_SKIP, Y2_SKIP);
        cairo_line_to(cr, (double)i*space+X1_SKIP, height-Y1_SKIP+TICK_OFFSET); 
        cairo_stroke(cr);
        cairo_set_line_width(cr, 1);
        switch(timebase)
        {
            case TB_DAY:    if(width<2000)
                            sprintf(time,"%02d:00:00",i*2); 
                    else
                            sprintf(time,"%02d:00:00",i);
                            break;
            case TB_WEEK:   if(i<7) sprintf(time,"%d",i+1); else strcpy(time,"\0");  break;
            case TB_MONTH:  if(i<31) sprintf(time,"%d",i+1); else strcpy(time,"\0"); break;
            case TB_YEAR:   if(width<9999)
                    {
                        if(i<12)
                            sprintf(time,"%d",i+1);
                        else
                            strcpy(time,"\0");
                    }
                    else
                    {
                        if(i<365)
                            sprintf(time,"%d",i+1);
                        else
                            strcpy(time,"\0");
                    }
                    break;

        }
        cairo_move_to(cr, i*space+X1_SKIP-X1_TO_TEXT2, height - Y1_TO_TEXT);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 9.0);
        cairo_show_text(cr, time);
    }
    
}

static void drawYLegend(cairo_t *cr, double temp_max, double temp_min, int width, int height)
{
    double range = temp_max - temp_min;
    int one_degree_height = (height-Y1_SKIP-Y2_SKIP)/10;
    int i;
    char tstring[10];
    
    for(i=0;i<10;i++)
    {
        cairo_move_to(cr, X1_SKIP-TICK_OFFSET, one_degree_height*i+Y2_SKIP);
        cairo_line_to(cr, width-X2_SKIP ,one_degree_height*i+Y2_SKIP);
        cairo_stroke(cr);

        
        sprintf(tstring,"%d",(int)(temp_max-(range/10)*i));
        cairo_move_to(cr ,X1_SKIP-X1_TO_TEXT, one_degree_height*i+Y2_SKIP);
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 9.0);
        cairo_show_text(cr, tstring);
    }
}

/* 
 * Gives back the absolute position for the temperature in the picture
 */
static double transformY(double temperature, double max, double min, int height)
{
    const double range = (double)max - (double)min;
    return (double)(1.0-((temperature-(double)min)/range))*((double)height-(Y1_SKIP+Y2_SKIP))+Y2_SKIP;
}


static double transformX(time_t x, int timebase, int width)
{
    double x_div;
    struct tm *time = localtime(&x);

    x = time->tm_hour*60*60 + time->tm_min*60 + time->tm_sec;

    switch(timebase)
    {
        case TB_DAY:    x_div = SECONDS_PER_DAY; break;
        case TB_WEEK:   x_div = SECONDS_PER_WEEK;
                if(time->tm_wday == 0) //sunday
                    x += SECONDS_PER_DAY*6;
                else
                    x += SECONDS_PER_DAY*(time->tm_wday-1);
                break;
        case TB_MONTH:  x_div = SECONDS_PER_MONTH;
                x += SECONDS_PER_DAY*(time->tm_mday-1);
                break; 
        case TB_YEAR:   x_div = SECONDS_PER_YEAR;
                x += SECONDS_PER_DAY*time->tm_yday;
                break; 
    }
    return (double)(x/x_div*(width-X1_SKIP-X2_SKIP)+X1_SKIP);
}

