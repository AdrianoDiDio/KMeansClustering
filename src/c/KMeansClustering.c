#include "KMeansClustering.h"

void CreateDirIfNotExists(char *DirName) {
    struct stat FileStat;

    if (stat(DirName, &FileStat) == -1) {
#ifdef _WIN32
        mkdir(DirName);
#else
        mkdir(DirName, 0700);
#endif
    }
}

void PointSubtract(float *PointA,float *PointB,float *PointOut,int Stride)
{
    int i;
    for( i = 0; i < Stride; i++ ) {
        PointOut[i] = PointA[i] - PointB[i];
    }
}

float PointDistanceSquared(float *PointA,float *PointB,int Stride)
{
    float Sum;
    int i;
    Sum = 0.f;
    for( i = 0; i < Stride; i++ ) {
        Sum += (PointB[i] - PointA[i]) * (PointB[i] - PointA[i]);
    }
    return Sum;
}
int IsAlpha(char c)
{
    if( !c ){
        return 0;
    }

    if( ( c >= 'A' && c <= 'Z' ) ||
        ( c >= 'a' && c <= 'z' ) ){
        return 1;
    }

    return 0;
}

int IsNumber(char c)
{
    if( !c ){
        return 0;
    }

    if( ( c >= '0' && c <= '9' ) ){
        return 1;
    }

    return 0;
}

char *StringCopy(const char *From)
{
    char *Dest;

    Dest = malloc(strlen(From) + 1);

    if ( !Dest ) {
        return NULL;
    }

    strcpy(Dest, From);
    return Dest;
}
void DPrintf(char *Fmt, ...)
{
    char Temp[1000];
    va_list arglist;

    va_start(arglist, Fmt);
    vsnprintf(Temp, sizeof( Temp ), Fmt, arglist);
#ifdef _DEBUG
    fputs(Temp, stdout);
#endif
    va_end(arglist);
}

float StringToFloat(char *String)
{
    char *EndPtr;    
    float Value;
    
    Value = strtof(String, &EndPtr);
    
    if( errno == ERANGE && Value == -HUGE_VALF ) {
        DPrintf("StringToFloat %s (%f) invalid...underflow occurred\n",String,Value);
        return 0;
    } else if( errno == ERANGE && Value == HUGE_VALF) {
        DPrintf("StringToFloat %s (%f) invalid...overflow occurred\n",String,Value);
        return 0;
    }
    return Value;
}

int GetFileLength(FILE *Fp)
{
    int Length;
    int CurrentPosition;

    if ( !Fp ) {
        return -1; //Must be a valid file
    }

    CurrentPosition = ftell(Fp);
    fseek(Fp, 0, SEEK_END);
    Length = ftell(Fp);
    fseek(Fp, CurrentPosition, SEEK_SET);

    return Length;
}

char *ReadTextFile(char *File,int Length)
{
    FILE *Fp;
    int FileSize;
    char *Result;
    int Ret;
    
    Fp = fopen(File,"r");
    
    if( !Fp ) {
        DPrintf("File %s not found.\n",File);
        return NULL;
    }
    FileSize = Length != 0 ? Length : GetFileLength(Fp);
    Result = malloc(FileSize + 1);
    Ret = fread(Result,1, FileSize,Fp);
    if( Ret != FileSize ) {
        DPrintf("Failed to read file %s\n",File);
        return NULL;
    }
    Result[Ret] = '\0';
    fclose(Fp);
    return Result;
}

