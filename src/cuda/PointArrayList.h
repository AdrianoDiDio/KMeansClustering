#ifndef __POINTARRAYLIST_H_
#define __POINTARRAYLIST_H_


typedef struct PointArrayList_s {
    float   *Points;
    int      Stride;
    int      NumPoints; // Number of points
    int      Size;
} PointArrayList_t;

PointArrayList_t *LoadPointsDataset(int *Stride);

void PointArrayListInit(PointArrayList_t *PointList,int InitialSize,int Stride);
void PointArrayListAdd(PointArrayList_t *PointList, float *Point);
void PointArrayListCleanUp(PointArrayList_t *PointList);


#endif //__POINTARRAYLIST_H_
