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

    Dest = (char *) malloc(strlen(From) + 1);

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
    Result = (char *) malloc(FileSize + 1);
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
    FlowerDataset = (Flower_t *) malloc(sizeof(Flower_t));
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
    FlowerDataset = (Flower_t *) malloc(sizeof(Flower_t));
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

void DumpClusters(PointArrayList_t *Dataset,float *Centroids,int NumCentroids,int Stride,int Pass)
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
            fprintf(OutCentroidCSV,"%f,",Centroids[i * Stride + j]);
        }
        fprintf(OutCentroidCSV,"%f\n",Centroids[i * Stride + (Stride -1)]);
    }
    
    BaseChar = 'x';
    for ( i = 0; i < Stride; i++ ) {
        fprintf(OutDatasetCSV,"%c,",BaseChar + i);
    }
    fprintf(OutDatasetCSV,"centroidIndex\n");
    for( i = 0; i < Dataset->NumPoints; i++ ) {
        for( j = 0; j < Stride; j++ ) {
            fprintf(OutDatasetCSV,"%f,",Dataset->Points[i * Stride + j]);
        }
//         fprintf(OutDatasetCSV,"%i\n",Dataset->Points[i].CentroidIndex);
    }
    
    fclose(OutCentroidCSV);
    fclose(OutDatasetCSV);
}
typedef struct MinComparator_s
{
    float Value;
    int   Index;
} MinComparator_t;
#pragma omp declare reduction(NearestCentroid : MinComparator_t : omp_out = omp_in.Value < omp_out.Value ? omp_in : omp_out) \
                    initializer(omp_priv = {9999999, -1})
