/* Bjoern Biesenbach <bjoern@bjoern-b.de>
 */


#include <stdio.h>
#include <stdlib.h>
#include <gd.h>
#include <gdfontl.h>
#include <gdfonts.h>
#include <mysql/mysql.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "libhagraph.h"

#define MYSQL_SERVER    "88.198.17.204"
#define MYSQL_USER      "home_automation"
#define MYSQL_PASS      "rfm12"
#define MYSQL_DB        "home_automation"
#define MYSQL_DB_WS2000	"wetterstation"

static int IMG_WIDTH, IMG_HEIGHT;

int createGraph(const char *filename, int width, int heigth, const char *time_from,
		const char *time_to, int *modul, int *sensor, int numGraphs)
{
	IMG_HEIGHT = heigth;
	IMG_WIDTH = width;

	gdImagePtr im;
	FILE *pngout;
	int white, black, red, green, blue, purple, orange;
	int sec_max;
	int c;
	float temp_max = 0.0,
	      temp_min = 0.0;

	int view;
	
	view = decideView(time_from, time_to);
	
	MYSQL *mysql_connection;

	mysql_connection = mysql_init(NULL);
	if (!mysql_real_connect(mysql_connection, MYSQL_SERVER, MYSQL_USER, MYSQL_PASS, MYSQL_DB, 0, NULL, 0))
	{
		fprintf(stderr, "%s\n", mysql_error(mysql_connection));
		exit(0);
	}
	mysql_connection->reconnect=1;
	
	im = gdImageCreate(width, heigth);
	white = gdImageColorAllocate(im,255,255,255); // Hintergrundfarbe
	black = gdImageColorAllocate(im,0,0,0);
	red = gdImageColorAllocate(im,255,0,0);
	green = gdImageColorAllocate(im,0,255,0);
	blue = gdImageColorAllocate(im,0,0,255);
	purple = gdImageColorAllocate(im,255,0,255);
	orange = gdImageColorAllocate(im,255,255,0);
	
	int colors[]={red,blue,green,purple,orange};

	for(c=0; c< numGraphs; c++)
	{
		getMaxMinValues(mysql_connection, time_from, time_to, &temp_max, &sec_max, &temp_min, modul[c],sensor[c]);
	}
	
	temp_max = ceil(temp_max/10)*10;
	temp_min = floor(temp_min/10)*10;
	
	for(c=0;c < numGraphs;c++)
	{
		addGraph(im, mysql_connection, colors[c], time_from, time_to, view, modul[c], sensor[c], temp_max, temp_min);
	}
	
	drawXLegend(im, view, black, (unsigned char*)time_from);
	drawYLegend(im, temp_max, temp_min, black);
	pngout = fopen(filename, "wb");
	gdImagePng(im, pngout);
	fclose(pngout);
	gdImageDestroy(im);
	mysql_close(mysql_connection);
	return 0;
}

static void drawXLegend(gdImagePtr im, char timebase, int color, unsigned char *title);
static void getMaxMinValues(MYSQL *mysql_connection, const char *time_from, const char *time_to, float *max, int *sec_max, float *min, int modul, int sensor);
static int transformY(float temperature, float max, float min);
static void addGraph(gdImagePtr im, MYSQL *mysql_connection, int color, const char *time_from, const char *time_to, char timebase, int modul, int sensor, float temp_max, float temp_min);
static void drawYLegend(gdImagePtr im, float temp_max, float temp_min, int color);
static int decideView(char *time_from, char *time_to);

/* 
 * X-Achse zeichnen
 * Möglichkeiten für timebase: TB_DAY, TB_WEEK, TB_MONTH, TB_YEAR
 * 
 */
