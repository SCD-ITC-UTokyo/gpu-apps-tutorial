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
  XYZ_g=View2D<KREAL>("XYZ",N,3); //device
  auto XYZ_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, XYZ_g); //host(mirror)
  
  for(i=0;i<N;i++){
    for(j=0;j<3;j++){
      XYZ_h(i,j)=0.0;
    }
  }
  
  for(i=0;i<N;i++){
    fscanf(fp,"%d %lf %lf %lf",&ii,&XYZ_h(i,0),&XYZ_h(i,1),&XYZ_h(i,2));
  }
  Kokkos::deep_copy(XYZ_g,XYZ_h); //host to device

/**
   ELEMENT
**/
  fscanf(fp,"%d",&ICELTOT);

  ICELNOD_g=View2D<KINT>("ICELNOD",ICELTOT,8); //device
  auto ICELNOD_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, ICELNOD_g); //host(mirror)
  for(i=0;i<ICELTOT;i++) fscanf(fp,"%d",&NTYPE);
  
  for(icel=0;icel<ICELTOT;icel++){
    fscanf(fp,"%d %d %d %d %d %d %d %d %d %d",&ii,&IMAT,
	   &ICELNOD_h(icel,0),&ICELNOD_h(icel,1),&ICELNOD_h(icel,2),&ICELNOD_h(icel,3),
	   &ICELNOD_h(icel,4),&ICELNOD_h(icel,5),&ICELNOD_h(icel,6),&ICELNOD_h(icel,7));
  }
  Kokkos::deep_copy(ICELNOD_g,ICELNOD_h); //host to device

/**
   NODE grp. info.
**/
  fscanf(fp,"%d",&NODGRPtot);
  
  NODGRP_INDEX=(KINT*  )allocate_vector(sizeof(KINT),NODGRPtot+1);
  NODGRP_NAME =(CHAR80*)allocate_vector(sizeof(CHAR80),NODGRPtot);
  for(i=0;i<NODGRPtot+1;i++) NODGRP_INDEX[i]=0;
  
  for(i=0;i<NODGRPtot;i++) fscanf(fp,"%d",&NODGRP_INDEX[i+1]);
  nn=NODGRP_INDEX[NODGRPtot];
  NODGRP_ITEM_g=View1D<KINT>("NODGRP_ITEM",nn);
  auto NODGRP_ITEM_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, NODGRP_ITEM_g); //host(mirror)
  
  for(k=0;k<NODGRPtot;k++){
    iS= NODGRP_INDEX[k];
    iE= NODGRP_INDEX[k+1];
    fscanf(fp,"%s",NODGRP_NAME[k].name);
    nn= iE - iS;
    if( nn != 0 ){
      for(kk=iS;kk<iE;kk++) fscanf(fp,"%d",&NODGRP_ITEM_h(kk));
    }
  }
  Kokkos::deep_copy(NODGRP_ITEM_g,NODGRP_ITEM_h);   //host to device

  fclose(fp);
}

