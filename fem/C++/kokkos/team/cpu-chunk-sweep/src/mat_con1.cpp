/**
 ** MAT_CON1
 **/
#include <stdio.h>
#include "pfem_util.h"
#include "allocate.h"
extern FILE* fp_log;
void MAT_CON1()
{
  int i,k,kk;
  
  indexLU_g=View1D<KINT>("indexLU",N+1);
  auto indexLU_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, indexLU_g); //host(mirror)
  for(i=0;i<N+1;i++) indexLU_h(i)=0;
  
  for(i=0;i<N;i++){
    indexLU_h(i+1)=indexLU_h(i)+INLU[i];
  }
  
  NPLU=indexLU_h(N);
  
  itemLU_g=View1D<KINT>("itemLU",NPLU);
  auto itemLU_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, itemLU_g); //host(mirror)
  
  for(i=0;i<N;i++){
    for(k=0;k<INLU[i];k++){
      kk=k+indexLU_h(i);
      itemLU_h(kk)=IALU[i][k]-1;
    }
  }

  Kokkos::deep_copy(indexLU_g,indexLU_h); //host to device
  Kokkos::deep_copy(itemLU_g,itemLU_h); //host to device
 
  deallocate_vector(INLU);
  deallocate_vector(IALU);
}