static void drawXLegend(gdImagePtr im, char timebase, int color, unsigned char *title)
{
	int width;
	int i,p;
	char time[200];
	
	gdImageSetThickness(im, 2);
	gdImageLine(im, X1_SKIP-5, IMG_HEIGHT-Y1_SKIP, IMG_WIDTH-X2_SKIP+5, IMG_HEIGHT-Y1_SKIP, color);

	gdImageSetThickness(im, 1);
	
	switch(timebase)
	{
		case TB_DAY: 	if(IMG_WIDTH<2000)
				{
					width = WIDTH_FOR_ONE_HOUR*2;i=0,p=13;
				}
				else
				{
					width = WIDTH_FOR_ONE_HOUR; i=0; p=25;
				}
				gdImageString(im,gdFontGetLarge(), IMG_WIDTH/2, 5, title,color); 
				break;
		case TB_WEEK: 	width = WIDTH_FOR_ONE_DAY_IN_WEEK; i=0; p=8; break;
		case TB_MONTH: 	width = WIDTH_FOR_ONE_DAY_IN_MONTH; i=0; p=32; break;
		case TB_YEAR: 	width = WIDTH_FOR_ONE_DAY_IN_YEAR; i=0; p=367; break;
	}
		
	for(;i<p;i++)
	{
#ifdef DRAW_VERTICAL_GRID
		gdImageDashedLine(im, i*width+X1_SKIP, Y2_SKIP, i*width+X1_SKIP, IMG_HEIGHT-Y1_SKIP+TICK_OFFSET, color);
#endif
		gdImageLine(im, i*width+X1_SKIP,IMG_HEIGHT-Y1_SKIP, i*width+X1_SKIP, IMG_HEIGHT -Y1_SKIP, color);
		switch(timebase)
		{
			case TB_DAY: 	if(IMG_WIDTH<2000)
							sprintf(time,"%02d:00:00",i*2); 
					else
							sprintf(time,"%02d:00:00",i);
							break;
			case TB_WEEK: 	if(i<7) sprintf(time,"%d",i+1); else strcpy(time,"\0");  break;
			case TB_MONTH: 	if(i<31) sprintf(time,"%d",i+1); else strcpy(time,"\0"); break;
			case TB_YEAR: 	sprintf(time,"%d",i+1); break;
		}
		gdImageString(im,gdFontGetSmall(), i*width+X1_SKIP-X1_TO_TEXT2,IMG_HEIGHT-Y1_TO_TEXT, time,color);
	}
	
}

static void drawYLegend(gdImagePtr im, float temp_max, float temp_min, int color)
{
	float range = temp_max - temp_min;
	int one_degree_height = (IMG_HEIGHT-Y1_SKIP-Y2_SKIP)/10;
	int i;
	char tstring[10];
	
	for(i=0;i<10;i++)
	{
#ifdef DRAW_HORIZONTAL_GRID
		gdImageDashedLine(im,X1_SKIP-TICK_OFFSET,one_degree_height*i+Y2_SKIP ,IMG_WIDTH-X2_SKIP,one_degree_height*i+Y2_SKIP ,color);
#endif
		gdImageDashedLine(im,X1_SKIP-TICK_OFFSET,one_degree_height*i+Y2_SKIP ,X1_SKIP+TICK_OFFSET,one_degree_height*i+Y2_SKIP ,color);

		
		sprintf(tstring,"%d",(int)(temp_max-(range/10)*i));
		gdImageString(im,gdFontGetSmall(), X1_SKIP-X1_TO_TEXT,one_degree_height*i+Y2_SKIP, tstring,color);
	}
}

