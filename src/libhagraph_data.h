/*
 * Copyright (C) 2007-2009 Bjoern Biesenbach <bjoern@bjoern-b.de>
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
#ifndef __DATA_H__
#define __DATA_H__


#define SECONDS_PER_HOUR (60*60)
#define SECONDS_PER_DAY (60*60*24)
#define SECONDS_PER_WEEK (SECONDS_PER_DAY*7)
#define SECONDS_PER_MONTH (SECONDS_PER_DAY*31)
#define SECONDS_PER_YEAR (SECONDS_PER_DAY*365)

#define TB_DAY 1
#define TB_WEEK 2
#define TB_MONTH 3
#define TB_YEAR 4

    
struct _graph_point
{
    long long x;
    double y;
};

struct _one_graph_data
{
    int modul;
    int sensor;
    double min;
    double max;
    double average;
    int num_points;
    struct _graph_point *points;
};

struct _graph_data
{
    char time_from[20];
    char time_to[20];
    time_t timestamp_from;
    time_t timestamp_to;
    int view;
    double min;
    double max;
    int num_graphs;
    struct _one_graph_data *graphs;
};

extern void transformDate(char *time_from, char *time_to, const char *date, int view);
extern void initGraph(struct _graph_data *graph, const char *time_from, const char *time_to);
extern void freeGraph(struct _graph_data *graph);
extern int addGraphData(struct _graph_data *graph, int modul, int sensor);

//extern int getLastValueTable(char *table,
//    char *mysql_host,
//    char *mysql_user,
//    char *mysql_password,
//    char *mysql_database,
//    char *mysql_database_ws2000);

#endif

