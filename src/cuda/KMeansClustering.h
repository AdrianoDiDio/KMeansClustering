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
#include <sys/time.h>
#include <sys/types.h>

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cuda_profiler_api.h>

#include "Utils.h"
#include "PointArrayList.h"

#ifdef __GNUC__
#define Attribute(x) __attribute__(x)
#else
#define Attribute(x)
#endif

#define KMEANS_ALGORITHM_TOLERANCE 0.4f

typedef struct Flower_s {
    float SepalWidth;
    float SepalLength;
    float PetalWidth;
    float PetalLength;
    char  *Species;
    
    struct Flower_s *Next;
} Flower_t;

typedef float Vec_t;
typedef Vec_t Vec2_t[2];

// typedef struct Vec2_s {
//     float x;
//     float y; 
// } Vec2_t;
typedef struct Point_s {
    Vec2_t Position;
    int CentroidIndex;
} Point_t;

typedef struct Centroid_s {
    Vec2_t Position;
} Centroid_t;


void    DPrintf(char *Fmt, ...) Attribute((format(printf,1,2)));

#endif //__KMEANSCLUSTERING_H_
