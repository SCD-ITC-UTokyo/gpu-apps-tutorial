/**
 ** CG
 **/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pfem_util.h"

extern FILE *fp_log;
/***
    CG solves the linear system Ax = b using the Conjugate Gradient 
    iterative method with the following preconditioners
 ***/
void  CG  (
	   KINT NP,KINT NPLU,KREAL D[],
	   KREAL AMAT[],KINT indexLU[], KINT itemLU[],
	   KREAL B[],KREAL X[],KREAL RESID,KINT ITER, KINT *ERROR)
{
  int i,j;
  /*
  int ieL,isL,ieU,isU;
  KREAL BNRM20,DNRM20;
  KREAL S1_TIME,E1_TIME;
  int    iterPRE;
  KREAL COMPtime;
  KREAL C10,RHO0
  */
  KREAL ALPHA,BETA;
  KREAL BNRM2,DNRM2;
  KREAL WVAL;
  KREAL C1,RHO,RHO1;
  
  /*
    KREAL **WW;
    KINT R=0,Z=1,Q=1,P=2,DD=3;
  */
  KREAL *RW, *ZW, *QW, *PW, *DW;
  

  KINT MAXIT;
  KREAL TOL;
  

/**
   +-------+
   | INIT. |
   +-------+
**/
  //  ERROR= 0;
  *ERROR= 0; 
  
  //  WW=(KREAL**) allocate_matrix(sizeof(KREAL),4,N);

  RW=(KREAL*)malloc(sizeof(KREAL)*NP);
  ZW=(KREAL*)malloc(sizeof(KREAL)*NP);
  QW=(KREAL*)malloc(sizeof(KREAL)*NP);
  PW=(KREAL*)malloc(sizeof(KREAL)*NP);
  DW=(KREAL*)malloc(sizeof(KREAL)*NP);
  {
  
  MAXIT  = ITER;
  TOL   = RESID;          

#pragma omp target teams distribute parallel for   \
  private(i)
  for(i=0;i<NP;i++){
    X[i]=0.0;	
    //    WW[R][i]=0.0;
    //    WW[Z][i]=0.0;
    //    WW[P][i]=0.0;
    //    WW[DD][i]=0.0;

    RW[i]=0.0;
    ZW[i]=0.0;
    QW[i]=0.0;
    PW[i]=0.0;
    DW[i]=0.0;
  }
/**
   +-----------------------+
   | {r0}= {b} - [A]{xini} |
   +-----------------------+
**/
#pragma omp target teams distribute parallel for \
  private(i,j,WVAL)
  for(i=0;i<NP;i++){
    //    WW[DD][i]= 1.0/D[i];
    DW[i]= 1.0/D[i];
    WVAL= B[i] - D[i]*X[i];
    
    for( j=indexLU[i];j<indexLU[i+1];j++){
      WVAL+=  -AMAT[j]*X[itemLU[j]];
    }
    //    WW[R][i]= WVAL;
    RW[i]= WVAL;
  }
  
  BNRM2= 0.e0;
#pragma omp target teams distribute parallel for \
  private(i) reduction(+:BNRM2)
  for(i=0;i<NP;i++){
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
#pragma omp target teams distribute parallel for \
  private(i)
    for(i=0;i<NP;i++){
      //      WW[Z][i]= WW[DD][i]*WW[R][i];
      ZW[i]= DW[i]*RW[i];
    }
/**
   +---------------+
   | {RHO}= {r}{z} |
   +---------------+
**/
    RHO= 0.e0;
#pragma omp target teams distribute parallel for \
  private(i) reduction(+:RHO)
    for(i=0;i<NP;i++){
      //      RHO+= WW[R][i]*WW[Z][i];
      RHO+= RW[i]*ZW[i];
    }
/**
   +-----------------------------+
   | {p} = {z} if      ITER=1    |
   | BETA= RHO / RHO1  otherwise |
   +-----------------------------+
**/
    if( ITER == 1 ){
#pragma omp target teams distribute parallel for \
  private(i)
      for(i=0;i<NP;i++){
	//	WW[P][i]=WW[Z][i];
	PW[i]=ZW[i];
      }
    }else{
      BETA= RHO / RHO1;
#pragma omp target teams distribute parallel for \
  private(i)
      for(i=0;i<NP;i++){
	//	WW[P][i]=WW[Z][i] + BETA*WW[P][i];
	PW[i]=ZW[i] + BETA*PW[i];
      }
    }
/**
   +-------------+
   | {q}= [A]{p} |
   +-------------+
**/      
#pragma omp target teams distribute parallel for \
  private(i,j,WVAL)
    for( i=0;i<NP;i++){
      //      WVAL= D[i] * WW[P][i];
      WVAL= D[i] * PW[i];
      for(j=indexLU[i];j<indexLU[i+1];j++){
	//	WVAL+= AMAT[j] * WW[P][itemLU[j]];
	WVAL+= AMAT[j] * PW[itemLU[j]];
      }
      //      WW[Q][i]=WVAL;
      QW[i]=WVAL;
    }

/**
   +---------------------+
   | ALPHA= RHO / {p}{q} |
   +---------------------+
**/
    C1= 0.e0;
#pragma omp target teams distribute parallel for \
  private(i) reduction(+:C1)
    for(i=0;i<NP;i++){
      //      C1+=WW[P][i]*WW[Q][i];
      C1+=PW[i]*QW[i];
    }
    ALPHA= RHO / C1;

/**
   +----------------------+
   | {x}= {x} + ALPHA*{p} |
   | {r}= {r} - ALPHA*{q} |
   +----------------------+
**/
#pragma omp target teams distribute parallel for \
  private(i)
    for(i=0;i<NP;i++){
      //      X [i]   +=  ALPHA *WW[P][i];
      X [i]   +=  ALPHA *PW[i];
      //      WW[R][i]+= -ALPHA *WW[Q][i];
      RW[i]+= -ALPHA *QW[i];
    }
  
    DNRM2= 0.e0;
#pragma omp target teams distribute parallel for \
  private(i) reduction(+:DNRM2)
    for(i=0;i<NP;i++){
      //      DNRM2+=WW[R][i]*WW[R][i];
      DNRM2+=RW[i]*RW[i];
    }
#if   FP ==  32
    RESID= sqrtf(DNRM2/BNRM2);
#elif FP ==  64
    RESID= sqrt (DNRM2/BNRM2);
#elif FP == 128
    RESID= sqrtl(DNRM2/BNRM2);
#endif

    /** ##### ITERATION HISTORY ***/
#ifndef BENCHMARK_MODE
    fprintf(stdout,"%d %e\n",ITER,RESID);
    fprintf(fp_log,"%d %e\n",ITER,RESID);
#endif
    /** ***/
    if ( RESID <= TOL   ) break;
    if ( ITER  == MAXIT ) *ERROR= -300;
  
    RHO1 = RHO ;                                                           
  }
/** **/
/***
    INTERFACE data EXCHANGE
***/
  }

//  deallocate_matrix(WW);
  free(RW);
  free(ZW);
  free(QW);
  free(PW);
  free(DW);

  FLOP = (double)ITER*(NP*14 + NPLU*2) + NP*3 + NPLU*2;

  /* グローバル ITERactual に実反復回数を保存 (CG パラメータの ITER はローカル shadow なので追加で必要) */
  ITERactual = ITER;
}
