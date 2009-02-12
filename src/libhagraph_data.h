#ifndef __DATA_H__
#define __DATA_H__

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

extern int transformDate(char *time_from, char *time_to, const char *date, int view);
extern void initGraph(struct _graph_data *graph, const char *time_from, const char *time_to);
extern void freeGraph(struct _graph_data *graph);
extern void addGraphData(struct _graph_data *graph, int modul, int sensor,
	char *mysql_host,
	char *mysql_user,
	char *mysql_password,
	char *mysql_database,
	char *mysql_database_ws2000);

#endif

