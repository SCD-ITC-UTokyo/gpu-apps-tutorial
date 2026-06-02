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
  Kokkos::parallel_for("mat_ass_bc_loop1", policy_1d(NODGRP_INDEX[ib0],NODGRP_INDEX[ib0+1],Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int ib){
    int in=NODGRP_ITEM_d(ib);
    IWKX_d(in-1)=1;
  });
  Kokkos::fence();

  Kokkos::parallel_for("mat_ass_bc_loop2", policy_1d(0,NP,Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int in){
    if( IWKX_d(in) == 1 ){
      B_d(in)= 0.e0;
      D_d(in)= 1.e0;
      for(int k=indexLU_d(in);k<indexLU_d(in+1);k++){
	AMAT_d(k)= 0.e0;
      }
    }
  });
  Kokkos::fence();

  Kokkos::parallel_for("mat_ass_bc_loop3", policy_1d(0,NP,Kokkos::ChunkSize(CHUNK_SIZE)), KOKKOS_LAMBDA(const int in){
    for(int k=indexLU_d(in);k<indexLU_d(in+1);k++){
      if (IWKX_d(itemLU_d(k)) == 1 ) {
	AMAT_d(k)= 0.e0;
      }
    }
  });
  Kokkos::fence();
}
