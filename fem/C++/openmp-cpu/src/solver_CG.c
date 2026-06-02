/**
 ** CG
 **/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "precision.h"
#include "allocate.h"
extern FILE *fp_log;
/***
    CG solves the linear system Ax = b using the Conjugate Gradient 
    iterative method with the following preconditioners
 ***/
void  CG  (
	   KINT N,KINT NPLU,KREAL D[],
	   KREAL AMAT[],KINT indexLU[], KINT itemLU[],
	   KREAL B[],KREAL X[],KREAL RESID,KINT ITER, KINT *ERROR)
{
  int i,j,k;
  int ieL,isL,ieU,isU;
  double WVAL;
  double BNRM20,BNRM2,DNRM20,DNRM2;
  double S1_TIME,E1_TIME;
  double ALPHA,BETA;
  double C1,C10,RHO,RHO0,RHO1;
  int    iterPRE;
  
  KREAL **WW;
  
  KINT R=0,Z=1,Q=1,P=2,DD=3;
  KINT MAXIT;
  KREAL TOL;
  
  double COMPtime;

/**
   +-------+
   | INIT. |
   +-------+
**/
  ERROR= 0;
  
  WW=(KREAL**) allocate_matrix(sizeof(KREAL),4,N);
  
  MAXIT  = ITER;
  TOL   = RESID;          
#pragma omp parallel for private (i)
  for(i=0;i<N;i++){
    X[i]=0.0;	
  }
#pragma omp parallel for private (i,j)
  for(i=0;i<N;i++) for(j=0;j<4;j++) WW[j][i]=0.0;
/**
   +-----------------------+
   | {r0}= {b} - [A]{xini} |
   +-----------------------+
**/
#pragma omp parallel for private (i,j,k,WVAL)
  for(j=0;j<N;j++){
    WW[DD][j]= 1.0/D[j];
    WVAL= B[j] - D[j]*X[j];
    
    for( k=indexLU[j];k<indexLU[j+1];k++){
      i= itemLU[k];
      WVAL+=  -AMAT[k]*X[i];
    }
    WW[R][j]= WVAL;
  }
  
  BNRM2= 0.e0;
#pragma omp parallel for private (i) reduction(+:BNRM2)    
  for(i=0;i<N;i++){
    BNRM2+= B[i]*B[i];
  }
  
  if (BNRM2 == 0.e0) BNRM2= 1.e0;
  
  ITER = 0;
  
  for( ITER=1;ITER<= MAXIT;ITER++){
/**
	************************************************* Conjugate Gradient Iteration
**/

/**
   +----------------+
   | {z}= [Minv]{r} |
   +----------------+
**/
#pragma omp parallel private(i,j,k,WVAL)
{
#pragma omp for  
  for(i=0;i<N;i++){
    WW[Z][i]= WW[DD][i]*WW[R][i];
  }
/**
   +---------------+
   | {RHO}= {r}{z} |
   +---------------+
**/
  RHO= 0.e0;
#pragma omp for reduction(+:RHO)  
  for(i=0;i<N;i++){
    RHO+= WW[R][i]*WW[Z][i];
  }
/**
   +-----------------------------+
   | {p} = {z} if      ITER=1    |
   | BETA= RHO / RHO1  otherwise |
   +-----------------------------+
**/
  if( ITER == 1 ){
#pragma omp for  
    for(i=0;i<N;i++){
      WW[P][i]=WW[Z][i];
    }
  }else{
    BETA= RHO / RHO1;
#pragma omp for  
    for(i=0;i<N;i++){
      WW[P][i]=WW[Z][i] + BETA*WW[P][i];
    }
  }
/**
   +-------------+
   | {q}= [A]{p} |
   +-------------+
**/      
#pragma omp for  
  for( j=0;j<N;j++){
    WVAL= D[j] * WW[P][j];
    for(k=indexLU[j];k<indexLU[j+1];k++){
      i=itemLU[k];
      WVAL+= AMAT[k] * WW[P][i];
    }
    WW[Q][j]=WVAL;
  }

/**
   +---------------------+
   | ALPHA= RHO / {p}{q} |
   +---------------------+
**/
  C1= 0.e0;
#pragma omp for reduction(+:C1)  
  for(i=0;i<N;i++){
    C1+=WW[P][i]*WW[Q][i];
  }

  ALPHA= RHO / C1;

/**
   +----------------------+
   | {x}= {x} + ALPHA*{p} |
   | {r}= {r} - ALPHA*{q} |
   +----------------------+
**/
#pragma omp for  
  for(i=0;i<N;i++){
    X [i]   +=  ALPHA *WW[P][i];
    WW[R][i]+= -ALPHA *WW[Q][i];
  }
  
  DNRM2= 0.e0;
#pragma omp for reduction(+:DNRM2)  
  for(i=0;i<N;i++){
    DNRM2+=WW[R][i]*WW[R][i];
  }
  }
  RESID= sqrt(DNRM2/BNRM2);

/** ##### ITERATION HISTORY ***/
  fprintf(stdout,"%d %e\n",ITER,RESID);
  fprintf(fp_log,"%d %e\n",ITER,RESID);
/** ***/
  if ( RESID <= TOL   ) break;
  if ( ITER  == MAXIT ) *ERROR= -300;
  
  RHO1 = RHO ;                                                           
  }
/** **/
/***
    INTERFACE data EXCHANGE
***/

  free(WW);
}
