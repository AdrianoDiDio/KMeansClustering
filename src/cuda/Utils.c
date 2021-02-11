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

void DumpClusters(float *Points,int NumPoints,float *Centroids,int NumCentroids,int *Clusters,int Stride,int Pass)
{
    FILE *OutCentroidCSV;
    FILE *OutDatasetCSV;
    char OutFile[256];
    int i;
    int j;
    
    //Write 2 CVS 1 for the centroids 1 for the dataset
    if( !Points ) {
        DPrintf("DumpClusters:Invalid Dataset.\n");
        return;
    }
    if( !Centroids ) {
        DPrintf("DumpClusters:Invalid Centroids data.\n");
        return;
    }
    if( !Clusters ) {
        DPrintf("DumpClusters:Invalid Cluster List data.\n");
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
    
    fprintf(OutCentroidCSV,"x,y\n");
    for( i = 0; i < NumCentroids; i++ ) {
        for( j = 0; j < Stride; j++ ) {
            fprintf(OutCentroidCSV,"%f,",Centroids[i * Stride + j]);
        }
        fprintf(OutCentroidCSV,"\n");
    }
    
    fprintf(OutDatasetCSV,"x,y,centroidIndex\n");
    for( i = 0; i < NumPoints; i++ ) {
        for( j = 0; j < Stride; j++ ) {
            fprintf(OutDatasetCSV,"%f,",Points[i * Stride + j]);
        }
        fprintf(OutDatasetCSV,"%i\n",Clusters[i]);
    }
    
    fclose(OutCentroidCSV);
    fclose(OutDatasetCSV);
}
 
