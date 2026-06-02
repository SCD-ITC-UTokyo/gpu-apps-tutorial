/**
 ** OUTPUT_UCD
 **/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "pfem_util.h"
#include "allocate.h"
extern FILE *fp_log;
void OUTPUT_UCD()
{
  FILE *fp;
  int i,ie;
  int N0,N1,N3,N4;
  int in1,in2,in3,in4,in5,in6,in7,in8;
  double ZERO;
  double XX,YY,ZZ;
  double igmax;
  
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
  ZERO= 0.e0;
  
  fprintf(fp,"%8d%8d%8d%8d%8d\n",N,ICELTOT,N1,N0,N0);
  
  for(i=0;i<N;i++){
    XX=XYZ_g(i,0);
    YY=XYZ_g(i,1);
    ZZ=XYZ_g(i,2);
    fprintf(fp,"%8d%16.6e%16.6e%16.6e\n",i+1,XX,YY,ZZ);
  }
  
  for(ie=0;ie<ICELTOT;ie++){
    strcpy(ETYPE," hex  ");
    in1= ICELNOD_g(ie,0);
    in2= ICELNOD_g(ie,1);
    in3= ICELNOD_g(ie,2);
    in4= ICELNOD_g(ie,3);
    in5= ICELNOD_g(ie,4);
    in6= ICELNOD_g(ie,5);
    in7= ICELNOD_g(ie,6);
    in8= ICELNOD_g(ie,7);
    fprintf(fp,"%8d%3d%-8s%8d%8d%8d%8d%8d%8d%8d%8d\n",
	    ie+1,N1,ETYPE,in1, in2, in3, in4, in5, in6, in7, in8);
  }
  
  fprintf(fp,"%3d%3d\n",N1, N1);
  fprintf(fp,"temp,temp\n");
  
  for(i=0;i<N;i++){
    fprintf(fp,"%8d%16.6e\n",i+1,X_g(i));
  }
  fclose(fp);
}

