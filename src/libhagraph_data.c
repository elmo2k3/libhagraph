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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#ifdef _WIN32
#include <mysql/my_global.h>
#endif
#include <mysql/mysql.h>

#include "libhagraph.h"
#include "libhagraph_data.h"
#include "graph_names.h"

#ifdef _WIN32
# define __isleap(year) \
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
#endif

static int decideView(struct _graph_data *graph, const char *time_from, const char *time_to);
static int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

void initGraph(struct _graph_data *graph, const char *time_from, const char *time_to)
{
    memset(graph, 0, sizeof(struct _graph_data));
    strncpy(graph->time_from, time_from, 10);
    strncpy(graph->time_to, time_to, 10);
    
    graph->view = decideView(graph, graph->time_from, graph->time_to);
    graph->min = 999.99;
    graph->max = -999.99;

    graph->num_graphs = 0;
    graph->graphs = NULL;
}

void freeGraph(struct _graph_data *graph)
{
    int i;
    for(i=0; i< graph->num_graphs;i++)
    {
        free(graph->graphs[i].points);
    }
    free(graph->graphs);
}

int addGraphData(struct _graph_data *graph, int modul, int sensor,
    char *mysql_host,
    char *mysql_user,
    char *mysql_password,
    char *mysql_database,
    char *mysql_database_ws2000)
{
    MYSQL_RES *mysql_res;
    MYSQL_ROW mysql_row;
    MYSQL *mysql_helper_connection;

    int i=0;
    int num_points;
    double seconds, temperature;
    char query[1024];
    double max = -999.9;
    double min = 999.9;
    long double average = 0.0;
    
    /* allocate new space for _one_graph_data */
    graph->graphs = realloc(graph->graphs, sizeof(struct _one_graph_data)*(graph->num_graphs+1));
    graph->graphs[graph->num_graphs].modul = modul;
    graph->graphs[graph->num_graphs].sensor = sensor;
    
    MYSQL *mysql_connection;

    mysql_connection = mysql_init(NULL);
    mysql_options(mysql_connection, MYSQL_OPT_COMPRESS, 0);
    if (!mysql_real_connect(mysql_connection, mysql_host, mysql_user, mysql_password, mysql_database, 0, NULL, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(mysql_connection));
        return -1;
    }
    mysql_connection->reconnect=1;
    
    mysql_helper_connection = mysql_connection;
    if(modul==4)
    {
        mysql_connection = mysql_init(NULL);
        mysql_options(mysql_connection, MYSQL_OPT_COMPRESS, 0);
        if (!mysql_real_connect(mysql_connection, mysql_host, mysql_user, mysql_password, mysql_database_ws2000, 0, NULL, 0))
        {
            fprintf(stderr, "%s\n", mysql_error(mysql_connection));
            return -1;
        }
        if(sensor == 0)
            sprintf(query,"SELECT UNIX_TIMESTAMP(CONCAT(date,\" \",time)),\
            T_1*1000\
            FROM sensor_1_8\
            WHERE date>='%s'\
            AND date<'%s'\
            AND ok_1='0'\
            ORDER BY date,time asc",graph->time_from, graph->time_to);
        if(sensor == 1)
            sprintf(query,"SELECT UNIX_TIMESTAMP(CONCAT(date,\" \",time)),\
            T_i*1000\
            FROM inside\
            WHERE date>='%s'\
            AND date<'%s'\
            AND ok='0'\
            ORDER BY date,time asc",graph->time_from, graph->time_to);
    }
    else
    {
        sprintf(query,"SELECT date,\
            value*1000 \
            FROM modul_%d \
            WHERE sensor='%d' \
            AND value!=85.0 \
            AND date>'%ld'\
            AND date<'%ld'\
            ORDER BY date asc", modul, sensor, graph->timestamp_from, graph->timestamp_to);
    }

    if(mysql_query(mysql_connection,query))
    {
        fprintf(stderr, "%s\n", mysql_error(mysql_connection));
        return -1;
    }
    mysql_res = mysql_store_result(mysql_connection);

    /* lets decide how much memory to allocate */

    num_points = mysql_num_rows(mysql_res);

    if(!num_points)
    {
        if(modul==4)
            mysql_close(mysql_connection);
        mysql_connection = mysql_helper_connection;
        mysql_close(mysql_connection);
#ifdef _DEBUG
        printf("no points ..\n");
        printf("empty query was: \n%s\n",query);
#endif
        return -2;
    }

    graph->graphs[graph->num_graphs].num_points = num_points;
    graph->graphs[graph->num_graphs].points = malloc(sizeof(struct _graph_point)*num_points);

    struct _graph_point *helper = graph->graphs[graph->num_graphs].points;
    
    while((mysql_row = mysql_fetch_row(mysql_res)))
    {
        
        if(!mysql_row)
        {   
            fprintf(stderr, "%s\n", mysql_error(mysql_connection));
            return -1;
        }
        
        if(mysql_row[0]) seconds    = (long long)atoi(mysql_row[0]);
        
        if(strcmp(mysql_row[1],"0.0")) temperature = (double)atoi(mysql_row[1])/1000;

        helper[i].x = seconds;
        helper[i].y = temperature;
        average += temperature;
        if(temperature > max)
            max = temperature;
        if(temperature < min)
            min = temperature;
        i++;
    }
    if(modul==4)
        mysql_close(mysql_connection);
    mysql_connection = mysql_helper_connection;

    mysql_free_result(mysql_res);
    mysql_close(mysql_connection);

    if(min < graph->min)
        graph->min = floor(min/10.0)*10;
    if(max > graph->max)
        graph->max = ceil(max/10.0)*10; 
    
    graph->graphs[graph->num_graphs].min = min;
    graph->graphs[graph->num_graphs].max = max;
    graph->graphs[graph->num_graphs].average = average/num_points;
    graph->num_graphs++;

    return 0;
}

