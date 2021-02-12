/*
    PlaceHolder
*/
extern "C" {
#include "KMeansClustering.h"
}

#define CUDA_CHECK_RETURN(Value) CheckCudaErrorAux(__FILE__,__LINE__, #Value, Value)
void CheckCudaErrorAux(const char *File, unsigned Line,const char *Statement, cudaError_t ErrorCode)
{
   if (ErrorCode != cudaSuccess) 
   {
      fprintf(stderr,"CheckCudaErrorAux: %s returned %s %s %d\n",Statement,cudaGetErrorString(ErrorCode), File, Line);
      exit(ErrorCode);
   }
}

__global__ void CompareCentroidsKernel(int *Sum,float *Centroids,float *OldCentroids,int NumCentroids,int Stride)
{
    int ThreadIndexX;
    int ThreadIndexY;
    float Delta;
    int Value;
    ThreadIndexX = blockIdx.x * blockDim.x + threadIdx.x;
    ThreadIndexY = blockIdx.y * blockDim.y + threadIdx.y;

    if( ThreadIndexX < NumCentroids ) {
        Delta = fabsf(Centroids[ThreadIndexX * Stride + ThreadIndexY] - OldCentroids[ThreadIndexX * Stride + ThreadIndexY]);
        Value = Delta < KMEANS_ALGORITHM_TOLERANCE ? 1 : 0;
        atomicAdd(Sum,Value);
    }
}
__global__ void MeanPointsInClustersKernel(float *Centroids,int NumCentroids,float *ClusterCounter,int Stride)
{
    int ThreadIndexX;
    int ThreadIndexY;
    int NumClusters;
    
    ThreadIndexX = blockIdx.x * blockDim.x + threadIdx.x;
    ThreadIndexY = blockIdx.y * blockDim.y + threadIdx.y;
    NumClusters = ClusterCounter[ThreadIndexX];
    if( ThreadIndexX < NumCentroids && NumClusters != 0) {
        Centroids[ThreadIndexX * Stride + ThreadIndexY] /= ( ClusterCounter[ThreadIndexX] / Stride );
    }
}
__global__ void SumPointsInClustersKernel(float *Centroids,int NumCentroids,float *ClusterCounter,
                                          int *Clusters,float *Points,int NumPoints,int Stride)
{
    int ThreadIndexX;
    int ThreadIndexY;
    int CentroidIndex;
    ThreadIndexX = blockIdx.x * blockDim.x + threadIdx.x;
    ThreadIndexY = blockIdx.y * blockDim.y + threadIdx.y;
    if( ThreadIndexX < NumPoints && ThreadIndexY < Stride ) {
        CentroidIndex = Clusters[ThreadIndexX];
        atomicAdd(&(Centroids[CentroidIndex * Stride + ThreadIndexY]),Points[ThreadIndexX * Stride + ThreadIndexY]);
        atomicAdd(&(ClusterCounter[CentroidIndex]), 1.f);
    }
}
__global__ void BuildClusterListKernel(int *Clusters,float *Distances,int NumPoints,int NumCentroids,int Stride)
{
    int ThreadIndexX = blockIdx.x * blockDim.x + threadIdx.x;
    int DistanceArrayBaseIndex = ThreadIndexX * NumCentroids;
    float Min;
    int MinIndex;
    int i;

    if( ThreadIndexX < NumPoints ) {
        Min = INFINITY;
        MinIndex = 0;
        for( i = 0; i < NumCentroids; i++ ) {
            if( Distances[DistanceArrayBaseIndex + i] < Min ) {
                Min = Distances[DistanceArrayBaseIndex + i];
                MinIndex = i;
            }
        }
        Clusters[ThreadIndexX] = MinIndex;
    }
}

__global__ void ClusterComputeDistanceSquaredKernel(float *Distances,
    float *Centroids,int NumCentroids,float *Points,int NumPoints,int Stride)
{
    int ThreadIndexX;
    int ThreadIndexY;
    int CentroidIndex;
    int DatasetIndex;
    int i;
    float LocalDistance;
    
    ThreadIndexX = blockIdx.x * blockDim.x + threadIdx.x;
    ThreadIndexY = blockIdx.y * blockDim.y + threadIdx.y;
    if( ThreadIndexX < NumPoints && ThreadIndexY < NumCentroids ) {
    LocalDistance = 0.f;
        for( i = 0; i < Stride; i++ ) {
            CentroidIndex = ThreadIndexY * Stride + i;
            DatasetIndex = ThreadIndexX  * Stride + i;
            LocalDistance += (Centroids[CentroidIndex] - Points[DatasetIndex]) * 
                        (Centroids[CentroidIndex] - Points[DatasetIndex]);
        }
        Distances[ThreadIndexX * NumCentroids + ThreadIndexY] = LocalDistance;
    }
}

