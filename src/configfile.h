#ifndef __CONFIGFILE_H__
#define __CONFIGFILE_H__


char *getGraphName(int module, int sensor); // static allocated internally, no free

char *getDbValue(char *what, int db_num, int module, int sensor); // static
int getDbNum(int module, int sensor);


#endif

