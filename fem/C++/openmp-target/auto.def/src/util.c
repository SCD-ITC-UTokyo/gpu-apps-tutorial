#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "precision.h"

/**
 ** mSORT
 **/
void mSORT(KINT STEM[], KINT INUM[], int NN)
{
  int ii,jj;
  int ITEM;
  
  for(ii=1;ii<=NN;ii++){
    INUM[ii-1]=ii;
  }
  
  for( ii=1;ii<=NN-1;ii++){
    for( jj=1;jj<=NN-ii;jj++){
      if( STEM[INUM[jj-1]-1] <  STEM[INUM[jj]-1] ){
	ITEM=INUM[jj];
	INUM[jj  ]=INUM[jj-1];
	INUM[jj-1]=ITEM;
      }
    }
  }
}
/**
 ** matconSORT
 **/
void  matconSORT( KINT STEM[], KINT INUM[], int N, int NN)
{
  int i,k,ii,ik1,ik2,icon;
  int *ISTACK;
  int ICONmax;
  
  ISTACK=(int*)malloc(sizeof(int)*(NN+2)*2);
  
  ISTACK[0*2+0]=0;
  ISTACK[0*2+1]=0;
  
  for(i=1;i<=N;i++){
    INUM[i-1]=i;
    STEM[i-1]++;
  }
  
  for(i=1;i<=N+1;i++){
    ISTACK[(i-1)*2+0]=0;
  }
  
  ICONmax= -N;
  
  for(i=0;i<N;i++){
    ii=STEM[i];
    if( ii > ICONmax ){
      ICONmax=ii;
    }
    ISTACK[(ii-1)*2+0]++;
  }
  
  for(k=1;k<=ICONmax;k++){
    ISTACK[k*2+0]+=ISTACK[(k-1)*2+0];
    ISTACK[k*2+1] =ISTACK[k*2+0];
  }
  
  ISTACK[0*2+1]=ISTACK[1*2+1];
  
  for(k=1;k<=ICONmax;k++){
    ik1=ICONmax - k;
    ik2=ik1     + 1;
    ISTACK[k*2+0]=ISTACK[ik2*2+1]-ISTACK[ik1*2+1]+ISTACK[(k-1)*2+0];
  }
  
  for(k=1;k<=ICONmax;k++){
    ISTACK[k*2+1]= 0;
  }
  
  for(i=0;i<N;i++){
    ii=STEM[i];
    icon=ISTACK[ii*2+1]+1;
    ISTACK[ii*2+1]=icon;
    INUM[ISTACK[(ICONmax-ii+1-1)*2+0]+icon-1]= i;
  }
  
  for(i=0;i<N;i++){
    STEM[i]+=-1;
  }
  
  free(ISTACK);
}