__global__ void CentroidsInitKernel(float *Centroids,int NumCentroids, float *Points,int NumPoints,int Stride)
{
    int ThreadIndexX;
    int ThreadIndexY;
    ThreadIndexX = blockIdx.x * blockDim.x + threadIdx.x;
    ThreadIndexY = blockIdx.y * blockDim.y + threadIdx.y;
    if( ThreadIndexX < NumCentroids && ThreadIndexY < Stride) {
        Centroids[ThreadIndexX * Stride + ThreadIndexY] = Points[ThreadIndexX * Stride + ThreadIndexY];
    }
}

int CudaCompareCentroidsList(int *DeviceSum,float *DeviceCentroidList,float *DeviceOldCentroidList,int NumCentroids,int Stride)
{
    dim3   BlockSize;
    dim3   GridSize;
    int    Sum;

    BlockSize = dim3(256,Stride,1);
    GridSize = dim3((NumCentroids + BlockSize.x - 1) / BlockSize.x,1,1);
    
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceSum,0,sizeof(int)));
    CompareCentroidsKernel<<<GridSize,BlockSize>>>(DeviceSum,DeviceCentroidList,DeviceOldCentroidList,NumCentroids,Stride);

    CUDA_CHECK_RETURN(cudaMemcpy(&Sum,DeviceSum,sizeof(int),cudaMemcpyDeviceToHost));
    
    return Sum == (NumCentroids * Stride) ? 1 : 0;
}
void CudaComputeDistances(float *DeviceDistanceList,float *DeviceCentroidList,int NumCentroids,float *DevicePointList,
                            int NumPoints,int Stride)
{
    dim3   BlockSize;
    dim3   GridSize;
    BlockSize = dim3(32,32,1);
    GridSize = dim3((NumPoints + BlockSize.x - 1) / BlockSize.x ,(NumCentroids + BlockSize.y - 1) / BlockSize.y ,1);
    ClusterComputeDistanceSquaredKernel<<<GridSize,BlockSize>>>
        (DeviceDistanceList,DeviceCentroidList,NumCentroids,DevicePointList,NumPoints,Stride);
}
void CudaBuildClusterList(int *DeviceClusterList,float *DeviceDistanceList,int NumPoints,int NumCentroids,int Stride)
{
    dim3   BlockSize;
    dim3   GridSize;

    BlockSize = dim3(256,1,1);
    GridSize = dim3((NumPoints + BlockSize.x - 1) / BlockSize.x,1,1);
    BuildClusterListKernel<<<GridSize,BlockSize>>>(DeviceClusterList,DeviceDistanceList,NumPoints,NumCentroids,Stride);
}
void CudaSumPointsInClusters(float *DeviceCentroidList,int NumCentroids,int *DeviceClusterList,float *DeviceClusterCounter,
                             float *DevicePointList,int NumPoints,int Stride)
{
    dim3   BlockSize;
    dim3   GridSize;

    BlockSize = dim3(256,Stride,1);
    GridSize = dim3((NumPoints + BlockSize.x - 1) / BlockSize.x,1,1);

    SumPointsInClustersKernel<<<GridSize,BlockSize>>>(DeviceCentroidList,NumCentroids,DeviceClusterCounter,
                                                      DeviceClusterList,DevicePointList,NumPoints,Stride);
}

void CudaMeanPointsInClusters(float *DeviceCentroidList,int NumCentroids,float *DeviceClusterCounter,int Stride)
{
    dim3   BlockSize;
    dim3   GridSize;
    
    BlockSize = dim3(256,Stride,1);
    GridSize = dim3((NumCentroids + BlockSize.x - 1) / BlockSize.x,1,1);
    
    MeanPointsInClustersKernel<<<GridSize,BlockSize>>>(DeviceCentroidList,NumCentroids,DeviceClusterCounter,Stride);
}


void CudaUpdateCentroidList(float *DeviceCentroidList,int NumCentroids,float *DevicePoints,int NumPoints,int *DeviceClusterList,
                            float *DeviceClusterCounter,int Stride)
{
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceCentroidList,0,NumCentroids * Stride * sizeof(float)));
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceClusterCounter,0,NumCentroids * sizeof(float)));
    
    CudaSumPointsInClusters(DeviceCentroidList,NumCentroids,DeviceClusterList,DeviceClusterCounter,DevicePoints,
                            NumPoints,Stride);
    CudaMeanPointsInClusters(DeviceCentroidList,NumCentroids,DeviceClusterCounter,Stride);
}
float *CudaInitCentroids(int NumCentroids,float *DevicePointList,int NumPoints,int Stride)
{
    float *DeviceCentroidOutputList;
    int    CentroidListSize;
    dim3   BlockSize;
    dim3   GridSize;

    CentroidListSize = NumCentroids * Stride * sizeof(float);
    
    CUDA_CHECK_RETURN(cudaMalloc((void**)&DeviceCentroidOutputList,CentroidListSize));
    
    BlockSize = dim3(64, Stride, 1);
    GridSize = dim3((NumCentroids + BlockSize.x - 1) / BlockSize.x,1,1);
    
    CentroidsInitKernel<<<GridSize,BlockSize>>>
            (DeviceCentroidOutputList,NumCentroids,DevicePointList,
             NumPoints,Stride);
    
    return DeviceCentroidOutputList;
}

