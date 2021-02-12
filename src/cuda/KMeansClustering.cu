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
        //NOTE(Adriano):This gets added Stride times for each point...
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
        //NOTE(Adriano):Store it flat, in an array which is made of blocks
        //of size equal to the number of points in the dataset
        //where each block contains a float distance for each centroid.
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
        //Assign it
        Centroids[ThreadIndexX * Stride + ThreadIndexY] = Points[ThreadIndexX * Stride + ThreadIndexY];
    }
    //Centroids[] => Points[]
}
/*
    Check if we have converged by comparing the old centroid array with the
    updated one.
    Returns 1 if the algorithm has converged (Centroid's position are not changing anymore) or
    0 if not.
    The way it does it's by subtracting the two arrays elements and adding
    a 1 if the difference is greater than the defined threshold or 0 if not.
    When the kernel has finished, if sum is equal to (NumCentroids*Stride) then
    we know that all centroid's position have not changed much.
*/
int CudaCompareCentroidsList(int *DeviceSum,float *DeviceCentroidList,float *DeviceOldCentroidList,int NumCentroids,int Stride)
{
    dim3   BlockSize;
    dim3   GridSize;
    int    Sum;

    BlockSize = dim3(256,Stride,1);
    GridSize = dim3((NumCentroids + BlockSize.x - 1) / BlockSize.x,1,1);
    
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceSum,0,sizeof(int)));
    CompareCentroidsKernel<<<GridSize,BlockSize>>>(DeviceSum,DeviceCentroidList,DeviceOldCentroidList,NumCentroids,Stride);

//     CUDA_CHECK_RETURN( cudaPeekAtLastError() );
//     CUDA_CHECK_RETURN( cudaDeviceSynchronize() );
    
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

//     CUDA_CHECK_RETURN( cudaPeekAtLastError() );
//     CUDA_CHECK_RETURN( cudaDeviceSynchronize() );
//     return DeviceCentroidList;
}
void CudaBuildClusterList(int *DeviceClusterList,float *DeviceDistanceList,int NumPoints,int NumCentroids,int Stride)
{
    dim3   BlockSize;
    dim3   GridSize;

    BlockSize = dim3(256,1,1);
    GridSize = dim3((NumPoints + BlockSize.x - 1) / BlockSize.x,1,1);
    BuildClusterListKernel<<<GridSize,BlockSize>>>(DeviceClusterList,DeviceDistanceList,NumPoints,NumCentroids,Stride);

//     CUDA_CHECK_RETURN( cudaPeekAtLastError() );
//     CUDA_CHECK_RETURN( cudaDeviceSynchronize() );
//     return DeviceClusterList;
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

//     CUDA_CHECK_RETURN( cudaPeekAtLastError() );
//     CUDA_CHECK_RETURN( cudaDeviceSynchronize() );
}

void CudaMeanPointsInClusters(float *DeviceCentroidList,int NumCentroids,float *DeviceClusterCounter,int Stride)
{
    dim3   BlockSize;
    dim3   GridSize;

    BlockSize = dim3(256,Stride,1);
    GridSize = dim3((NumCentroids + BlockSize.x - 1) / BlockSize.x,1,1);

    MeanPointsInClustersKernel<<<GridSize,BlockSize>>>(DeviceCentroidList,NumCentroids,DeviceClusterCounter,Stride);

//     CUDA_CHECK_RETURN( cudaPeekAtLastError() );
//     CUDA_CHECK_RETURN( cudaDeviceSynchronize() );
}