static void addGraph(gdImagePtr im, MYSQL *mysql_connection, int color, const char *time_from, const char *time_to, char timebase, int modul, int sensor, float temp_max, float temp_min)
{
	char query[255];
	MYSQL_RES *mysql_res;
	MYSQL_ROW mysql_row;
	MYSQL *mysql_helper_connection;
	
	float seconds[2];		// vorgänger und aktueller wert
	int day_of_week, day_of_month, day_of_year;
	float temperature[2];
	float x_div;
	int x1,y1,x2,y2;
	
	mysql_helper_connection = mysql_connection;
	if(modul==4)
	{
		mysql_connection = mysql_init(NULL);
		if (!mysql_real_connect(mysql_connection, MYSQL_SERVER, MYSQL_USER, MYSQL_PASS, MYSQL_DB_WS2000, 0, NULL, 0))
		{
			fprintf(stderr, "%s\n", mysql_error(mysql_connection));
			exit(0);
		}
		sprintf(query,"SELECT TIME_TO_SEC(time), DAYOFWEEK(date), DAYOFMONTH(date), DAYOFYEAR(date), T_1 FROM sensor_1_8 WHERE date>='%s' AND date<'%s' AND ok_1='0' ORDER BY date,time asc",time_from, time_to);
	}
	else
		sprintf(query,"SELECT TIME_TO_SEC(date), DAYOFWEEK(date), DAYOFMONTH(date), DAYOFYEAR(date), temperature FROM temperatures WHERE modul_id='%d' AND sensor_id='%d' AND date>'%s' AND date<'%s' ORDER BY date asc", modul, sensor, time_from, time_to);
	if(mysql_query(mysql_connection,query))
	{
		fprintf(stderr, "%s\n", mysql_error(mysql_connection));
		exit(0);
	}
	mysql_res = mysql_use_result(mysql_connection);
	int i=0;
	while((mysql_row = mysql_fetch_row(mysql_res)))
	{
		
		if(!mysql_row)
		{	
			fprintf(stderr, "%s\n", mysql_error(mysql_connection));
			exit(0);
		}
		
		if(mysql_row[0]) seconds[1]	= atoi(mysql_row[0]);
		else seconds[1]	= 0;
		
		if(strcmp(mysql_row[1],"0.0")) temperature[1] = atof(mysql_row[4]);
		else temperature[1] = 0;
		
		day_of_week = atoi(mysql_row[1]) -2;	// MYSQL gibt Sonntag = 1... zurück
		if(day_of_week == -1) day_of_week = 6;		// jetzt Montag=1, Sonntag=7
		day_of_month = atoi(mysql_row[2]) -1;
		day_of_year = atoi(mysql_row[3]) -1;
	
		gdImageSetThickness(im, 2);
		switch(timebase)
		{
			case TB_DAY: 	x_div = SECONDS_PER_DAY; break;
			case TB_WEEK: 	x_div = SECONDS_PER_WEEK;
							seconds[1] += SECONDS_PER_DAY*day_of_week;
							break;
			case TB_MONTH: 	x_div = SECONDS_PER_MONTH;
							seconds[1] += SECONDS_PER_DAY*day_of_month;
							break; 
			case TB_YEAR: 	x_div = SECONDS_PER_YEAR;
							seconds[1] += SECONDS_PER_DAY*day_of_year;
							break; 
		}
		if(i) // erst ab dem zweiten Durchlauf
		{
			x1 = seconds[0]/x_div*(IMG_WIDTH-X1_SKIP-X2_SKIP)+X1_SKIP;
			x2 = seconds[1]/x_div*(IMG_WIDTH-X1_SKIP-X2_SKIP)+X1_SKIP;
			y1 = transformY(temperature[0],temp_max,temp_min);
			y2 = transformY(temperature[1],temp_max,temp_min);

			gdImageLine(im,x1,y1,x2,y2,color);
		}
		i++;
		temperature[0]=temperature[1];
		seconds[0]=seconds[1];
	}
	if(modul==4)
		mysql_close(mysql_connection);
	mysql_connection = mysql_helper_connection;
}

/* 
 * Gives back the absolute position for the temperature in the picture
 */
static int transformY(float temperature, float max, float min)
{
	const float range = max - min;
	return (1-((temperature-min)/range))*(IMG_HEIGHT-80)+40;
}