void FlowerPrint(Flower_t *Flower)
{
    if( Flower == NULL ){
        return;
    }
    DPrintf("Species:%s\n",Flower->Species);
    DPrintf("Sepal Length:%f\n",Flower->SepalLength);
    DPrintf("Sepal Width:%f\n",Flower->SepalWidth);
    DPrintf("Petal Length:%f\n",Flower->PetalLength);
    DPrintf("Petal Length:%f\n",Flower->PetalWidth);
}
Flower_t *LoadIrisDatasetOld()
{

    Flower_t *FlowerDataset;
    Flower_t Iterator;
    FILE *CSVDatasetFile;
    int RetValue;
    
    CSVDatasetFile = fopen("Dataset/iris.csv","r");
    if( CSVDatasetFile == NULL ) {
        DPrintf("LoadIrisDataset:Couldn't open dataset...\n");
        return NULL;
    }
    FlowerDataset = malloc(sizeof(Flower_t));
    FlowerDataset->Next = NULL;
    char Head1[256];
    char Head2[256];
    char Head3[256];
    char Head4[256];
    char Head5[256];
    RetValue = fscanf(CSVDatasetFile,"%s,%s,%s,%s,%s\n",Head1,Head2,Head3,Head4,Head5);
    while( 1 ) {
        RetValue = fscanf(CSVDatasetFile,"%f,%f,%f,%f,%s\n",&Iterator.SepalLength,&Iterator.SepalWidth,&Iterator.PetalLength,
            &Iterator.PetalWidth,Iterator.Species);
        if( RetValue != 5 ) {
            DPrintf("Done or found an invalid line...\n");
            break;
        }
        DPrintf("Loaded %f;%f;%f;%f; Species %s\n",Iterator.SepalLength,Iterator.SepalWidth,Iterator.PetalLength,
            Iterator.PetalWidth,Iterator.Species);
    }
    return FlowerDataset; 
}
char *CSVGetNumberFromBuffer(char *Buffer,float *Value)
{
    int i = 0;
    char String[256];
    if( Value == NULL ) {
        return Buffer;
    }
    do {
        if( *Buffer == '\r' ) {
            Buffer++;
            continue;
        }
        if( *Buffer == '\n' ) {
            break;
        }
        if( *Buffer == ',' ) {
            Buffer++;
            break;
        }
        String[i] = *Buffer;
        Buffer++;
        i++;
    } while ( IsNumber(*Buffer) || *Buffer == '.' || *Buffer == ',' || *Buffer == '\n' || *Buffer == '\r');
    String[i] = '\0';
    *Value = StringToFloat(String);
    return Buffer;
}

char *CSVGetStringFromBuffer(char *Buffer,char *Value)
{
    int i = 0;
    char String[256];

    do {
        if( *Buffer == '\n' ) {
            break;
        }
        if( *Buffer == ',' ) {
            Buffer++;
            break;
        }
        String[i] = *Buffer;
        Buffer++;
        i++;
    } while ( IsAlpha(*Buffer) || *Buffer == '.' || *Buffer == '-' || *Buffer == ',' || *Buffer == '\n');
    String[i] = '\0';
    if( Value == NULL ) {
        Value = StringCopy(String);
    } else {
        strcpy(Value,String);
    }
    return Buffer;
}

char *CSVSkipLine(char *Buffer,int *NumColumns)
{
    int LocalNumColumns;
    LocalNumColumns = -1;
    do {
        if( IsAlpha(*Buffer) ) {
            if( LocalNumColumns == -1 ) {
                LocalNumColumns = 1;
            }
        }
        if( *Buffer == ',' ) {
            LocalNumColumns++;
        }
        if( *Buffer == '\n' ) {
            break;
        }
        Buffer++;
    } while( *Buffer );
    if( NumColumns != NULL ) {
        *NumColumns = LocalNumColumns;
    }
    return Buffer;
}

Flower_t *LoadIrisDataset()
{
    Flower_t *FlowerDataset;
    Flower_t Iterator;
    char *Buffer;

//     char *Line;
//     int RetValue;
//     int i;
    

    Buffer = ReadTextFile("Dataset/iris.csv",0);
    if( Buffer == NULL ) {
        DPrintf("Couldn't read file\n");
        return NULL;
    }
    FlowerDataset = malloc(sizeof(Flower_t));
    FlowerDataset->Next = NULL;
    
    int LineNumber = 0;
    while( *Buffer ) {
        //Buffer a line
        if( LineNumber == 0 ) {
            Buffer = CSVSkipLine(Buffer,NULL);
        } else {
            Buffer = CSVGetNumberFromBuffer(Buffer,&Iterator.SepalLength);
            Buffer = CSVGetNumberFromBuffer(Buffer,&Iterator.SepalWidth);
            Buffer = CSVGetNumberFromBuffer(Buffer,&Iterator.PetalLength);
            Buffer = CSVGetNumberFromBuffer(Buffer,&Iterator.PetalWidth);
            Buffer = CSVGetStringFromBuffer(Buffer,Iterator.Species);
            FlowerPrint(&Iterator);
        }
        if( *Buffer == '\n' ) {
            LineNumber++;
        }
        Buffer++;
    }
    DPrintf("File has %i lines\n",LineNumber);
    return FlowerDataset; 
}

void PrintPoint(float *Position,int Stride)
{
    int i;
    DPrintf("Point: ");
    for( i = 0; i < Stride; i++ ) {
        DPrintf("%f;",Position[i]);
    }
    DPrintf("\n");
}

