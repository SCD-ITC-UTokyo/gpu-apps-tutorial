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
  
  indexLU=(KINT*)allocate_vector(sizeof(KINT),N+1);
  for(i=0;i<N+1;i++) indexLU[i]=0;
  
  for(i=0;i<N;i++){
    indexLU[i+1]=indexLU[i]+INLU[i];
  }
  
  NPLU=indexLU[N];
  
  itemLU=(KINT*)allocate_vector(sizeof(KINT),NPLU);
  
  for(i=0;i<N;i++){
    for(k=0;k<INLU[i];k++){
      kk=k+indexLU[i];
      itemLU[kk]=IALU[i][k]-1;
    }
  }
  
  deallocate_vector(INLU);
  deallocate_vector(IALU);
}