static int decideView(struct _graph_data *graph, const char *time_from, const char *time_to)
{
    char c_from[255], c_to[255];
    struct tm from, to;
    memset(&from,0,sizeof(struct tm));  
    memset(&to,0,sizeof(struct tm));    
    strcpy(c_from,time_from);
    strcpy(c_to,time_to);
    
    from.tm_year = atoi(strtok(c_from,"-")) -1900;
    from.tm_mon = atoi(strtok(NULL,"-")) - 1;
    from.tm_mday = atoi(strtok(NULL,"-"));
    from.tm_hour = 0;
    from.tm_min = 0;
    from.tm_sec = 0;
    from.tm_isdst = -1;
    
    to.tm_year = atoi(strtok(c_to,"-")) -1900;
    to.tm_mon = atoi(strtok(NULL,"-")) - 1;
    to.tm_mday = atoi(strtok(NULL,"-"));
    to.tm_hour = 0;
    to.tm_min = 0;
    to.tm_sec = 0;
    to.tm_isdst = -1;

    graph->timestamp_from = mktime(&from);
    graph->timestamp_to = mktime(&to);

    if(mktime(&to)-mktime(&from) <= SECONDS_PER_DAY + SECONDS_PER_HOUR)
        return TB_DAY;
    else if(mktime(&to)-mktime(&from) <= SECONDS_PER_WEEK + SECONDS_PER_HOUR)
        return TB_WEEK;
    else if(mktime(&to)-mktime(&from) <= SECONDS_PER_MONTH + SECONDS_PER_HOUR)
        return TB_MONTH;
    else
        return TB_YEAR;
}
    
void transformDate(char *time_from, char *time_to, const char *date, int view)
{
    char c_date[255];
    struct tm from, *to;
    time_t s_from, s_to;
    time_t seconds_to_add;

    memset(&from,0,sizeof(struct tm));

    if(strlen(date)!=4 && strlen(date)!=7 && strlen(date) !=10)
        return 0;
    strcpy(c_date,date);
    
    if(strlen(c_date) == 4)
        view = TB_YEAR;
    else if(strlen(c_date) == 7)
        view = TB_MONTH;

    switch(view)
    {
        case TB_WEEK:   from.tm_year = atoi(strtok(c_date,"-")) - 1900;
                from.tm_mon = atoi(strtok(NULL,"-")) - 1;
                from.tm_mday = atoi(strtok(NULL,"-"));
                seconds_to_add = SECONDS_PER_WEEK; 
                break;
        case TB_DAY:    from.tm_year = atoi(strtok(c_date,"-")) - 1900;
                from.tm_mon = atoi(strtok(NULL,"-")) - 1;
                from.tm_mday = atoi(strtok(NULL,"-"));
                seconds_to_add = SECONDS_PER_DAY;
                break;
        case TB_MONTH:  from.tm_year = atoi(strtok(c_date,"-")) - 1900;
                from.tm_mon = atoi(strtok(NULL,"-")) - 1;
                from.tm_mday = 1;
                if(from.tm_mon == 1 && __isleap(from.tm_year+1900))
                {
                    seconds_to_add = SECONDS_PER_DAY * 29;
                }
                else
                    seconds_to_add = SECONDS_PER_DAY * days_in_month[from.tm_mon];
                break;
        case TB_YEAR:   from.tm_year = atoi(c_date) - 1900;
                from.tm_mon = 0;
                from.tm_mday = 1;
                seconds_to_add = SECONDS_PER_YEAR;
                if(__isleap(from.tm_year+1900))
                {
                    seconds_to_add += SECONDS_PER_DAY;
                }
                break;
    }
    from.tm_hour = 0;
    from.tm_min = 0;
    from.tm_sec = 0;
    s_from = mktime(&from);
    if(view == TB_WEEK)
    {
        if(from.tm_wday == 0) //sunday
            s_from -= SECONDS_PER_DAY * 6;
        else
            s_from -= SECONDS_PER_DAY * (from.tm_wday-1);
        to = localtime(&s_from);
        memcpy(&from, to, sizeof(struct tm));
    }
    s_to = s_from + seconds_to_add;
    to = localtime(&s_to);

    strftime(time_from, 20, "%Y-%m-%d", &from);
    strftime(time_to, 20, "%Y-%m-%d", to);
    time_from[10] = '\0';
    time_to[10] = '\0';
}

