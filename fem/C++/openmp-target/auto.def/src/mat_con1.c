/**
 ** MAT_CON1
 **/
#include <stdio.h>
#include <stdlib.h>
#include "pfem_util.h"

extern FILE* fp_log;
void MAT_CON1()
{
  int i,k,kk;
  
  indexLU=(KINT*)malloc(sizeof(KINT)*(NP+1));
  for(i=0;i<NP+1;i++) indexLU[i]=0;
  
  for(i=0;i<NP;i++){
    indexLU[i+1]=indexLU[i]+INLU[i];
  }
  
  NPLU=indexLU[NP];
  
  itemLU=(KINT*)malloc(sizeof(KINT)*NPLU);
  
  for(i=0;i<NP;i++){
    for(k=0;k<INLU[i];k++){
      kk=k+indexLU[i];
      itemLU[kk]=IALU[i*NLU+k]-1;
    }
  }
  
  
  free(INLU);
  free(IALU);
}