void DumpClusters(PointArrayList_t *Dataset,Centroid_t *Centroids,int NumCentroids,int Stride,int Pass)
{
    FILE *OutCentroidCSV;
    FILE *OutDatasetCSV;
    char OutFile[256];
    int BaseChar;
    int i;
    int j;
    
    //Write 2 CVS 1 for the centroids 1 for the dataset
    if( !Dataset ) {
        DPrintf("DumpClusters:Invalid Dataset.\n");
        return;
    }
    if( !Centroids ) {
        DPrintf("DumpClusters:Invalid Centroids data.\n");
        return;
    }
    if( NumCentroids <= 0 ) {
        DPrintf("DumpClusters:Invalid number of centroids.\n");
        return;
    }
    CreateDirIfNotExists("Out");
    sprintf(OutFile,"Out/out_centroids_%i.csv",Pass);
    OutCentroidCSV = fopen(OutFile,"w");
    sprintf(OutFile,"Out/out_dataset_%i.csv",Pass);
    OutDatasetCSV = fopen(OutFile,"w");
    
    BaseChar = 'x';
    for ( i = 0; i < Stride - 1; i++ ) {
        fprintf(OutCentroidCSV,"%c,",BaseChar + i);
    }
    fprintf(OutCentroidCSV,"%c\n",BaseChar + (Stride - 1));
    for( i = 0; i < NumCentroids; i++ ) {
        for( j = 0; j < Stride - 1; j++ ) {
            fprintf(OutCentroidCSV,"%f,",Centroids[i].Position[j]);
        }
        fprintf(OutCentroidCSV,"%f\n",Centroids[i].Position[Stride - 1]);
    }
    
    BaseChar = 'x';
    for ( i = 0; i < Stride; i++ ) {
        fprintf(OutDatasetCSV,"%c,",BaseChar + i);
    }
    fprintf(OutDatasetCSV,"centroidIndex\n");
    for( i = 0; i < Dataset->NumPoints; i++ ) {
        for( j = 0; j < Stride; j++ ) {
            fprintf(OutDatasetCSV,"%f,",Dataset->Points[i].Position[j]);
        }
        fprintf(OutDatasetCSV,"%i\n",Dataset->Points[i].CentroidIndex);
    }
    
    fclose(OutCentroidCSV);
    fclose(OutDatasetCSV);
}
void KMeansClustering(PointArrayList_t *Dataset,int NumCentroids,int Stride)
{
    Centroid_t *Centroids;
    float Min;
    float Distance;
    float ClusterSize;
    float *Sum;
    float *Delta;
    int i;
    int j;
    int k;
    int NumStep;
    int HasToStop;
    int NumClustersSet;
    
    if( !Dataset ) {
        DPrintf("KMeansClustering:Invalid Dataset.\n");
        return;
    }
    if( NumCentroids <= 0 ) {
        DPrintf("KMeansClustering:Invalid Number of centroids %i\n",NumCentroids);
        return;
    }
    
    if( Stride <= 0 ) {
        DPrintf("KMeansClustering:Invalid Stride %i\n",Stride);
        return;
    }
    Centroids = malloc(sizeof(Centroid_t) * NumCentroids );
    
    printf("Selected KMeans Algorithm with a dataset of size %i and %i centroids.\n",Dataset->NumPoints,NumCentroids);
    
    // 1) Selects K (NumCentroids) random centroids from the dataset.
    srand(time(0)); 
    for( i = 0; i < NumCentroids; i++ ) {
        Centroids[i].Position = malloc(Stride * sizeof(float));
//         DPrintf("Centroid %i: ",i);
        for( j = 0; j < Stride; j++ ) {
//         Index = (rand() % (Dataset->NumPoints + 1)); 
            Centroids[i].Position[j] = Dataset->Points[/*Index*/i].Position[j];
//             DPrintf("%f;",Centroids[i].Position[j]);
        }
//         DPrintf("\n");
        Centroids[i].Stride = Stride;
    }
    //Test with 5 steps...
    HasToStop = 0;
    NumStep = 0;
    Sum = malloc( Stride * sizeof(float));
    Delta = malloc( Stride * sizeof(float));
    while( !HasToStop ) {
        // 2) Assign each points of the dataset to the nearest centroid.
        for( i = 0; i < Dataset->NumPoints; i++ ) {
            Min = INFINITY;
            for( j = 0; j < NumCentroids; j++ ) {
                Distance = PointDistanceSquared(Dataset->Points[i].Position,Centroids[j].Position,Stride);
                if( Distance < Min ) {
                    Dataset->Points[i].CentroidIndex = j;
                    Min = Distance;
                }
            }
        }
//         DumpClusters(Dataset,Centroids,NumCentroids,0);
        // 3) Recalculate centroid position based on the new clusters.
        NumClustersSet = 0;
        for( i = 0; i < NumCentroids; i++ ) {
            memset(Sum,0,Stride * sizeof(float));
            ClusterSize = 0;
            for( j = 0; j < Dataset->NumPoints; j++ ) {
                if( Dataset->Points[j].CentroidIndex != i ) {
                    continue;
                }
                for( k = 0; k < Stride; k++ ) {
                    Sum[k] += Dataset->Points[j].Position[k];
                }
                ClusterSize++;
            }
            if( ClusterSize == 0.f ) {
                continue;
            }
            for( k = 0; k < Stride; k++ ) {
                Sum[k] /= ClusterSize;
            }
            PointSubtract(Sum,Centroids[i].Position,Delta,Stride);
//             DPrintf("ClusterSize %f Old Position %f;%f | New Position: %f;%f | Delta: %f;%f\n",ClusterSize,
//                     Sum.x,Sum.y,Centroids[i].Position.x,
//                 Centroids[i].Position.y,Delta.x,Delta.y
//             );
            for( k = 0; k < Stride; k++ ) {
                if( fabsf(Delta[k]) > KMEANS_ALGORITHM_TOLERANCE) {
                    break;
                }
            }
            if( k == Stride ) {
                NumClustersSet++;
            }
            memcpy(Centroids[i].Position,Sum,Stride * sizeof(float));
//             DPrintf("New Centroid Position for %i is %f;%f or %f;%f\n",i,Sum[0],Sum[1],
//                 Centroids[i].Position[0],Centroids[i].Position[1]
//             );
        }
        if( NumClustersSet == NumCentroids ) {
            break;
        }
        if( NumStep < 3 ) {
            DumpClusters(Dataset,Centroids,NumCentroids,Stride,NumStep);
        }
        NumStep++;
    }
    printf("KMeansAlgorithm has finished...took %i steps to complete\n",NumStep);
    DumpClusters(Dataset,Centroids,NumCentroids,Stride,NumStep);
    for( i = 0; i < NumCentroids; i++ ) {
        free(Centroids[i].Position);
    }
    free(Centroids);
    free(Sum);
    free(Delta);
}

