#ifndef __POINTARRAYLIST_H_
#define __POINTARRAYLIST_H_

struct Point_s;
typedef struct Point_s Point_t;

typedef struct PointArrayList_s {
    Point_t *Points;
    int      NumPoints;
    int      Size;
} PointArrayList_t;

void PointArrayListInit(PointArrayList_t *PointList,int InitialSize);
void PointArrayListAdd(PointArrayList_t *PointList, Point_t Point);
void PointArrayListCleanUp(PointArrayList_t *PointList);

#endif //__POINTARRAYLIST_H_