void CudaUpdateCentroidList(float *DeviceCentroidList,int NumCentroids,float *DevicePoints,int NumPoints,int *DeviceClusterList,
                            float *DeviceClusterCounter,int Stride)
{
    
    //Make sure we zero out the centroid's position...
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceCentroidList,0,NumCentroids * Stride * sizeof(float)));
    CUDA_CHECK_RETURN(cudaMemset((void *)DeviceClusterCounter,0,NumCentroids * sizeof(float)));

    CudaSumPointsInClusters(DeviceCentroidList,NumCentroids,DeviceClusterList,DeviceClusterCounter,DevicePoints,
                            NumPoints,Stride);
    CudaMeanPointsInClusters(DeviceCentroidList,NumCentroids,DeviceClusterCounter,Stride);
    //CudaGet
}
float *CudaInitCentroids(int NumCentroids,float *DevicePointList,int NumPoints,int Stride)
{
    float *DeviceCentroidOutputList;
    int    CentroidListSize;
    dim3   BlockSize;
    dim3   GridSize;

    //Step.1 Initialize all the centroids.
    CentroidListSize = NumCentroids * Stride * sizeof(float);


    CUDA_CHECK_RETURN(cudaMalloc((void**)&DeviceCentroidOutputList,CentroidListSize));

    BlockSize = dim3(64, Stride, 1);
    GridSize = dim3((NumCentroids + BlockSize.x - 1) / BlockSize.x,1,1);
    CentroidsInitKernel<<<GridSize,BlockSize>>>(DeviceCentroidOutputList,NumCentroids,DevicePointList,NumPoints,Stride);

//     CUDA_CHECK_RETURN( cudaPeekAtLastError() );
//     CUDA_CHECK_RETURN( cudaDeviceSynchronize() );
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
    CUDA_CHECK_RETURN(cudaMemcpy(DevicePointList,PointList->Points, PointListSize,
                                 cudaMemcpyHostToDevice));
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
    //DEBUG
    float *DebugOutputList;
    int    DebugOutputListSize;
    int   *DebugOutputList2;
    int    DebugOutputListSize2;
    float  *DebugOutputList3;
    int    DebugOutputListSize3;
    float  *DebugOutputList4;
    int    DebugOutputListSize4;
    int Step;
    int Start;
    int End;
//     cudaProfilerStart();
    DebugOutputListSize = PointList->NumPoints * NumCentroids * sizeof(float);
    DebugOutputList = (float *) malloc(DebugOutputListSize);
    
    DebugOutputListSize2 = PointList->NumPoints * sizeof(int);
    DebugOutputList2 = (int *) malloc(DebugOutputListSize2);
    
    DebugOutputListSize3 = PointList->NumPoints * Stride * sizeof(float);
    DebugOutputList3 = (float *) malloc(DebugOutputListSize3);
    
    DebugOutputListSize4 = NumCentroids * Stride * sizeof(float);
    DebugOutputList4 = (float *) malloc(DebugOutputListSize4);

    
    assert(PointList->Stride == Stride);
    
    Start = SysMilliseconds();
    
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

    #if 0/* _DEBUG*/
        //Test to check if the Distance and BuildClusterList Kernel are working as intended.
        cudaMemcpy(DebugOutputList, DeviceDistanceList, DebugOutputListSize,cudaMemcpyDeviceToHost);
        cudaMemcpy(DebugOutputList2, DeviceClusterList, DebugOutputListSize2,cudaMemcpyDeviceToHost);
        int PointNumber = 0;
        float Min;
        int SelectedCentroid;
        for( int i = 0; i < PointList->NumPoints * NumCentroids; i+=NumCentroids ) {
            DPrintf("Point %i Distances: \n",PointNumber);
            Min = INFINITY;
            SelectedCentroid = -1;
            for( int j = 0; j < NumCentroids; j++ ) {
                DPrintf("Centroid (i == %i) %i:%f\n",i,j,DebugOutputList[i+j]);
                if( DebugOutputList[i+j] < Min ) {
                    Min = DebugOutputList[i+j];
                    SelectedCentroid = j;
                }
            }
    //         DPrintf("Point %i has chosen centroid %i with a min distance of %f\n",PointNumber,SelectedCentroid,Min);
    //         DPrintf("In Cuda List was %i\n",DebugOutputList2[i]);
    //         if( SelectedCentroid != DebugOutputList2[PointNumber] ) {
    //             DPrintf("Mismatched Cuda/Linear Centroid selection...Expected %i found %i\n",
    //                 SelectedCentroid,DebugOutputList2[PointNumber]);
    //         }
            assert( SelectedCentroid == DebugOutputList2[PointNumber] );
            PointNumber++;
        }
    #endif
        cudaMemcpy(DeviceOldCentroidList,DeviceCentroidList,OldCentroidListSize,cudaMemcpyDeviceToDevice);
        CudaUpdateCentroidList(DeviceCentroidList,NumCentroids,DevicePointList,PointList->NumPoints,DeviceClusterList,
                            DeviceClusterCounter,Stride);
        Sum = CudaCompareCentroidsList(DeviceSum,DeviceCentroidList,DeviceOldCentroidList,NumCentroids,Stride);
//         cudaDeviceSynchronize();

        //CentroidList
//         DPrintf("Sum is %i || Iteration:%i\n",Sum,Step);
        if( Sum == 1 ) {
            DPrintf("Reached max condition...\n");
            break;
        }
        Step++;
    }
    End = SysMilliseconds();
    printf("Cuda Completed in %i steps %i ms elapsed.\n",Step,End-Start);
    cudaMemcpy(DebugOutputList4, DeviceCentroidList, DebugOutputListSize4,cudaMemcpyDeviceToHost);
        //Cluster Index List
    cudaMemcpy(DebugOutputList2, DeviceClusterList, DebugOutputListSize2,cudaMemcpyDeviceToHost);
        //Dataset
    cudaMemcpy(DebugOutputList3, DevicePointList, DebugOutputListSize3,cudaMemcpyDeviceToHost);
//         cudaDeviceSynchronize();
    DumpClusters(DebugOutputList3,PointList->NumPoints,DebugOutputList4,NumCentroids,DebugOutputList2,Stride,Step);

    cudaFree(DevicePointList);
    cudaFree(DeviceCentroidList);
    cudaFree(DeviceOldCentroidList);
    cudaFree(DeviceDistanceList);
    cudaFree(DeviceClusterList);
    cudaFree(DeviceClusterCounter);
    cudaFree(DeviceSum);
    free(DebugOutputList);
    free(DebugOutputList2);
    free(DebugOutputList3);
    free(DebugOutputList4);
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
    
    CudaMain(NumClusters,Stride,PointList);
    
    PointArrayListCleanUp(PointList);
    
    free(PointList);
}
