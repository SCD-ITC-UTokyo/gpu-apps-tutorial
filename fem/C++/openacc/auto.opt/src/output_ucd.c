/**
 ** OUTPUT_UCD
 **/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "pfem_util.h"

extern FILE *fp_log;
//extern void JACOBI(); 
void OUTPUT_UCD()
{
  FILE *fp;
  int i,ie;
  int N0,N1;
  int in1,in2,in3,in4,in5,in6,in7,in8;
  KREAL XX,YY,ZZ;
  /*
    int N3,N4;
    KREAL ZERO;
    KREAL igmax;
  */
  
  char ETYPE[7];
 
/***
	+----------+
	| AVS file |
	+----------+
***/
  if( (fp=fopen("test.inp","w")) == NULL){
    fprintf(stdout,"output file cannot be opened!\n");
    exit(1);}
  
  N0= 0;
  N1= 1;
  //  ZERO= 0.e0;
  
  fprintf(fp,"%8d%8d%8d%8d%8d\n",NP,ICELTOT,N1,N0,N0);
  
  for(i=0;i<NP;i++){
    XX=XYZ[i*3+0];
    YY=XYZ[i*3+1];
    ZZ=XYZ[i*3+2];
    fprintf(fp,"%8d%16.6e%16.6e%16.6e\n",i+1,XX,YY,ZZ);
  }
  
  for(ie=0;ie<ICELTOT;ie++){
    strcpy(ETYPE," hex  ");
    in1= ICELNOD[ie*8+0];
    in2= ICELNOD[ie*8+1];
    in3= ICELNOD[ie*8+2];
    in4= ICELNOD[ie*8+3];
    in5= ICELNOD[ie*8+4];
    in6= ICELNOD[ie*8+5];
    in7= ICELNOD[ie*8+6];
    in8= ICELNOD[ie*8+7];
    fprintf(fp,"%8d%3d%-8s%8d%8d%8d%8d%8d%8d%8d%8d\n",
	    ie+1,N1,ETYPE,in1, in2, in3, in4, in5, in6, in7, in8);
  }
  
  fprintf(fp,"%3d%3d\n",N1, N1);
  fprintf(fp,"temp,temp\n");
  
  for(i=0;i<NP;i++){
    fprintf(fp,"%8d%16.6e\n",i+1,X[i]);
  }
  fclose(fp);
}

