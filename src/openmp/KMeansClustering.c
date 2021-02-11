#include "KMeansClustering.h"

int StartSeconds = 0;
int SysMilliseconds()
{
    struct timeval tp;
    int CTime;

    gettimeofday(&tp, NULL);

    if ( !StartSeconds ){
        StartSeconds = tp.tv_sec;
        return tp.tv_usec/1000;
    }

    CTime = (tp.tv_sec - StartSeconds)*1000 + tp.tv_usec / 1000;

    return CTime;
}

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

int StringToInt(char *String)
{
    char *EndPtr;    
    long Value;
    
    Value = strtol(String, &EndPtr, 10);
    
    if( errno == ERANGE && Value == LONG_MIN ) {
        DPrintf("StringToInt %s (%lu) invalid...underflow occurred\n",String,Value);
        return 0;
    } else if( errno == ERANGE && Value == LONG_MAX ) {
        DPrintf("StringToInt %s (%lu) invalid...overflow occurred\n",String,Value);
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

void PrintPoint(float *Position,int Stride)
{
    int i;
    DPrintf("Point: ");
    for( i = 0; i < Stride; i++ ) {
        DPrintf("%f;",Position[i]);
    }
    DPrintf("\n");
}

void DumpClusters(PointArrayList_t *Dataset,float *Centroids,int NumCentroids,int *Clusters,int Stride,int Pass)
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
        fprintf(OutDatasetCSV,"%i\n",Clusters[i]);
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
    int   *ClusterCounter;
    int    ClusterCounterSize;
    float  *ClusterMeans;
    int    ClusterMeansSize;
    int    Sum;
    int    Start;
    int    End;
//     int    i;
//     int    j;
//     int    k;
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
    printf("Selected KMeans Algorithm with a dataset of size %i and %i centroids Max Threads:%i.\n",
           Dataset->NumPoints,NumCentroids,MaxThreadNumber
    );
    
    Start = SysMilliseconds();
    // 1) Selects K (NumCentroids) random centroids from the dataset.
//     srand(time(0));
    #pragma omp parallel for shared(Centroids) \
    num_threads(MaxThreadNumber) schedule(static, (NumCentroids*Stride)/MaxThreadNumber)
    for( int i = 0; i < NumCentroids; i++ ) {
        for( int j = 0; j < Stride; j++ ) {
            #pragma omp atomic write
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
    
    ClusterCounterSize = NumCentroids * sizeof(int);
    ClusterCounter = (int *) malloc(ClusterCounterSize);
    
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
        #pragma omp parallel for firstprivate(Centroids,Dataset,Stride) /*private(j,k)*/ /*num_threads(MaxThreadNumber)*/ \
//         schedule(static, (Dataset->NumPoints*NumCentroids)/MaxThreadNumber)
        for( int i = 0; i < Dataset->NumPoints; i++ ) {
            for( int j = 0; j < NumCentroids; j++ ) {
                float LocalDistance = 0.f;
                for( int k = 0; k < Stride; k++ ) {
                    LocalDistance += (Centroids[j * Stride + k]
                        - Dataset->Points[i * Stride + k]) * 
                        (Centroids[j * Stride + k] - Dataset->Points[i * Stride + k]);
                }
                #pragma omp atomic write
                Distances[i * NumCentroids + j] = LocalDistance;
            }
        }
        memset(ClusterCounter,0,ClusterCounterSize);
        memset(Clusters,0,ClusterSize);

//         #pragma omp parallel firstprivate(ClusterCounter,Clusters)
        #pragma omp parallel for /*schedule(guided)*/ \
            \
            firstprivate(Dataset,Stride) shared(Distances,ClusterMeans,ClusterCounter) /*private(j)*/
        for( int i = 0; i < Dataset->NumPoints; i++ ) {
            float Min = INFINITY;
            int Index = -1;
            float LocalDistance = 0.f;
//             #pragma omp parallel for reduction(NearestCentroid:Comparator)
            for( int j = 0; j < NumCentroids; j++ ) {
                LocalDistance = Distances[i * NumCentroids + j];
                if( LocalDistance < Min ) {
                    Min = LocalDistance;
                    Index = j;
                }
            }
            #pragma omp atomic write
            Clusters[i] = Index;
            #pragma omp atomic
            ClusterCounter[Index]++;
        }
        
        memset(ClusterMeans,0,ClusterMeansSize);

        #pragma omp parallel for firstprivate(ClusterCounter) shared(ClusterMeans)
        for( int i = 0; i < Dataset->NumPoints * Stride; i++ ) {
            int PointIndex = i / Stride;
            int StrideIndex = i % Stride;
            int CentroidIndex = Clusters[PointIndex];
            int LocalAddValue = Dataset->Points[PointIndex * Stride + StrideIndex];
            #pragma omp atomic
            ClusterMeans[CentroidIndex * Stride + StrideIndex] += LocalAddValue;
        }
//         break;
        #pragma omp parallel for firstprivate(ClusterCounter) shared(ClusterMeans)
        for( int i = 0; i < NumCentroids * Stride; i++ ) {
            int CentroidIndex = i / Stride;
            int StrideIndex = i % Stride;
            int NumClusters = ClusterCounter[CentroidIndex]; 
            if( NumClusters == 0 ) {
                continue;
            }
            #pragma omp atomic
            ClusterMeans[CentroidIndex * Stride + StrideIndex] /= (float) NumClusters;
        }
        
        Sum = 0;
        #pragma omp parallel for reduction(+: Sum)
        for( int i = 0; i < NumCentroids * Stride; i++ ) {
            float Delta;
            int   Value;
            int CentroidIndex = i / Stride;
            int StrideIndex = i % Stride;
            Delta = fabsf(ClusterMeans[CentroidIndex * Stride + StrideIndex] - Centroids[CentroidIndex * Stride + StrideIndex]);
            Value = Delta < KMEANS_ALGORITHM_TOLERANCE ? 1 : 0;
//             #pragma omp atomic
            Sum = Sum + Value;
        }
        if( Sum == NumCentroids * Stride ) {
            break;
        }
        memcpy(Centroids,ClusterMeans,ClusterMeansSize);
        Step++;
//         break;
    }
    End = SysMilliseconds();
    printf("Took %i steps to complete %i ms elapsed\n",Step,End-Start);
    DumpClusters(Dataset,Centroids,NumCentroids,Clusters,Stride,0);
    free(Centroids);
    free(Distances);
    free(ClusterCounter);
    free(Clusters);
    free(ClusterMeans);
}

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
        printf("LoadPointsDataset:Invalid file.\n");
        return NULL;
    }
    
    Buffer = ReadTextFile(File,0);
    if( Buffer == NULL ) {
        DPrintf("LoadPointsDataset:Couldn't read file %s\n",File);
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
            free(Point);
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
    int NumClusters;
    int Stride;

    if( argc != 3 ) {
        printf("Usage:%s <Dataset File> <Number of Clusters>\n",argv[0]);
        return -1;
    }
    PointList = LoadPointsDataset(argv[1],&Stride);
    
    if( PointList == NULL ) {
        DPrintf("Couldn't load point dataset.\n");
        return -1;
    }
    NumClusters = StringToInt(argv[2]);
    KMeansClustering(PointList,NumClusters,Stride);
    PointArrayListCleanUp(PointList);
    free(PointList);
}