PointArrayList_t *LoadPointsDataset(int *Stride)
{    
    PointArrayList_t *PointList;
    Point_t Iterator;
    char *Buffer;
    char *Temp;
    int LineNumber;
    int LocalStride;
    int i;
    
    Buffer = ReadTextFile("Dataset/data_blob.csv",0);
    if( Buffer == NULL ) {
        DPrintf("Couldn't read file\n");
        return NULL;
    }
    PointList = malloc(sizeof(PointArrayList_t));
    PointArrayListInit(PointList,64);    Temp = Buffer;
    LineNumber = 0;
    LocalStride = 0;
    
    while( *Temp ) {
        if( LineNumber == 0 ) {
            Temp = CSVSkipLine(Temp,&LocalStride);
            assert(LocalStride != -1);
        } else {
            Iterator.Stride = LocalStride;
            Iterator.Position = malloc( Iterator.Stride * sizeof(float));
            for( i = 0; i < LocalStride; i++ ) {
                Temp = CSVGetNumberFromBuffer(Temp,&Iterator.Position[i]);
            }
            PointArrayListAdd(PointList,Iterator);
        }
        if( *Temp == '\n' ) {
            LineNumber++;
        }
        Temp++;
    }
#if 1 /*_DEBUG*/
//     int i;
    for( i = 0; i < PointList->NumPoints; i++ ) {
        PrintPoint(PointList->Points[i].Position,LocalStride);
    }
    DPrintf("Read %i points || %i lines\n",PointList->NumPoints,LineNumber);
#endif
    if( Stride != NULL ) {
        *Stride = LocalStride;
    }
    free(Buffer);
    return PointList; 
}
int main(int argc,char** argv)
{
    PointArrayList_t *PointList;
    int Stride;
//     Flower_t *FlowerDataset;
    
//     FlowerDataset = LoadIrisDataset();
    PointList = LoadPointsDataset(&Stride);
    
    if( PointList == NULL ) {
        DPrintf("Couldn't load point dataset.\n");
        return -1;
    }
    KMeansClustering(PointList,5,Stride);
    PointArrayListCleanUp(PointList);
    free(PointList);
//     if( !FlowerDataset ) {
//         return -1;
//     }
    
//     free(FlowerDataset);

}