int getLastValueTable(char *table,
    char *mysql_host,
    char *mysql_user,
    char *mysql_password,
    char *mysql_database,
    char *mysql_database_ws2000)
{
    MYSQL_RES *mysql_res;
    MYSQL_ROW mysql_row;
    MYSQL *mysql_helper_connection;
    MYSQL *mysql_connection;
    double temperature;
    char query[2048];
    
    if(readGraphNameFile("/etc/hagraphs.conf"))
    {
        if(readGraphNameFile("hagraphs.conf"))
        {
            fprintf(stderr,"could not find hagraphs.conf");
            return;
        }
    }
    mysql_connection = mysql_init(NULL);
    mysql_options(mysql_connection, MYSQL_OPT_COMPRESS, 0);
    if (!mysql_real_connect(mysql_connection, mysql_host, mysql_user, mysql_password, mysql_database, 0, NULL, 0))
    {
        fprintf(stderr, "%s\n", mysql_error(mysql_connection));
        return -1;
    }
    mysql_connection->reconnect=1;
    
    mysql_helper_connection = mysql_connection;

    char row[100];
    int i, p;
    for(i=0;i<MAX_MODULES;i++)
    {
        for(p=0;p<MAX_SENSORS;p++)
        {
            if(getGraphName(i,p) && strlen(getGraphName(i,p)))
            {
                if(i==4)
                {
                    mysql_connection = mysql_init(NULL);
                    mysql_options(mysql_connection, MYSQL_OPT_COMPRESS, 0);
                    if (!mysql_real_connect(mysql_connection, mysql_host, mysql_user, mysql_password, mysql_database_ws2000, 0, NULL, 0))
                    {
                        fprintf(stderr, "%s\n", mysql_error(mysql_connection));
                        return -1;
                    }
                    if(p == 0)
                        sprintf(query,"SELECT T_1*1000\
                        FROM sensor_1_8\
                        ORDER BY date desc, time desc LIMIT 1");
                    if(p == 1)
                        sprintf(query,"SELECT T_i*1000\
                        FROM inside\
                        ORDER BY date desc, time desc LIMIT 1");
                }
                else
                {
                    sprintf(query,"SELECT value*1000\
                        FROM modul_%d \
                        WHERE sensor='%d' \
                        AND value!=85.0 \
                        ORDER BY date desc LIMIT 1", i,p);
                }
                if(mysql_query(mysql_connection,query))
                {
                    fprintf(stderr, "%s\n", mysql_error(mysql_connection));
                    return -1;
                }
                mysql_res = mysql_store_result(mysql_connection);
                mysql_row = mysql_fetch_row(mysql_res);
                if(!mysql_row)
                {   
                    fprintf(stderr, "%s\n", mysql_error(mysql_connection));
                    return -1;
                }
                temperature = (double)atoi(mysql_row[0])/1000;

                sprintf(row,"%s: %3.2f\n",getGraphName(i,p),temperature);
                strcat(table,row);
                if(i==4)
                {
                    mysql_close(mysql_connection);
                    mysql_connection = mysql_helper_connection;
                }
                mysql_free_result(mysql_res);
            }
        }
    }

    mysql_close(mysql_connection);
    return strlen(table);
}
