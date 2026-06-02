/**
 ** INPUT_GRID
 **/
#include <stdio.h>
#include <stdlib.h>
#include "pfem_util.h"
#include "allocate.h"
void INPUT_GRID()
{
  FILE *fp;
  int i,j,k,ii,kk,nn,icel,iS,iE;
  int NTYPE,IMAT;
  
  if( (fp=fopen(fname,"r")) == NULL){
    fprintf(stdout,"input file cannot be opened!\n");
    exit(1);
  }
/**
   NODE
**/
  fscanf(fp,"%d",&N);
  
  NP=N;
  XYZ=(KREAL**)allocate_matrix(sizeof(KREAL),N,3);
  
  for(i=0;i<N;i++){
    for(j=0;j<3;j++){
      XYZ[i][j]=0.0;
    }
  }
  
  for(i=0;i<N;i++){
    fscanf(fp,"%d %lf %lf %lf",&ii,&XYZ[i][0],&XYZ[i][1],&XYZ[i][2]);
  }
/**
   ELEMENT
**/
  fscanf(fp,"%d",&ICELTOT);

  ICELNOD=(KINT**)allocate_matrix(sizeof(KINT),ICELTOT,8);
  for(i=0;i<ICELTOT;i++) fscanf(fp,"%d",&NTYPE);
  
  
  for(icel=0;icel<ICELTOT;icel++){
    fscanf(fp,"%d %d %d %d %d %d %d %d %d %d",&ii,&IMAT,
	   &ICELNOD[icel][0],&ICELNOD[icel][1],&ICELNOD[icel][2],&ICELNOD[icel][3],
	   &ICELNOD[icel][4],&ICELNOD[icel][5],&ICELNOD[icel][6],&ICELNOD[icel][7]);
  }
/**
   NODE grp. info.
**/
  fscanf(fp,"%d",&NODGRPtot);
  
  NODGRP_INDEX=(KINT*  )allocate_vector(sizeof(KINT),NODGRPtot+1);
  NODGRP_NAME =(CHAR80*)allocate_vector(sizeof(CHAR80),NODGRPtot);
  for(i=0;i<NODGRPtot+1;i++) NODGRP_INDEX[i]=0;
  
  for(i=0;i<NODGRPtot;i++) fscanf(fp,"%d",&NODGRP_INDEX[i+1]);
  nn=NODGRP_INDEX[NODGRPtot];
  NODGRP_ITEM=(KINT*)allocate_vector(sizeof(KINT),nn);
  
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

