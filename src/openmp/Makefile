
CC= gcc
CFLAGS=-Wall -fopenmp

Obj = PointArrayList.o KMeansClustering.o
LDFLAGS =

all: release

debug: CFLAGS +=-D_DEBUG -g
debug: KMeansClustering

release: CFLAGS += -O3
release: KMeansClustering

KMeansClustering: $(Obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(Obj) KMeansClustering
