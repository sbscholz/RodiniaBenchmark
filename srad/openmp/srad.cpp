// srad.cpp : Defines the entry point for the console application.
//

//#define OUTPUT


//#define OPEN
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <omp.h>

#ifndef SIZE
#define SIZE 2048
#endif

#define R1     0
#define R2     127
#define C1     0
#define C2     127

#define ROWS     SIZE
#define COLS     SIZE
#define LAMBDA   0.5f
#define NITER    1000

static void random_matrix(float *I, int rows, int cols)
{
  srand(7);
  
  for( int i = 0 ; i < rows ; i++) {
    for ( int j = 0 ; j < cols ; j++) {
      I[i * cols + j] = (float)rand()/(float)RAND_MAX ;
    }
  }
}

void usage(int argc, char **argv)
{
  fprintf(stderr, "Usage: %s <rows> <cols> <y1> <y2> <x1> <x2> <no. of threads><lamda> <no. of iter>\n", argv[0]);
  fprintf(stderr, "\t<rows>   - number of rows\n");
  fprintf(stderr, "\t<cols>    - number of cols\n");
  fprintf(stderr, "\t<y1> 	 - y1 value of the speckle\n");
  fprintf(stderr, "\t<y2>      - y2 value of the speckle\n");
  fprintf(stderr, "\t<x1>       - x1 value of the speckle\n");
  fprintf(stderr, "\t<x2>       - x2 value of the speckle\n");
  fprintf(stderr, "\t<no. of threads>  - no. of threads\n");
  fprintf(stderr, "\t<lamda>   - lambda (0,1)\n");
  fprintf(stderr, "\t<no. of iter>   - number of iterations\n");
	
  exit(1);
}

