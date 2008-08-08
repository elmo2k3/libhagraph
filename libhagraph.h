#ifndef __HAGRAPH_H__
#define __HAGRAPH_H__

#include <gd.h>
#include <mysql/mysql.h>

#define IMG_WIDTH_STD 800
#define IMG_HEIGHT_STD 600

#define X1_SKIP 40
#define X2_SKIP 40
#define Y1_SKIP 40
#define Y2_SKIP 40
#define X1_TO_TEXT 25
#define X1_TO_TEXT2 5
#define Y1_TO_TEXT 20
#define TICK_OFFSET 10

#define DRAW_VERTICAL_GRID
#define DRAW_HORIZONTAL_GRID

#define SECONDS_PER_DAY (60*60*24)
#define SECONDS_PER_WEEK (SECONDS_PER_DAY*7)
#define SECONDS_PER_MONTH (SECONDS_PER_DAY*31)
#define SECONDS_PER_YEAR (SECONDS_PER_DAY*366)
	
#define WIDTH_FOR_ONE_HOUR ((IMG_WIDTH-X1_SKIP-X2_SKIP)/24)
#define WIDTH_FOR_ONE_DAY_IN_WEEK ((IMG_WIDTH-X1_SKIP-X2_SKIP)/7)
#define WIDTH_FOR_ONE_DAY_IN_MONTH ((IMG_WIDTH-X1_SKIP-X2_SKIP)/31)
#define WIDTH_FOR_ONE_DAY_IN_YEAR ((IMG_WIDTH-X1_SKIP-X2_SKIP)/366)

#define TB_DAY 1
#define TB_WEEK 2
#define TB_MONTH 3
#define TB_YEAR 4

extern int createGraph(const char *filename, int width, int heigth, const char *time_from,
		const char *time_to, int *modul, int *sensor, int numGraphs);


#endif