static void getMaxMinValues(MYSQL *mysql_connection, const char *time_from, const char *time_to, float *max, int *sec_max, float *min, int modul, int sensor)
{
	MYSQL *mysql_helper_connection;
	MYSQL_RES *mysql_res;
	MYSQL_ROW mysql_row;
	float t_max, s_max, t_min, s_min;
	char query[255];
	
	mysql_helper_connection = mysql_connection;
	if(modul==4)
	{
		mysql_connection = mysql_init(NULL);
		if (!mysql_real_connect(mysql_connection, MYSQL_SERVER, MYSQL_USER, MYSQL_PASS, MYSQL_DB_WS2000, 0, NULL, 0))
		{
			fprintf(stderr, "%s\n", mysql_error(mysql_connection));
			exit(0);
		}
		sprintf(query,"SELECT TIME_TO_SEC(time), T_1 FROM sensor_1_8 WHERE date>='%s' AND date<'%s' AND ok_1='0' ORDER BY T_1 desc LIMIT 1", time_from, time_to);
	}
	else
		sprintf(query,"SELECT TIME_TO_SEC(date), temperature FROM temperatures WHERE modul_id='%d' AND sensor_id='%d' AND date>'%s' AND date<'%s' AND temperature!='85.0' ORDER BY temperature desc LIMIT 1", modul, sensor, time_from, time_to);
		
	if(mysql_query(mysql_connection,query))
	{
		fprintf(stderr, "%s\n", mysql_error(mysql_connection));
		exit(0);
	}
	mysql_res = mysql_use_result(mysql_connection);
	if(!(mysql_row = mysql_fetch_row(mysql_res)))
		return;
	mysql_free_result(mysql_res);
	if(mysql_row[0]) s_max = atoi(mysql_row[0]);
	else s_max = 0;
	if(mysql_row[1]) t_max = atof(mysql_row[1]);
	else t_max = 0;
	if(modul==4)
		sprintf(query,"SELECT TIME_TO_SEC(time), T_1 FROM sensor_1_8 WHERE date>='%s' AND date<'%s' AND ok_1='0' ORDER BY T_1 asc LIMIT 1", time_from,  time_to);
	else
		sprintf(query,"SELECT TIME_TO_SEC(date), temperature FROM temperatures WHERE modul_id='%d' AND sensor_id='%d' AND date>'%s' AND date<'%s' ORDER BY temperature asc LIMIT 1", modul, sensor, time_from, time_to);
	if(mysql_query(mysql_connection,query))
	{
		fprintf(stderr, "%s\n", mysql_error(mysql_connection));
		exit(0);
	}
	mysql_res = mysql_use_result(mysql_connection);
	mysql_row = mysql_fetch_row(mysql_res);
	mysql_free_result(mysql_res);
	if(mysql_row[0]) s_max = atoi(mysql_row[0]);
	else s_min = 0;
	if(mysql_row[1]) t_min = atof(mysql_row[1]);
	else t_min = 0;
	//printf("%f %f \n",t_max,t_min);
	if(*max == 0.0 && *min == 0.0)
	{
		*max = t_max;
		*min = t_min;
	}
	else
	{
		if(t_max > *max) *max = t_max;
		if(t_min < *min) *min = t_min;
	}
	if(modul==4)
		mysql_close(mysql_connection);
	mysql_connection = mysql_helper_connection; // wieder herstellen
}
static int decideView(char *time_from, char *time_to)
{
	char c_from[255], c_to[255];
	struct tm from, to;
	
	strcpy(c_from,time_from);
	strcpy(c_to,time_to);
	
	from.tm_year = atoi(strtok(c_from,"-")) -1900;
	from.tm_mon = atoi(strtok(NULL,"-")) - 1;
	from.tm_mday = atoi(strtok(NULL,"-"));
	from.tm_hour = 0;
	from.tm_min = 0;
	from.tm_sec = 0;
	
	to.tm_year = atoi(strtok(c_to,"-")) -1900;
	to.tm_mon = atoi(strtok(NULL,"-")) - 1;
	to.tm_mday = atoi(strtok(NULL,"-"));
	to.tm_hour = 0;
	to.tm_min = 0;
	to.tm_sec = 0;
	
	if(mktime(&to)-mktime(&from) <= SECONDS_PER_DAY)
		return TB_DAY;
	else if(mktime(&to)-mktime(&from) <= SECONDS_PER_WEEK)
		return TB_WEEK;
	else if(mktime(&to)-mktime(&from) <= SECONDS_PER_MONTH)
		return TB_MONTH;
	else
		return TB_YEAR;
}
