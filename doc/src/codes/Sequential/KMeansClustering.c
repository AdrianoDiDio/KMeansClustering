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
        Sum += (PointB[i] - PointA[i]) * 
            (PointB[i] - PointA[i]);
    }
    return Sum;
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
    
    printf("Selected KMeans Algorithm with a dataset of size %i and %i centroids.\n",Dataset->NumPoints,NumCentroids);
    
    // 1) Selects K (NumCentroids) random centroids from the dataset.
    srand(time(0)); 
    Centroids = malloc(sizeof(Centroid_t) * NumCentroids );
    for( i = 0; i < NumCentroids; i++ ) {
        Centroids[i].Position = malloc(Stride * sizeof(float));
        for( j = 0; j < Stride; j++ ) {
            Centroids[i].Position[j] = Dataset->Points[i].Position[j];
        }
        Centroids[i].Stride = Stride;
    }
    //Test with 5 steps...
    HasToStop = 0;
    NumStep = 0;
    Sum = malloc( Stride * sizeof(float));
    Delta = malloc( Stride * sizeof(float));
    while( !HasToStop ) {
        // 2) Assign each points of the dataset to the nearest centroid.
        for(i = 0; i < Dataset->NumPoints; i++) {
            Min = 99999;
            for(j = 0; j < NumCentroids; j++) {
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
        for( int i = 0; i < NumCentroids; i++ ) {
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
            for( k = 0; k < Stride; k++ ) {
                if( fabsf(Delta[k]) > KMEANS_ALGORITHM_TOLERANCE) {
                    break;
                }
            }
            if( k == Stride ) {
                NumClustersSet++;
            }
            memcpy(Centroids[i].Position,Sum,Stride * sizeof(float));
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
