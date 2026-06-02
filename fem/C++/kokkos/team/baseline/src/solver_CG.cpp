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

  // TeamPolicy
  const int chunk = CHUNK_SIZE; //chunk size
  const int league_size = (N + chunk - 1) / chunk; //num of teams
  team_policy policy_t(league_size, Kokkos::AUTO());

/**
   +-------+
   | INIT. |
   +-------+
**/
  ERROR= 0;
  
  View2D<KREAL> WW("WW", 4, N);
  
  MAXIT  = ITER;
  TOL   = RESID;          
  Kokkos::parallel_for("init_X", policy_t, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i)
    {
      X(i)=0.0;	
    });
  });
  Kokkos::parallel_for("init_WW", policy_t, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i)
    {
      for(int j=0;j<4;j++)
      {
        WW(j, i) = 0.0;
      }
    });
  });
  Kokkos::fence();

/**
   +-----------------------+
   | {r0}= {b} - [A]{xini} |
   +-----------------------+
**/
  Kokkos::parallel_for("r0", policy_t, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int j)
    {
      WW(DD, j)= 1.0/D(j);
      KREAL WVAL= B(j) - D(j)*X(j);
    
      for( int k=indexLU(j);k<indexLU(j+1);k++){
        int i = itemLU(k);
        WVAL += -AMAT(k)*X(i);
      }
      WW(R, j)= WVAL;
    });
  });
  
  BNRM2= 0.e0;
  Kokkos::parallel_reduce("BNRM2", policy_t, KOKKOS_LAMBDA(const member_type& team, KREAL &sum_outer)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    KREAL sum_local = 0.0;
    Kokkos::parallel_reduce(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i, KREAL &sum_inner)
    {
      sum_inner+= B(i)*B(i);
    }, sum_local);
    sum_outer += sum_local;
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
  Kokkos::parallel_for("z", policy_t, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i)
    {
      WW(Z, i)= WW(DD, i)*WW(R, i);
    });
  });
  Kokkos::fence();
/**
   +---------------+
   | {RHO}= {r}{z} |
   +---------------+
**/
  RHO= 0.e0;
  Kokkos::parallel_reduce("RHO", policy_t, KOKKOS_LAMBDA(const member_type& team, KREAL &sum_outer)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    KREAL sum_local = 0.0;
    Kokkos::parallel_reduce(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i, KREAL &sum_inner)
    {
      sum_inner+= WW(R, i)*WW(Z, i);
    }, sum_local);
    sum_outer += sum_local;
    }, RHO);
  Kokkos::fence();
/**
   +-----------------------------+
   | {p} = {z} if      ITER=1    |
   | BETA= RHO / RHO1  otherwise |
   +-----------------------------+
**/
  if( ITER == 1 ){
    Kokkos::parallel_for("z", policy_t, KOKKOS_LAMBDA(const member_type& team)
    {
      const int begin = team.league_rank() * chunk;
      const int end   = (begin + chunk < N) ? (begin + chunk) : N;
      Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i)
      {
        WW(P, i)=WW(Z, i);
      });
    });
  }else{
    BETA= RHO / RHO1;
    Kokkos::parallel_for("p_other", policy_t, KOKKOS_LAMBDA(const member_type& team)
    {
      const int begin = team.league_rank() * chunk;
      const int end   = (begin + chunk < N) ? (begin + chunk) : N;
      Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i)
      {
        WW(P, i)=WW(Z, i) + BETA*WW(P, i);
      });
    });
  }
  Kokkos::fence();
/**
   +-------------+
   | {q}= [A]{p} |
   +-------------+
**/      
  Kokkos::parallel_for("q", policy_t, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int j)
    {
      KREAL WVAL= D(j) * WW(P, j);
      for(int k=indexLU(j);k<indexLU(j+1);k++){
        int i=itemLU(k);
        WVAL+= AMAT(k) * WW(P, i);
      }
      WW(Q, j)=WVAL;
    });
  });
  Kokkos::fence();

/**
   +---------------------+
   | ALPHA= RHO / {p}{q} |
   +---------------------+
**/
  C1= 0.e0;
  Kokkos::parallel_reduce("C1", policy_t, KOKKOS_LAMBDA(const member_type& team, KREAL &sum_outer)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    KREAL sum_local = 0.0;
    Kokkos::parallel_reduce(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i, KREAL &sum_inner)
    {
      sum_inner+=WW(P, i)*WW(Q, i);
    }, sum_local);
    sum_outer += sum_local;
  }, C1);
  Kokkos::fence();

  ALPHA= RHO / C1;

/**
   +----------------------+
   | {x}= {x} + ALPHA*{p} |
   | {r}= {r} - ALPHA*{q} |
   +----------------------+
**/
  Kokkos::parallel_for("xr", policy_t, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i)
    {
      X (i)   +=  ALPHA *WW(P, i);
      WW(R, i)+= -ALPHA *WW(Q, i);
    });
  });
  Kokkos::fence();
  
  DNRM2= 0.e0;
  Kokkos::parallel_reduce("DNRM2", policy_t, KOKKOS_LAMBDA(const member_type& team, KREAL &sum_outer)
  {
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < N) ? (begin + chunk) : N;
    KREAL sum_local = 0.0;
    Kokkos::parallel_reduce(Kokkos::TeamThreadRange(team, begin, end), [&] (const int i, KREAL &sum_inner)
    {
      sum_inner+=WW(R, i)*WW(R, i);
    }, sum_local);
    sum_outer += sum_local;
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
