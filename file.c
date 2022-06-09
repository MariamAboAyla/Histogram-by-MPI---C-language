#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <math.h>
#include <omp.h>
#include <mpi.h>


int main(int argc, char *argv[]) {
    
	int* ranges;
	int numOfBars, numOfProcesses, numOfPoints;
	//int numOfThreads;
	int myId;
	int maxValue=0, num=0, cnt=0;
	int step; // to check bar step
	int* data;
	int recievedData[1000];
	int *finalCount;
	int i=0, j=0, prev=0;
	int sum=0, rem;
	char line[1000];
	
	FILE* file;
	
	MPI_Init(& argc ,& argv );

	MPI_Comm_size(MPI_COMM_WORLD , &numOfProcesses);
	MPI_Comm_rank( MPI_COMM_WORLD ,&myId);

		
	
	if(myId == 0)
	{
		// master
		printf("Enter number of Bars :");
		scanf("%d" , &numOfBars);
		printf("Enter number of points :");
		scanf("%d" , &numOfPoints);
		//printf("Enter number of threads: ");
		//scanf("%d", &numOfThreads);
			
		
		ranges = malloc(sizeof(int)*numOfBars);
		data = malloc(sizeof(int)*numOfPoints);
	
		file = fopen("/shared/dataset.txt", "r");
		
		maxValue = -1;
		while (fgets(line, 1000, file)) {
			sscanf(line, "%d", &data[j]);
			//printf("%d\n", data[j]);
			if(data[j]>maxValue)
			{
				maxValue = data[j];
			}
			j = j +1;
		}
		fclose(file);
		
	
		step = maxValue/numOfBars;
		ranges[0] = step;
		for(i=1; i<numOfBars; i++)
		{
			ranges[i] = step*(i+1); //6*2=12
		}
		
		rem = numOfPoints % numOfProcesses;
		
	}
		
		
		
	// broadcast
	MPI_Bcast(&numOfBars, 1, MPI_INT, 0, MPI_COMM_WORLD);
	//MPI_Bcast(&numOfThreads, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&rem, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&numOfPoints, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(ranges, numOfBars, MPI_INT, 0, MPI_COMM_WORLD);
	
	
	int* localPoints = malloc(sizeof(int)*numOfProcesses);
	int* displacements = malloc(sizeof(int)*numOfProcesses);
	int* countRange = malloc(sizeof(int)*numOfBars);

	
	for (i = 0; i < numOfProcesses; i++) {
		localPoints[i] = numOfPoints / numOfProcesses;
		if (rem > 0) {
			localPoints[i]++;
			rem--;
		}

		displacements[i] = sum;
		sum += localPoints[i];
		//printf("%d\n", localPoints[i]);
	}
	

	MPI_Scatterv(data, localPoints, displacements, MPI_INT, recievedData, 1000, MPI_INT, 0, MPI_COMM_WORLD);


	
	#pragma omp parallel private(i,j)
	{

		#pragma omp for schedule(static)
		for(i=0; i<numOfBars; i++)
		{
			countRange[i]=0;	
		}
		
		#pragma omp for schedule(static)
		for(i=0; i<localPoints[myId]; i++)
		{
			prev = 1;
			for(j=0; j<numOfBars; j++)
			{
				
				if( recievedData[i] >= prev && recievedData[i]<=ranges[j] )
				{
					// 6 12 18 24
					#pragma omp critical
					countRange[j]++;
					break;
				}
				prev = ranges[j]+1;
				
				
			}
			
			/*
			//printf("id %d\n", myId);
			for(j=0; j<numOfBars; j++){
				
				printf(" %d", countRange[j]);
			}
			printf("\n");
			*/
		
		}
		
	}
	
	if(myId==0)
	{
		finalCount = malloc(sizeof(int)*numOfBars * numOfProcesses);
	}
	
	MPI_Gather(countRange, numOfBars, MPI_INT, finalCount, numOfBars, MPI_INT, 0, MPI_COMM_WORLD);

	if (myId == 0) {
		#pragma omp parallel shared(finalCount, numOfBars, numOfProcesses) private(i)
		{
			#pragma omp for schedule(static)
			for (i = numOfBars; i < numOfProcesses * numOfBars; ++i) {
				finalCount[i % numOfBars] += finalCount[i];
			}

        }


        int st = 0, end = step;
        for (i = 0; i < numOfBars; ++i) {
            printf("The range start with %d, end with %d, with count %d\n", st, end, finalCount[i]);
            st += step;
            end += step;
        }

    }	

	
	MPI_Finalize();
    return 0;
}
