/**
 ** MAT_ASS_BC
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfem_util.h"

extern FILE *fp_log;
void MAT_ASS_BC()
{
  /*
  int icel;
  int in1,in2,in3,in4,in5,in6,in7,in8;
  int iq1,iq2,iq3,iq4,iq5,iq6,iq7,iq8;
  int iS,iE;
  KREAL STRESS,VAL;
  */
  int i,j,k,in,ib,ib0;
  KINT *IWKX;
  
  IWKX=(KINT*)malloc(sizeof(KINT)*NP*2);
  for(i=0;i<NP;i++) for(j=0;j<2;j++) IWKX[i*2+j]=0;
  
/**
   Z=Zmax
**/
#pragma omp parallel for private(in)
  for(in=0;in<NP;in++) IWKX[in*2+0]=0;
  
  ib0=-1;
  for( ib0=0;ib0<NODGRPtot;ib0++){
    if( strcmp(NODGRP_NAME[ib0].name,"Zmax") == 0 ) break;
  }
#pragma omp parallel for private(ib,in)    
  for( ib=NODGRP_INDEX[ib0];ib<NODGRP_INDEX[ib0+1];ib++){
    in=NODGRP_ITEM[ib];
    IWKX[(in-1)*2+0]=1;
  }

#pragma omp target data \
  map(to: IWKX[0:NP*2],indexLU[0:NP+1],itemLU[0:NPLU])
  {
  
#pragma omp target teams distribute parallel for	   \
  map(alloc: IWKX[0:NP*2],indexLU[0:NP+1])		   \
  map(alloc: AMAT[0:NPLU],B[0:NP],D[0:NP])		   \
  private(in,k)
  for(in=0;in<NP;in++){
    if( IWKX[in*2+0] == 1 ){
      B[in]= 0.e0;
      D[in]= 1.e0;
      for(k=indexLU[in];k<indexLU[in+1];k++){
	AMAT[k]= 0.e0;
      }
    }
  }

#pragma omp target teams distribute parallel for \
  map(alloc: IWKX[0:NP*2],indexLU[0:NP+1],itemLU[0:NPLU])	\
  map(alloc: AMAT[0:NPLU]) \
  private(in,k)
  for(in=0;in<NP;in++){
    for(k=indexLU[in];k<indexLU[in+1];k++){
      if (IWKX[itemLU[k]*2+0] == 1 ) {
	AMAT[k]= 0.e0;
      }
    }
  }

  } // pragma omp target end data

  free(IWKX); 
}
