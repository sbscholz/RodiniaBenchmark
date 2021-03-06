
/*************************************************************************/
/**   File:         kmeans_clustering.c                                 **/
/**   Description:  Implementation of regular k-means clustering        **/
/**                 algorithm                                           **/
/**   Author:  Wei-keng Liao                                            **/
/**            ECE Department, Northwestern University                  **/
/**            email: wkliao@ece.northwestern.edu                       **/
/**                                                                     **/
/**   Edited by: Jay Pisharath                                          **/
/**              Northwestern University.                               **/
/**                                                                     **/
/**   ================================================================  **/
/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "kmeans.h"
#include <omp.h>
#include <sys/time.h>

#define RANDOM_MAX 2147483647

#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38
#endif

extern double wtime(void);
extern int num_omp_threads;

int find_nearest_point(float  *pt,          /* [nfeatures] */
                       int     nfeatures,
                       float **pts,         /* [npts][nfeatures] */
                       int     npts)
{
  int index, i;
  float max_dist=FLT_MAX;

  /* find the cluster center id with min distance to pt */
  for (i=0; i<npts; i++) {
    float dist;
    dist = euclid_dist_2(pt, pts[i], nfeatures);  /* no need square root */
    if (dist < max_dist) {
      max_dist = dist;
      index    = i;
    }
  }
  return(index);
}

/*----< euclid_dist_2() >----------------------------------------------------*/
/* multi-dimensional spatial Euclid distance square */
__inline
float euclid_dist_2(float *pt1,
                    float *pt2,
                    int    numdims)
{
  int i;
  float ans=0.0;

  for (i=0; i<numdims; i++) {
    ans += (pt1[i]-pt2[i]) * (pt1[i]-pt2[i]);
  }

  return(ans);
}


/*----< kmeans_clustering() >---------------------------------------------*/
float** kmeans_clustering(float **feature,    /* in: [npoints][nfeatures] */
                          int     nfeatures,
                          int     npoints,
                          int     nclusters,
                          float   threshold,
                          int    *membership) /* out: [npoints] */
{

  int      i, j, k, n=0, index, loop=0;
  int     *new_centers_len;			/* [nclusters]: no. of points in each cluster */
  float  **new_centers;				/* [nclusters][nfeatures] */
  float  **clusters;				/* out: [nclusters][nfeatures] */
  float    delta;
      
  double   timing;

  int      nthreads;
  int    **partial_new_centers_len;
  float ***partial_new_centers;

  nthreads = num_omp_threads; 

  /* allocate space for returning variable clusters[] */
  clusters    = (float**) malloc(nclusters *             sizeof(float*));
  clusters[0] = (float*)  malloc(nclusters * nfeatures * sizeof(float));
  for (i=1; i<nclusters; i++) {
    clusters[i] = clusters[i-1] + nfeatures;
  }
  
  /* randomly pick cluster centers */
  for (i=0; i<nclusters; i++) {
    //n = (int)rand() % npoints;
    for (j=0; j<nfeatures; j++) {
      clusters[i][j] = feature[n][j];
    }
    n++;
  }
 
  /* Initially, all membership is -1 */
  for (i=0; i<npoints; i++) {
    membership[i] = -1;
  }

  /* need to initialize new_centers_len and new_centers[0] to all 0 */
  new_centers_len = (int*) calloc(nclusters, sizeof(int));  /* Stores the number of points in each cluster */

  new_centers    = (float**) malloc(nclusters *            sizeof(float*));
  new_centers[0] = (float*)  calloc(nclusters * nfeatures, sizeof(float));
  for (i=1; i<nclusters; i++) {
    new_centers[i] = new_centers[i-1] + nfeatures;
  }

  partial_new_centers_len    = (int**) malloc(nthreads * sizeof(int*));
  partial_new_centers_len[0] = (int*)  calloc(nthreads*nclusters, sizeof(int));
  for (i=1; i<nthreads; i++) {
    partial_new_centers_len[i] = partial_new_centers_len[i-1]+nclusters;
  }

  partial_new_centers    =(float***)malloc(nthreads * sizeof(float**));
  partial_new_centers[0] =(float**) malloc(nthreads*nclusters * sizeof(float*));
  for (i=1; i<nthreads; i++) {
    partial_new_centers[i] = partial_new_centers[i-1] + nclusters;
  }

  for (i=0; i<nthreads; i++) {
    for (j=0; j<nclusters; j++) {
      partial_new_centers[i][j] = (float*)calloc(nfeatures, sizeof(float));
    }
  }
  printf("num of threads = %d\n", num_omp_threads);

   
  struct timeval tv1, tv2;
  gettimeofday( &tv1, NULL);
  int c = 0; 
  do {
    delta = 0.0;
    omp_set_num_threads(num_omp_threads);
    #pragma omp parallel shared(feature,clusters,membership,partial_new_centers,partial_new_centers_len)
    {
      int tid = omp_get_thread_num();				
      #pragma omp for \
		  private(i,j,index) \
		  firstprivate(npoints,nclusters,nfeatures) \
		  schedule(static) \
		  reduction(+:delta)
      for (i=0; i<npoints; i++) {
	/* find the index of nestest cluster centers */	
        /* index can be 0, 1, 2, 3, or 4 */				
	index = find_nearest_point( feature[i],
		                    nfeatures,
		                    clusters,
		                    nclusters);				

	/* if membership changes, increase delta by 1 */
	if (membership[i] != index) {
          delta += 1.0;
        }

	/* assign the membership (i.e. belongs to which cluster) to object i */
	membership[i] = index;
			
	/* update new cluster centers : sum of all objects located within */
	partial_new_centers_len[tid][index]++;				
	for (j=0; j<nfeatures; j++) {
          partial_new_centers[tid][index][j] += feature[i][j];
        }
      }
    } /* end of #pragma omp parallel */

    /* let the main thread perform the array reduction */
    for (i=0; i<nclusters; i++) {
      for (j=0; j<nthreads; j++) {
	new_centers_len[i] += partial_new_centers_len[j][i];
	partial_new_centers_len[j][i] = 0.0;
	for (k=0; k<nfeatures; k++) {
	  new_centers[i][k] += partial_new_centers[j][i][k];
	  partial_new_centers[j][i][k] = 0.0;
	}
      }
    }    

    /* replace old cluster centers with new_centers */
    for (i=0; i<nclusters; i++) {
      for (j=0; j<nfeatures; j++) {
	if (new_centers_len[i] > 0) {
          clusters[i][j] = new_centers[i][j] / new_centers_len[i];
        }
        new_centers[i][j] = 0.0;   /* set back to 0 */
      }
      new_centers_len[i] = 0;   /* set back to 0 */
    }
    c++;
  } while (delta > threshold && loop++ < 500);
 

  gettimeofday( &tv2, NULL);
  double runtime = ((tv2.tv_sec+ tv2.tv_usec/1000000.0)-(tv1.tv_sec+ tv1.tv_usec/1000000.0));
  printf("Runtime(seconds): %f\n", runtime);
 
  printf("iterated %d times\n", c);

  free(new_centers[0]);
  free(new_centers);
  free(new_centers_len);

  return clusters;
}

