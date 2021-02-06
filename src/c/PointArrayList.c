#include "KMeansClustering.h"


void PointArrayListAdd(PointArrayList_t *PointList, Point_t Point) {
    if( PointList == NULL ) {
        DPrintf("PointArrayListAdd:Failed PointList is NULL.\n");
        return;
    }
    if( PointList->NumPoints == PointList->Size ) {
        //Grow
        PointList->Size *= 2;
        PointList->Points = realloc(PointList->Points, PointList->Size * sizeof(Point_t));
    }
    PointList->Points[PointList->NumPoints++] = Point;
}

void PointArrayListCleanUp(PointArrayList_t *PointList)
{
    int i;
    if( PointList == NULL ) {
        DPrintf("PointArrayListCleanUp:Failed PointList is NULL.\n");
        return;
    }
    for( i = 0; i < PointList->NumPoints; i++ ) {
        free(PointList->Points[i].Position);
    }
    free(PointList->Points);
    PointList->Points = NULL;
    PointList->NumPoints = 0;
    PointList->Size = 0;
}
void PointArrayListInit(PointArrayList_t *PointList,int InitialSize)
{
    if( PointList == NULL ) {
        DPrintf("PointArrayListInit:Failed PointList is NULL.\n");
        return;
    }
    if( InitialSize <= 0 ) {
        DPrintf("PointArrayListInit:Failed Invalid InitialSize\n");
        return;
    }
    PointList->Points = malloc(sizeof(Point_t) * InitialSize);
    PointList->NumPoints = 0;
    PointList->Size = InitialSize;
}
