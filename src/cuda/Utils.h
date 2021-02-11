#ifndef __UTILS_H_
#define __UTILS_H_ 

int SysMilliseconds();
void CreateDirIfNotExists(char *DirName);
int IsAlpha(char c);
int IsNumber(char c);
char *StringCopy(const char *From);
int StringToInt(char *String);
float StringToFloat(char *String);
int GetFileLength(FILE *Fp);
void DPrintf(char *Fmt, ...);
char *ReadTextFile(char *File,int Length);
char *CSVGetNumberFromBuffer(char *Buffer,float *Value);
char *CSVGetStringFromBuffer(char *Buffer,char *Value);
char *CSVSkipLine(char *Buffer,int *NumColumns);
void DumpClusters(float *Points,int NumPoints,float *Centroids,int NumCentroids,int *ClusterList,int Stride,int Pass);
#endif //__UTILS_H_ 
