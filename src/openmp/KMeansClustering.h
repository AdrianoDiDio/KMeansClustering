#ifndef __KMEANSCLUSTERING_H_
#define __KMEANSCLUSTERING_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <omp.h>

#include "PointArrayList.h"

#ifdef __GNUC__
#define Attribute(x) __attribute__(x)
#else
#define Attribute(x)
#endif

#define KMEANS_ALGORITHM_TOLERANCE 0.00001f

typedef struct Flower_s {
    float SepalWidth;
    float SepalLength;
    float PetalWidth;
    float PetalLength;
    char  *Species;
    
    struct Flower_s *Next;
} Flower_t;

typedef struct Vec2_s {
    float x;
    float y; 
} Vec2_t;
typedef struct Point_s {
    float *Position;
    int CentroidIndex;
    int Stride;
} Point_t;

typedef struct Centroid_s {
    float *Position;
    int    Stride;
} Centroid_t;


void    DPrintf(char *Fmt, ...) Attribute((format(printf,1,2)));

#endif //__KMEANSCLUSTERING_H_
