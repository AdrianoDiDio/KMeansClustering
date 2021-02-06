#ifndef __POINTARRAYLIST_H_
#define __POINTARRAYLIST_H_

struct Point_s;
typedef struct Point_s Point_t;

typedef struct PointArrayList_s {
    float   *Points;
    int      NumPoints;
    int      Size;
    int      Stride;
} PointArrayList_t;

void PointArrayListInit(PointArrayList_t *PointList,int InitialSize,int Stride);
void PointArrayListAdd(PointArrayList_t *PointList, float *Point);
void PointArrayListCleanUp(PointArrayList_t *PointList);

#endif //__POINTARRAYLIST_H_
