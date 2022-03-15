//#include "../inih/ini.h"
#include "./inih/ini.c"
#include <sqlite3.h>

typedef struct
{
    const char* port;//use char is comfortable
    const char* p1;  //p1 is integer
    const char* p2;  //p2 is string
} configuration;
