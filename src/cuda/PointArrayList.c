#include "KMeansClustering.h"


PointArrayList_t *LoadPointsDataset(char *File,int *Stride)
{    
    PointArrayList_t *PointList;
    float *Point;
    char *Buffer;
    char *Temp;
    int LineNumber;
    int LocalStride;
    int i;
    
    if( !File ) {
        printf("LoadPointsDataset:Invalid File\n");
        return NULL;
    }
    
    Buffer = ReadTextFile(File,0);
    
    if( Buffer == NULL ) {
        DPrintf("Couldn't read file\n");
        return NULL;
    }

    PointList = NULL;
    Temp = Buffer;
    LineNumber = 0;
    LocalStride = 0;
    
    while( *Temp ) {
        if( LineNumber == 0 ) {
            Temp = CSVSkipLine(Temp,&LocalStride);
            assert(LocalStride != -1);
            if( PointList == NULL ) {
                PointList = malloc(sizeof(PointArrayList_t));
                PointArrayListInit(PointList,64,LocalStride);
            }
        } else {
            Point = malloc( LocalStride * sizeof(float));
            for( i = 0; i < LocalStride; i++ ) {
                Temp = CSVGetNumberFromBuffer(Temp,&Point[i]);
            }
            PointArrayListAdd(PointList,Point);
            free(Point);
        }
        if( *Temp == '\n' ) {
            LineNumber++;
        }
        Temp++;
    }
#if 0 /*_DEBUG*/
    int i,j,base;
    for( i = 0; i < PointList->NumPoints; i++ ) {
        base = i * PointList->Stride;
        DPrintf("Point at ");
        for( j = 0; j < PointList->Stride; j++ ) {
            DPrintf("%i;",base+j);
        }
        DPrintf("\n");
//         PrintVec2(PointList->Points[i].Position);
    }
    DPrintf("Read %i points || %i lines\n",PointList->NumPoints,LineNumber);
#endif
    if( Stride != NULL ) {
        *Stride = LocalStride;
    }
    free(Buffer);
    return PointList; 
}

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
        PointList->Points = realloc(PointList->Points, PointList->Size * sizeof(Point_t) * PointList->Stride);
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
    PointList->Points = malloc(sizeof(Point_t) * InitialSize * Stride);
    PointList->Stride = Stride;
    PointList->NumPoints = 0;
    PointList->Size = InitialSize;
}
