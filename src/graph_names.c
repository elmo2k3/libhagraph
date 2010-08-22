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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "graph_names.h"

static char **sensor_names = NULL;
static int max_module = 0, max_sensor = 0;
static int initiated = 0;

static void setGraphName(int module, int sensor, char *name)
{
    sensor_names[module*(max_sensor+1)+sensor] = strdup(name);
}

static int allocateGraphNames()
{
    sensor_names = (char**)calloc(sizeof(char*),(max_module+1)*(max_sensor+1));
    return sensor_names ? 1:0;
}

static void freeGraphNames()
{
    int i,p;
    for(i=0;i<=max_module;i++)
    {
        for(p=0;p<=max_sensor;p++)
        {
            if(getGraphName(i,p))
                free(getGraphName(i,p));
        }
    }
    free(sensor_names);
    sensor_names = NULL;
}
/* returns
 *  0 on success
 *  1 file error
 *  2 allocate error
 *  3 module or sensor too big
 */
static int readGraphNameFile(char *filename)
{
    FILE *file;
    int module, sensor;
    char text[255];
    char *part;
    char *cr;
    
    if(sensor_names)
        freeGraphNames();

    file = fopen(filename, "r");
    if(!file)
        return 1;

    max_module = 0;
    max_sensor = 0;

    while(fgets(text, sizeof(text), file) != NULL)
    {
        part = strtok(text, " ");
        if(!part)
            continue;
        if(part == strchr(part,'\n'))
            continue;
        module = atoi(part);
        if(module > max_module)
            max_module = module;
        part = strtok(NULL, " ");
        if(!part)
            continue;
        sensor = atoi(part);
        if(sensor > max_sensor)
            max_sensor = sensor;
    }
    if(!allocateGraphNames())
    {
        fclose(file);
        return 2;
    }
    if(max_sensor > MAX_SENSORS || max_module > MAX_MODULES)
    {
        fclose(file);
        return 3;
    }
    rewind(file);
    while(fgets(text, sizeof(text), file) != NULL)
    {
        part = strtok(text, " ");
        if(!part)
            continue;
        if(part == strchr(part,'\n'))
            continue;
        module = atoi(part);
        part = strtok(NULL, " ");
        if(!part)
            continue;
        if(part == strchr(part,'\n'))
            continue;
        sensor = atoi(part);
        part = part+2;
        cr = strchr(part,'\n');
        *cr = '\0';
        setGraphName(module,sensor,part);
    }
    fclose(file);
    return 0;
}

static int graphNamesInit(void)
{
    if(initiated)
        return 2;
    if(readGraphNameFile("/etc/hagraphs.conf"))
    {
        if(readGraphNameFile("hagraphs.conf"))
        {
            fprintf(stderr,"could not find hagraphs.conf");
            return 1;
        }
    }
    initiated = 1;
    return 0;
}   

char *getGraphName(int module, int sensor)
{
    if(!initiated)
        if(graphNamesInit())
            return NULL;

    if(module > max_module || sensor > max_sensor || sensor_names==NULL)
        return NULL;
    return sensor_names[module*(max_sensor+1)+sensor];
}

