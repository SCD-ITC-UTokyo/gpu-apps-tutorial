/**
 ** INPUT_GRID
 **/
#include <stdio.h>
#include <stdlib.h>
#include "pfem_util.h"

void INPUT_GRID()
{
  FILE *fp;
  int i,j,k,ii,kk,nn,icel,iS,iE;
  int NTYPE,IMAT;
  
  if( (fp=fopen(fname,"r")) == NULL){
    fprintf(stdout,"cube file %s cannot be opened!\n", fname );
    exit(1);
  }
/**
   NODE
**/
  fscanf(fp,"%d",&NP);
  
  XYZ=(KREAL*)malloc(sizeof(KREAL)*NP*3);
  
  for(i=0;i<NP;i++){
    for(j=0;j<3;j++){
      XYZ[i*3+j]=0.0;
    }
  }
  
  for(i=0;i<NP;i++){
#if   FP ==  32
    fscanf(fp,"%d  %f  %f  %f",&ii,&XYZ[i*3+0],&XYZ[i*3+1],&XYZ[i*3+2]);
#elif FP ==  64
    fscanf(fp,"%d %lf %lf %lf",&ii,&XYZ[i*3+0],&XYZ[i*3+1],&XYZ[i*3+2]);
#elif FP == 128
    fscanf(fp,"%d %Lf %Lf %Lf",&ii,&XYZ[i*3+0],&XYZ[i*3+1],&XYZ[i*3+2]);
#endif    
  }
/**
   ELEMENT
**/
  fscanf(fp,"%d",&ICELTOT);

  ICELNOD=(KINT*)malloc(sizeof(KINT)*ICELTOT*8);
  for(i=0;i<ICELTOT;i++) fscanf(fp,"%d",&NTYPE);
  
  
  for(icel=0;icel<ICELTOT;icel++){
    fscanf(fp,"%d %d %d %d %d %d %d %d %d %d",&ii,&IMAT,
	   &ICELNOD[icel*8+0],&ICELNOD[icel*8+1],&ICELNOD[icel*8+2],&ICELNOD[icel*8+3],
	   &ICELNOD[icel*8+4],&ICELNOD[icel*8+5],&ICELNOD[icel*8+6],&ICELNOD[icel*8+7]);
  }
/**
   NODE grp. info.
**/
  fscanf(fp,"%d",&NODGRPtot);
  
  NODGRP_INDEX=(KINT*  )malloc(sizeof(KINT)*(NODGRPtot+1));
  NODGRP_NAME =(CHAR80*)malloc(sizeof(CHAR80)*NODGRPtot);
  for(i=0;i<NODGRPtot+1;i++) NODGRP_INDEX[i]=0;
  
  for(i=0;i<NODGRPtot;i++) fscanf(fp,"%d",&NODGRP_INDEX[i+1]);
  nn=NODGRP_INDEX[NODGRPtot];
  NODGRP_ITEM=(KINT*)malloc(sizeof(KINT)*nn);
  
  for(k=0;k<NODGRPtot;k++){
    iS= NODGRP_INDEX[k];
    iE= NODGRP_INDEX[k+1];
    fscanf(fp,"%s",NODGRP_NAME[k].name);
    nn= iE - iS;
    if( nn != 0 ){
      for(kk=iS;kk<iE;kk++) fscanf(fp,"%d",&NODGRP_ITEM[kk]);
    }
  }
  fclose(fp);
}