float *CudaInitClusterCounter(int NumCentroids)
{
    float *DeviceClusterCounter;
    int  ClusterCounterSize;

    ClusterCounterSize = NumCentroids * sizeof(float);

    CUDA_CHECK_RETURN(cudaMalloc((void **)&DeviceClusterCounter, ClusterCounterSize));
    
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceClusterCounter,0,ClusterCounterSize));

    return DeviceClusterCounter; 
}
int *CudaInitAssignments(PointArrayList_t *PointList)
{
    int *DeviceClusterList;
    int    ClusterListSize;

    ClusterListSize = PointList->NumPoints * sizeof(int);

    CUDA_CHECK_RETURN(cudaMalloc((void **)&DeviceClusterList, ClusterListSize));
    
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceClusterList,-1,ClusterListSize));

    return DeviceClusterList;
}
float *CudaInitDistances(int NumCentroids,int NumPoints)
{
    float *DeviceDistanceList;
    int    DistanceListSize;

    DistanceListSize = NumPoints * NumCentroids * sizeof(float);

    CUDA_CHECK_RETURN(cudaMalloc((void **)&DeviceDistanceList, DistanceListSize));
    
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceDistanceList,0.f,DistanceListSize));

    return DeviceDistanceList;
}
float *CudaInitPointList(PointArrayList_t *PointList)
{
    float *DevicePointList;
    int    PointListSize;
    
    PointListSize = PointList->NumPoints * PointList->Stride * sizeof(float);
    CUDA_CHECK_RETURN(cudaMalloc((void **)&DevicePointList, PointListSize));
    CUDA_CHECK_RETURN(
        cudaMemcpy(DevicePointList,PointList->Points, 
                   PointListSize,cudaMemcpyHostToDevice));
    return DevicePointList;
}
void CudaMain(int NumCentroids,int Stride,PointArrayList_t *PointList)
{
    float *DevicePointList;
    float *DeviceCentroidList;
    float *DeviceOldCentroidList;
    float *DeviceDistanceList;
    int   *DeviceClusterList;
    int    OldCentroidListSize;
    float *DeviceClusterCounter;
    int   *DeviceSum;
    int    Sum;
    int    SumSize;
    int    Step;
    
    DevicePointList = CudaInitPointList(PointList);
    DeviceCentroidList = CudaInitCentroids(NumCentroids,DevicePointList,PointList->NumPoints,Stride);
    OldCentroidListSize = NumCentroids * Stride * sizeof(float);
    CUDA_CHECK_RETURN(cudaMalloc((void **)&DeviceOldCentroidList,OldCentroidListSize));
    DeviceClusterCounter = CudaInitClusterCounter(NumCentroids);
    DeviceClusterList = CudaInitAssignments(PointList);
    DeviceDistanceList = CudaInitDistances(NumCentroids,PointList->NumPoints);
    SumSize = sizeof(int);
    CUDA_CHECK_RETURN(cudaMalloc((void **)&DeviceSum,SumSize));
    
    Step = 0;
    
    while( 1 ) {
        CudaComputeDistances(DeviceDistanceList,DeviceCentroidList,NumCentroids,DevicePointList,PointList->NumPoints,Stride);
        CudaBuildClusterList(DeviceClusterList,DeviceDistanceList,PointList->NumPoints,NumCentroids,Stride);
        cudaMemcpy(DeviceOldCentroidList,DeviceCentroidList,OldCentroidListSize,cudaMemcpyDeviceToDevice);
        CudaUpdateCentroidList(DeviceCentroidList,NumCentroids,DevicePointList,PointList->NumPoints,DeviceClusterList,
                            DeviceClusterCounter,Stride);
        Sum = CudaCompareCentroidsList(DeviceSum,DeviceCentroidList,DeviceOldCentroidList,NumCentroids,Stride);
        if( Sum == 1 ) {
            break;
        }
        Step++;
    }

    cudaFree(DevicePointList);
    cudaFree(DeviceCentroidList);
    cudaFree(DeviceOldCentroidList);
    cudaFree(DeviceDistanceList);
    cudaFree(DeviceClusterList);
    cudaFree(DeviceClusterCounter);
}
int main(int argc,char** argv)
{
    PointArrayList_t *PointList;
    long Start;
    long Delta;
    int Stride;
    PointList = LoadPointsDataset(&Stride);
    
    if( PointList == NULL ) {
        DPrintf("Couldn't load point dataset.\n");
        return -1;
    }
    Start = SysMilliseconds();
    CudaMain(1000,Stride,PointList);
    Delta = SysMilliseconds() - Start;
	printf("Time: %f seconds\r\n", Delta * 0.001f);
    
    
    PointArrayListCleanUp(PointList);
    
    free(PointList);
}
