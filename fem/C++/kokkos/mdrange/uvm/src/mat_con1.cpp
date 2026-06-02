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
  for(i=0;i<N+1;i++) indexLU_g(i)=0;
  
  for(i=0;i<N;i++){
    indexLU_g(i+1)=indexLU_g(i)+INLU[i];
  }
  
  NPLU=indexLU_g(N);
  
  itemLU_g=View1D<KINT>("itemLU",NPLU);
  
  for(i=0;i<N;i++){
    for(k=0;k<INLU[i];k++){
      kk=k+indexLU_g(i);
      itemLU_g(kk)=IALU[i][k]-1;
    }
  }

  deallocate_vector(INLU);
  deallocate_vector(IALU);
}
