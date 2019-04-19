#pragma once

#include <stdio.h>

#ifndef LOG_ERR
#define LOG_ERR(format, ...)\
do{\
    fprintf(stderr, "[ERROR] [%s:%d] " format "\n", __FUNCTION__ , __LINE__, ##__VA_ARGS__);\
}\
while(false)
#endif

#ifndef LOG
#define LOG(format, ...) LOG_ERR(format, ##__VA_ARGS__)
#endif