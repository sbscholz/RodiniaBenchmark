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

int find_nearest_point( float  *pt,          /* [nfeatures] */
                        int     nfeatures,
                        float **pts,         /* [nclusters][nfeatures] */
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

  int      i, j, n=0, index, loop=0;
  int     *new_centers_len; /* [nclusters]: no. of points in each cluster */
  float    delta;
  float  **clusters;        /* out: [nclusters][nfeatures] */
  float  **new_centers;     /* [nclusters][nfeatures] */

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

  for (i=0; i<npoints; i++) {
    membership[i] = -1;
  }

  /* need to initialize new_centers_len and new_centers[0] to all 0 */
  new_centers_len = (int*) calloc(nclusters, sizeof(int));

  new_centers    = (float**) malloc(nclusters *            sizeof(float*));
  new_centers[0] = (float*)  calloc(nclusters * nfeatures, sizeof(float));

  for (i=1; i<nclusters; i++) {
    new_centers[i] = new_centers[i-1] + nfeatures;
  }

  int* membership_new;
  membership_new = (int*)malloc(sizeof(int)*npoints);
  int ii;
  for( ii = 0; ii < npoints; ii++) { 
    membership_new[ii] = -1;
  }
  int cluster_id;


  int c = 0;
  struct timeval tv1, tv2;
  gettimeofday( &tv1, NULL);
  do {
    delta = 0.0;

    for (i=0; i<npoints; i++) {
      /* find the index of nestest cluster center */
      index = find_nearest_point(feature[i], nfeatures, clusters, nclusters);
      /* if membership changes, increase delta by 1 */
      if (membership[i] != index) delta += 1.0;

      /* assign the membership to object i */
      membership[i] = index;

      /* update new cluster centers : sum of objects located within */
      new_centers_len[index]++;
      for (j=0; j<nfeatures; j++) {         
	new_centers[index][j] += feature[i][j];
      }
    }
/*
    for (i = 0; i < npoints; i++) {		
      cluster_id = membership_new[i];
      new_centers_len[cluster_id] = new_centers_len[cluster_id]+1;
      if( membership_new[i] != membership[i]) {
        delta += 1.0;
        membership[i] = membership_new[i];
      }

      for (j = 0; j < nfeatures; j++) {			
        new_centers[cluster_id][j] = new_centers[cluster_id][j] + feature[i][j];
      }
    } 
*/

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
    //delta /= npoints;
    c++;
  } while (delta > threshold);
 
  printf("iterated %d times\n", c);

  gettimeofday( &tv2, NULL);
  double runtime = ((tv2.tv_sec+ tv2.tv_usec/1000000.0)-(tv1.tv_sec+ tv1.tv_usec/1000000.0));
  printf("Runtime(seconds): %f\n", runtime);
 
  free(new_centers[0]);
  free(new_centers);
  free(new_centers_len);

  return clusters;
}

