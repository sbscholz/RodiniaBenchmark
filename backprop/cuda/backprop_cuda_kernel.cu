

#ifndef _BACKPROP_CUDA_KERNEL_H_
#define _BACKPROP_CUDA_KERNEL_H_

#include <stdio.h>
#include "backprop.h"
#include "math.h"
#include "cuda.h"

__global__ void
my_bpnn_layerforward_CUDA( float *input_cuda,
	                   float *input_hidden_cuda,
                           float *hidden_partial_sum,
                           int in,
                           int hid) 
{
  int by = blockIdx.y;
  int tx = threadIdx.x;
  int ty = threadIdx.y;

  int index =  ( hid + 1 ) * HEIGHT * by + ( hid + 1 ) * ty + tx + 1 + ( hid + 1 ) ;  

  /* This has been changed to eabled memory coalescing */
  int index_in = HEIGHT * by + tx + 1;
   
  __shared__ float input_node[HEIGHT];
  __shared__ float weight_matrix[HEIGHT][WIDTH];

  /* This has been changed to eabled memory coalescing */
  if ( ty == 0 ) {
    input_node[tx] = input_cuda[index_in] ;
  }   

  __syncthreads();

  weight_matrix[ty][tx] = input_hidden_cuda[index]; // load weights

  __syncthreads();
   
  weight_matrix[ty][tx] = weight_matrix[ty][tx] * input_node[ty];

  __syncthreads();   
  
/*
  for ( int i = 1 ; i <= __log2f(HEIGHT) ; i++) {
     int power_two = __powf(2, i);

     if( ty % power_two == 0 ) {
       weight_matrix[ty][tx] = weight_matrix[ty][tx] + weight_matrix[ty + power_two/2][tx];
     }

     __syncthreads();
  }
*/

  if( ty == 0) {
    for( int i = 1; i < HEIGHT; i++) {
      weight_matrix[ty][tx] = weight_matrix[ty][tx] + weight_matrix[i][tx];
    } 
  }
   
  /* Do we need to following assignment???? */
  //input_hidden_cuda[index] = weight_matrix[ty][tx];
   
  __syncthreads();

  /* This has been changed to eabled memory coalescing */
  if ( ty == 0 ) {
    hidden_partial_sum[by * hid + tx] = weight_matrix[ty][tx];
  }
}

__global__ void
bpnn_layerforward_CUDA( float *input_cuda,
			float *input_hidden_cuda, // this is actuall the weights
			float *hidden_partial_sum,
			int in,
			int hid) 
{
  int by = blockIdx.y;
  int tx = threadIdx.x;
  int ty = threadIdx.y;

  int index =  ( hid + 1 ) * HEIGHT * by + ( hid + 1 ) * ty + tx + 1 + ( hid + 1 ) ;  

  int index_in = HEIGHT * by + ty + 1;
   
  __shared__ float input_node[HEIGHT];
  __shared__ float weight_matrix[HEIGHT][WIDTH];

  if ( tx == 0 ) {
    input_node[ty] = input_cuda[index_in] ;
  }   

  __syncthreads();

  weight_matrix[ty][tx] = input_hidden_cuda[index]; // load weights

  __syncthreads();
   
  weight_matrix[ty][tx] = weight_matrix[ty][tx] * input_node[ty];

  __syncthreads();   
  

  for ( int i = 1 ; i <= __log2f(HEIGHT) ; i++) {
     int power_two = __powf(2, i);

     if( ty % power_two == 0 ) {
       weight_matrix[ty][tx] = weight_matrix[ty][tx] + weight_matrix[ty + power_two/2][tx];
     }

     __syncthreads();
  }
   
  /* Do we need to following assignment???? */
  //input_hidden_cuda[index] = weight_matrix[ty][tx];
   
/*
   for ( unsigned int i = 2 ; i <= HEIGHT ; i *= 2){
 
	   unsigned int power_two = i - 1;

	   if( (ty & power_two) == 0 ) {
		weight_matrix[ty][tx] = weight_matrix[ty][tx] + weight_matrix[ty + power_two/2][tx];
	   }

   }
*/

  __syncthreads();

  if ( tx == 0 ) {
    hidden_partial_sum[by * hid + ty] = weight_matrix[tx][ty];
  }
}

__global__ void 
my_bpnn_adjust_weights_cuda( float * delta,   
                             int hid,         
                             float * ly,      
                             int in,          
                             float * w,       
                             float * oldw)  									
{
  
  
  int by = blockIdx.y;

  int tx = threadIdx.x;
  int ty = threadIdx.y;
	
  int index =  ( hid + 1 ) * HEIGHT * by + ( hid + 1 ) * ty + tx + 1 + ( hid + 1 ) ;  
  int index_x = tx + 1;

  __shared__ float tmp[WIDTH];
  int shr_idx = HEIGHT * by + tx + 1;
  if ( ty == 0 ) {
    tmp[tx] = ly[shr_idx];
  } 
  
  __syncthreads();

  w[index] +=   ((ETA * delta[index_x] * tmp[ty]) + (MOMENTUM * oldw[index]));
  oldw[index] = ((ETA * delta[index_x] * tmp[ty]) + (MOMENTUM * oldw[index]));

  //__syncthreads();

  if (ty == 0 && by ==0) {
    w[index_x] +=   ((ETA * delta[index_x]) + (MOMENTUM * oldw[index_x]));
    oldw[index_x] = ((ETA * delta[index_x]) + (MOMENTUM * oldw[index_x]));
  }
}

__global__ void 
bpnn_adjust_weights_cuda( float * delta,   
			  int hid,         
			  float * ly,      
			  int in,          
			  float * w,       
			  float * oldw)  									
{
  
  
  int by = blockIdx.y;

  int tx = threadIdx.x;
  int ty = threadIdx.y;
	
  int index =  ( hid + 1 ) * HEIGHT * by + ( hid + 1 ) * ty + tx + 1 + ( hid + 1 ) ;  
  int index_y = HEIGHT * by + ty + 1;
  int index_x = tx + 1;

  w[index] +=   ((ETA * delta[index_x] * ly[index_y]) + (MOMENTUM * oldw[index]));
  oldw[index] = ((ETA * delta[index_x] * ly[index_y]) + (MOMENTUM * oldw[index]));

  __syncthreads();

  if (ty == 0 && by ==0) {
    w[index_x] +=   ((ETA * delta[index_x]) + (MOMENTUM * oldw[index_x]));
    oldw[index_x] = ((ETA * delta[index_x]) + (MOMENTUM * oldw[index_x]));
  }
}
#endif 
