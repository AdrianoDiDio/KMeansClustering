void KMeansClustering(PointArrayList_t *Dataset,int NumCentroids,int Stride)
{

    Centroids = (float *) malloc(NumCentroids * Stride * sizeof(float));
    DistancesSize = Dataset->NumPoints * NumCentroids * sizeof(float);
    Distances = (float *) malloc(DistancesSize);
    
    ClusterCounterSize = NumCentroids * sizeof(float);
    ClusterCounter = (float *) malloc(ClusterCounterSize);
    
    ClusterSize = Dataset->NumPoints * sizeof(int);
    Clusters = (int *) malloc(ClusterSize);
    
    ClusterMeansSize = NumCentroids * Stride *sizeof(float);
    ClusterMeans = (float *) malloc(ClusterMeansSize);
    
    MaxThreadNumber = omp_get_max_threads();

    #pragma omp parallel for private(j) num_threads(MaxThreadNumber) schedule(static, (NumCentroids*Stride)/MaxThreadNumber)
    for( i = 0; i < NumCentroids; i++ ) {
        for( j = 0; j < Stride; j++ ) {
            Centroids[i * Stride + j] = Dataset->Points[i * Stride +j];
        }
    }
    
    while( 1 ) {
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

        #pragma omp parallel for schedule(guided) \
            shared(Distances,Clusters,ClusterCounter) private(j)
        for( i = 0; i < Dataset->NumPoints; i++ ) {
            float Min = INFINITY;
            int Index = -1;
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
    }
}