void KMeansClustering(PointArrayList_t *Dataset,int NumCentroids,int Stride)
{
    float *Centroids;
    float *Distances;
    int    DistancesSize;
    int   *Clusters;
    int    ClusterSize;
    float  *ClusterCounter;
    int    ClusterCounterSize;
    float  *ClusterMeans;
    int    ClusterMeansSize;
    float  Sum;
    int    i;
    int    j;
    int    k;
    int    Step;
    MinComparator_t Comparator;
    int    MaxThreadNumber;
    

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
    
    Centroids = (float *) malloc(NumCentroids * Stride * sizeof(float));
    MaxThreadNumber = omp_get_max_threads();
    printf("Selected KMeans Algorithm with a dataset of size %i and %i centroids Max Threads:%i.\n",Dataset->NumPoints,NumCentroids,
        MaxThreadNumber
    );
    
    // 1) Selects K (NumCentroids) random centroids from the dataset.
//     srand(time(0));
    #pragma omp parallel for private(j) num_threads(MaxThreadNumber) schedule(static, (NumCentroids*Stride)/MaxThreadNumber)
    for( i = 0; i < NumCentroids; i++ ) {
        for( j = 0; j < Stride; j++ ) {
            Centroids[i * Stride + j] = Dataset->Points[i * Stride +j];
        }
    }
//     for( j = 0; j < NumCentroids; j++ ) {
//         DPrintf("Centroid %i at %f;%f\n",j,Centroids[j * Stride],Centroids[j * Stride +1]);
//     }

//     HasToStop = 0;
//     NumStep = 0;
//     Sum = malloc( NumCentroids * Stride * sizeof(float));
//     ClustersCounter = malloc( NumCentroids * sizeof(int));
//     Delta = malloc( Stride * sizeof(float));
    DistancesSize = Dataset->NumPoints * NumCentroids * sizeof(float);
    Distances = (float *) malloc(DistancesSize);
    
    ClusterCounterSize = NumCentroids * sizeof(float);
    ClusterCounter = (float *) malloc(ClusterCounterSize);
    
    ClusterSize = Dataset->NumPoints * sizeof(int);
    Clusters = (int *) malloc(ClusterSize);
    
    ClusterMeansSize = NumCentroids * Stride *sizeof(float);
    ClusterMeans = (float *) malloc(ClusterMeansSize);
    
    Step = 0;
//     Comparator = malloc(sizeof(MinComparator_t));
    while( 1 ) {
//         2) Assign each points of the dataset to the nearest centroid.
//         First get the distances...
        memset(Distances,0,DistancesSize);
        #pragma omp parallel for private(j,k) num_threads(MaxThreadNumber) \
        schedule(static, (Dataset->NumPoints*NumCentroids)/MaxThreadNumber) collapse(2)
        for( i = 0; i < Dataset->NumPoints; i++ ) {
            for( j = 0; j < NumCentroids; j++ ) {
                float LocalDistance = 0.f;
                for( k = 0; k < Stride; k++ ) {
                    LocalDistance += (Centroids[j * Stride + k]
                        - Dataset->Points[i * Stride + k]) * 
                        (Centroids[j * Stride + k] - Dataset->Points[i * Stride + k]);
                }
                Distances[i * NumCentroids + j] = LocalDistance;
            }
        }
        memset(ClusterCounter,0,ClusterCounterSize);
//         #pragma omp parallel firstprivate(ClusterCounter,Clusters)
        #pragma omp parallel for schedule(guided) \
            shared(Distances,Clusters,ClusterCounter) private(j)
        for( i = 0; i < Dataset->NumPoints; i++ ) {
            float Min = INFINITY;
            int Index = -1;
//             #pragma omp parallel for reduction(NearestCentroid:Comparator)
            for( j = 0; j < NumCentroids; j++ ) {
                float LocalDistance = Distances[i * NumCentroids + j];
                if( LocalDistance < Min ) {
                    Min = LocalDistance;
                    Index = j;
                }
            }
            #pragma omp critical
            ClusterCounter[Index]++;
            Clusters[i] = Index;
        }
        memset(ClusterMeans,0,ClusterMeansSize);
        #pragma omp parallel for private(j) shared(ClusterMeans,Dataset)
        for( i = 0; i < Dataset->NumPoints; i++ ) {
            int CentroidIndex = Clusters[i];
            #pragma omp critical
            for( j = 0; j < Stride; j++ ) {
                ClusterMeans[CentroidIndex * Stride + j] += Dataset->Points[i * Stride + j];
            }
        }
        #pragma omp parallel for private(j) shared(ClusterMeans,ClusterCounter)
        for( i = 0; i < NumCentroids; i++ ) {
            if( ClusterCounter[i] == 0 ) {
                continue;
            }
            for( j = 0; j < Stride; j++ ) {
            #pragma omp critical
//                 DPrintf("New centroid %i Position:%f;%f\n",);
                ClusterMeans[i * Stride + j] /= ClusterCounter[i];
            }
        }
        Sum = 0.f;
        #pragma omp parallel for shared(ClusterMeans,Centroids) reduction(+: Sum)
        for( i = 0; i < NumCentroids * Stride; i++ ) {
            float Delta;
            float Value;
            int CentroidIndex = i / Stride;
            int StrideIndex = i % Stride;
            Delta = fabsf(ClusterMeans[CentroidIndex * Stride + StrideIndex] - Centroids[CentroidIndex * Stride + StrideIndex]);
            Value = Delta < KMEANS_ALGORITHM_TOLERANCE ? 1.f : 0.f;
            #pragma omp critical
            Sum = Sum + Value;
        }
        if( Sum == NumCentroids * Stride ) {
            break;
        }
        memcpy(Centroids,ClusterMeans,ClusterMeansSize);
        Step++;
//         break;
    }
    printf("Took %i steps to complete\n",Step);
}

PointArrayList_t *LoadPointsDataset(int *Stride)
{    
    PointArrayList_t *PointList;
    float *Point;
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

    Temp = Buffer;
    LineNumber = 0;
    LocalStride = 0;
    PointList = NULL;
    
    while( *Temp ) {
        if( LineNumber == 0 ) {
            Temp = CSVSkipLine(Temp,&LocalStride);
            assert(LocalStride != -1);
            if( PointList == NULL ) {
                PointList = (PointArrayList_t *) malloc(sizeof(PointArrayList_t));
                PointArrayListInit(PointList,64,LocalStride);
            }
            
        } else {
            Point = (float *) malloc( LocalStride * sizeof(float));
            for( i = 0; i < LocalStride; i++ ) {
                Temp = CSVGetNumberFromBuffer(Temp,&Point[i]);
            }
            PointArrayListAdd(PointList,Point);
        }
        if( *Temp == '\n' ) {
            LineNumber++;
        }
        Temp++;
    }
#if 0 /*_DEBUG*/
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
    KMeansClustering(PointList,300,Stride);
//     PointArrayListCleanUp(PointList);
//     free(PointList);
//     if( !FlowerDataset ) {
//         return -1;
//     }
    
//     free(FlowerDataset);

}
