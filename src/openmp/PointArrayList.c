#include "KMeansClustering.h"


void PointArrayListAdd(PointArrayList_t *PointList, float *Point) {
    int Base;
    int i;
    if( PointList == NULL ) {
        DPrintf("PointArrayListAdd:Failed PointList is NULL.\n");
        return;
    }
    if( PointList->NumPoints == PointList->Size ) {
        //Grow
        PointList->Size *= 2;
        PointList->Points = (float *) realloc(PointList->Points, PointList->Size * sizeof(float) * PointList->Stride);
    }
    Base = PointList->NumPoints * PointList->Stride;
    for( i = 0; i < PointList->Stride; i++ ) {
        PointList->Points[Base + i] = Point[i];
    }
    PointList->NumPoints++;
}

void PointArrayListCleanUp(PointArrayList_t *PointList)
{
    if( PointList == NULL ) {
        DPrintf("PointArrayListCleanUp:Failed PointList is NULL.\n");
        return;
    }
    free(PointList->Points);
    PointList->Points = NULL;
    PointList->NumPoints = 0;
    PointList->Size = 0;
}

void PointArrayListInit(PointArrayList_t *PointList,int InitialSize,int Stride)
{
    if( PointList == NULL ) {
        DPrintf("PointArrayListInit:Failed PointList is NULL.\n");
        return;
    }
    if( InitialSize <= 0 ) {
        DPrintf("PointArrayListInit:Failed Invalid InitialSize (%i)\n",InitialSize);
        return;
    }
    
    if( Stride <= 0 ) {
        DPrintf("PointArrayListInit:Failed Invalid Stride (%i)\n",Stride);
        return;
    }
    PointList->Points = (float *) malloc(sizeof(Point_t) * InitialSize * Stride);
    PointList->Stride = Stride;
    PointList->NumPoints = 0;
    PointList->Size = InitialSize;
}
