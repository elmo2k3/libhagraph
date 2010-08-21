#ifndef __GRAPH_NAMES_H__
#define __GRAPH_NAMES_H__

#define MAX_MODULES 20
#define MAX_SENSORS 20

char *getGraphName(int module, int sensor);

/* returns
 *  0 on success
 *  1 file error
 *  2 allocate error
 *  3 module or sensor too big
 */
int readGraphNameFile(char *filename);

#endif