int main(int argc, char* argv[])
{   
  int rows, cols, size_I, size_R, niter = 10, iter, k;
  float *I, *J, q0sqr, sum, sum2, tmp, meanROI,varROI ;
  float Jc, G2, L, num, den, qsqr;
  int *iN,*iS,*jE,*jW;
  float *dN,*dS,*dW,*dE;
  int r1, r2, c1, c2;
  float cN,cS,cW,cE;
  float *c, D;
  float lambda;
  int i, j;
  int nthreads;

/*
  if (argc == 10) {
    rows = atoi(argv[1]); //number of rows in the domain
    cols = atoi(argv[2]); //number of cols in the domain
    if ((rows%16!=0) || (cols%16!=0)){
      fprintf(stderr, "rows and cols must be multiples of 16\n");
      exit(1);
    }
    r1   = atoi(argv[3]); //y1 position of the speckle
    r2   = atoi(argv[4]); //y2 position of the speckle
    c1   = atoi(argv[5]); //x1 position of the speckle
    c2   = atoi(argv[6]); //x2 position of the speckle
    nthreads = atoi(argv[7]); // number of threads
    lambda = atof(argv[8]); //Lambda value
    niter = atoi(argv[9]); //number of iterations
  }
  else{
    usage(argc, argv);
  }
*/
  nthreads = 4;
  
  rows = ROWS;
  cols = COLS;
  if((rows%16!=0) || (cols%16!=0)){
    fprintf(stderr, "rows and cols must be multiples of 16\n");
    exit(1);
  }
  r1 = R1;
  r2 = R2;
  c1 = C1;
  c2 = C2;	
  lambda = LAMBDA; 
  niter = NITER;

  size_I = cols * rows;
  size_R = (r2-r1+1)*(c2-c1+1);  

  I = (float *)malloc( size_I * sizeof(float) );
  J = (float *)malloc( size_I * sizeof(float) );
  c = (float *)malloc( size_I * sizeof(float) );

  /* The following four 1D arrays are used to hold indices */
  iN = (int *)malloc(sizeof(unsigned int*) * rows);
  iS = (int *)malloc(sizeof(unsigned int*) * rows);
  jW = (int *)malloc(sizeof(unsigned int*) * cols);
  jE = (int *)malloc(sizeof(unsigned int*) * cols);    

  /* The following four 2D arrays are used to hold the 
   * difference between the N/S/W/E element and the center
   * element, all taken from array J */
  dN = (float *)malloc(sizeof(float)* size_I);
  dS = (float *)malloc(sizeof(float)* size_I);
  dW = (float *)malloc(sizeof(float)* size_I);
  dE = (float *)malloc(sizeof(float)* size_I);    

  for (int i=0; i< rows; i++) {
    iN[i] = i-1;
    iS[i] = i+1;
  }    

  iN[0] = 0;
  iS[rows-1] = rows-1;

  for (int j=0; j< cols; j++) {
    jW[j] = j-1;
    jE[j] = j+1;
  }

  jW[0] = 0;
  jE[cols-1] = cols-1;
	
  printf("Randomizing the input matrix\n");

  random_matrix(I, rows, cols);

  for (k = 0;  k < size_I; k++ ) {
    J[k] = (float)exp(I[k]) ;
  }
  
  printf("Start the SRAD main loop\n");

  for (iter=0; iter < niter; iter++) {
    sum=0; 
    sum2=0;     
    
    for (i = r1; i <= r2; i++) {
      for (j = c1; j <= c2; j++) {
        tmp = J[i * cols + j];
        sum  += tmp ;
        sum2 += tmp*tmp;
      }
    }

    meanROI = sum / size_R;
    varROI  = (sum2 / size_R) - meanROI*meanROI;
    q0sqr   = varROI / (meanROI*meanROI);

#ifdef OPEN
    omp_set_num_threads(nthreads);
    #pragma omp parallel for shared(J, dN, dS, dW, dE, c, rows, cols, iN, iS, jW, jE) private(i, j, k, Jc, G2, L, num, den, qsqr)
#endif    
    for (int i = 0 ; i < rows ; i++) {
      for (int j = 0; j < cols; j++) { 
		
	k = i * cols + j;
	Jc = J[k];
 
        // directional derivates
	dN[k] = J[iN[i] * cols + j] - Jc;
	dS[k] = J[iS[i] * cols + j] - Jc;
	dW[k] = J[i * cols + jW[j]] - Jc;
	dE[k] = J[i * cols + jE[j]] - Jc;
			
	G2 = (dN[k]*dN[k] + dS[k]*dS[k] + dW[k]*dW[k] + dE[k]*dE[k]) / (Jc*Jc);

	L = (dN[k] + dS[k] + dW[k] + dE[k]) / Jc;

	num  = (0.5f*G2) - ((1.0f/16.0f)*(L*L)) ;
	den  = 1.0f + (0.25f*L);
	qsqr = num/(den*den);

	// diffusion coefficent (equ 33)
	den = (qsqr-q0sqr) / (q0sqr * (1+q0sqr)) ;
	c[k] = 1.0f / (1.0f + den) ;
	
	// saturate diffusion coefficent
	if (c[k] < 0) {
          c[k] = 0;
        }
	else if (c[k] > 1) {
          c[k] = 1;
        }
      }
    }

#ifdef OPEN
    omp_set_num_threads(nthreads);
    #pragma omp parallel for shared(J, c, rows, cols, lambda) private(i, j, k, D, cS, cN, cW, cE)
#endif 
    for (int i = 0; i < rows; i++) {
      for (int j = 0; j < cols; j++) {        
	// current index
	k = i * cols + j;
	
	// diffusion coefficent
	cN = c[k];
	cS = c[iS[i] * cols + j];
	cW = c[k];
	cE = c[i * cols + jE[j]];

	// divergence (equ 58)
	D = cN * dN[k] + cS * dS[k] + cW * dW[k] + cE * dE[k];
	
	// image update (equ 61)
	J[k] = J[k] + 0.25*lambda*D;
      }
    }
  }

#ifdef OUTPUT
  for( int i = 0 ; i < rows ; i++){
    for ( int j = 0 ; j < cols ; j++){
      printf("%.5f\n", J[i * cols + j]); 
    }
    printf("\n"); 
  }
#endif 

  printf("Computation Done\n");

  free(I);
  free(J);
  free(iN); free(iS); free(jW); free(jE);
  free(dN); free(dS); free(dW); free(dE);
  free(c);

  return( 0);
}



