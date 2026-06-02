/**
 ** CG
 **/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "precision.h"
#include "allocate.h"
#include "kokkos_settings.h"
extern FILE *fp_log;
/***
    CG solves the linear system Ax = b using the Conjugate Gradient 
    iterative method with the following preconditioners
 ***/
void  CG  (
           KINT N, KINT NPLU, View1D<KREAL> &D,
           View1D<KREAL> &AMAT, View1D<KINT> &indexLU, View1D<KINT> &itemLU,
           View1D<KREAL> &B, View1D<KREAL> &X, KREAL RESID, KINT ITER, KINT *ERROR)
{
  int ieL,isL,ieU,isU;
  double BNRM20,BNRM2,DNRM20,DNRM2;
  double S1_TIME,E1_TIME;
  double ALPHA,BETA;
  double C1,C10,RHO,RHO0,RHO1;
  int    iterPRE;
  
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
  
  View2D<KREAL> WW("WW", 4, N);
  
  MAXIT  = ITER;
  TOL   = RESID;          
  Kokkos::parallel_for("init_X", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i)
  {
    X(i)=0.0;	
  });
  Kokkos::parallel_for("init_WW", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i)
  {
    for(int j=0;j<4;j++){
      WW(j, i) = 0.0;
    }
  });
  Kokkos::fence();


/**
   +-----------------------+
   | {r0}= {b} - [A]{xini} |
   +-----------------------+
**/
  Kokkos::parallel_for("r0", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int j)
  {
    WW(DD, j)= 1.0/D(j);
    KREAL WVAL= B(j) - D(j)*X(j);
    
    for( int k=indexLU(j);k<indexLU(j+1);k++){
      int i = itemLU(k);
      WVAL += -AMAT(k)*X(i);
    }
    WW(R, j)= WVAL;
  });
  
  BNRM2= 0.e0;
  Kokkos::parallel_reduce("BNRM2", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i, KREAL &BNRM2_l)
  {
    BNRM2_l+= B(i)*B(i);
  }, BNRM2);
  Kokkos::fence();
  
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
{
  Kokkos::parallel_for("z", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i)
  {
    WW(Z, i)= WW(DD, i)*WW(R, i);
  });
  Kokkos::fence();
/**
   +---------------+
   | {RHO}= {r}{z} |
   +---------------+
**/
  RHO= 0.e0;
  Kokkos::parallel_reduce("RHO", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i, KREAL &RHO_l)
  {
    RHO_l+= WW(R, i)*WW(Z, i);
  }, RHO);
  Kokkos::fence();
/**
   +-----------------------------+
   | {p} = {z} if      ITER=1    |
   | BETA= RHO / RHO1  otherwise |
   +-----------------------------+
**/
  if( ITER == 1 ){
    Kokkos::parallel_for("p_iter1", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i)
    {
      WW(P, i)=WW(Z, i);
    });
  }else{
    BETA= RHO / RHO1;
    Kokkos::parallel_for("p_other", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i)
    {
      WW(P, i)=WW(Z, i) + BETA*WW(P, i);
    });
  }
  Kokkos::fence();
/**
   +-------------+
   | {q}= [A]{p} |
   +-------------+
**/      
  Kokkos::parallel_for("q", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int j)
  {
    KREAL WVAL= D(j) * WW(P, j);
    for(int k=indexLU(j);k<indexLU(j+1);k++){
      int i=itemLU(k);
      WVAL+= AMAT(k) * WW(P, i);
    }
    WW(Q, j)=WVAL;
  });
  Kokkos::fence();

/**
   +---------------------+
   | ALPHA= RHO / {p}{q} |
   +---------------------+
**/
  C1= 0.e0;
  Kokkos::parallel_reduce("C1", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i, KREAL &C1_l)
  {
    C1_l+=WW(P, i)*WW(Q, i);
  }, C1);
  Kokkos::fence();

  ALPHA= RHO / C1;

/**
   +----------------------+
   | {x}= {x} + ALPHA*{p} |
   | {r}= {r} - ALPHA*{q} |
   +----------------------+
**/
  Kokkos::parallel_for("xr", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i)
  {
    X (i)   +=  ALPHA *WW(P, i);
    WW(R, i)+= -ALPHA *WW(Q, i);
  });
  Kokkos::fence();
  
  DNRM2= 0.e0;
  Kokkos::parallel_reduce("DNRM2", policy_1d(0, N, Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int i, KREAL &DNRM2_l)
  {
    DNRM2_l+=WW(R, i)*WW(R, i);
  }, DNRM2);
  Kokkos::fence();
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

}
