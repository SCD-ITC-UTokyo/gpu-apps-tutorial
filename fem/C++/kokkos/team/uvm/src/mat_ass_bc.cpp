/**
 ** MAT_ASS_BC
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfem_util.h"
#include "allocate.h"
extern FILE *fp_log;
void MAT_ASS_BC()
{
  int i,j,k,in,ib,ib0,icel;
  int in1,in2,in3,in4,in5,in6,in7,in8;
  int iq1,iq2,iq3,iq4,iq5,iq6,iq7,iq8;
  int iS,iE;
  double STRESS,VAL;
  
  View1D<KINT> IWKX_d("IWKX",NP);
  Kokkos::deep_copy(IWKX_d, 0.0);

  // local copy (for KOKKOS_LAMBDA capture)
  auto NODGRP_ITEM_d=NODGRP_ITEM_g;
  auto indexLU_d=indexLU_g;
  auto itemLU_d=itemLU_g;
  auto B_d=B_g;
  auto D_d=D_g;
  auto AMAT_d=AMAT_g;

/**
   Z=Zmax
**/
  ib0=-1;
  for( ib0=0;ib0<NODGRPtot;ib0++){
    if( strcmp(NODGRP_NAME[ib0].name,"Zmax") == 0 ) break;
  }
  // TeamPolicy for loop NODGRP_INDEX
  int n1=NODGRP_INDEX[ib0+1]-NODGRP_INDEX[ib0];
  int ioff=NODGRP_INDEX[ib0];
  const int chunk1 = CHUNK_SIZE; //chunk size
  const int league_size1 = (n1 + chunk1 - 1) / chunk1; //num of teams
  team_policy policy_t1(league_size1, Kokkos::AUTO());
    Kokkos::parallel_for("mat_ass_bc_loop1", policy_t1, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk1;
    const int end   = (begin + chunk1 < n1) ? (begin + chunk1) : n1;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin+ioff, end+ioff), [&] (const int ib){
      int in=NODGRP_ITEM_d(ib);
      IWKX_d(in-1)=1;
    });
  });
  Kokkos::fence();

  // TeamPolicy for loop NP
  const int chunk2 = CHUNK_SIZE; //chunk size
  int n2=NP;
  const int league_size2 = (n2 + chunk2 - 1) / chunk2; //num of teams
  team_policy policy_t2(league_size2, Kokkos::AUTO());

  Kokkos::parallel_for("mat_ass_bc_loop2", policy_t2, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk2;
    const int end   = (begin + chunk2 < n2) ? (begin + chunk2) : n2;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int in){
      if( IWKX_d(in) == 1 ){
        B_d(in)= 0.e0;
        D_d(in)= 1.e0;
        for(int k=indexLU_d(in);k<indexLU_d(in+1);k++){
  	AMAT_d(k)= 0.e0;
        }
      }
    });
  });
  Kokkos::fence();

  Kokkos::parallel_for("mat_ass_bc_loop3", policy_t2, KOKKOS_LAMBDA(const member_type& team)
  {
    const int begin = team.league_rank() * chunk2;
    const int end   = (begin + chunk2 < n2) ? (begin + chunk2) : n2;
    Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin, end), [&] (const int in){
      for(int k=indexLU_d(in);k<indexLU_d(in+1);k++){
        if (IWKX_d(itemLU_d(k)) == 1 ) {
	  AMAT_d(k)= 0.e0;
        }
      }
    });
  });
  Kokkos::fence();
}
