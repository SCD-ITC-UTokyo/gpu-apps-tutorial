/**
 ** INPUT_GRID
 **
 ** 1 辺 nxn 節点の立方体メッシュ (8 節点六面体 1 次要素) を内部生成する。
 ** 旧版はメッシュファイル cube.0 を読んでいたが、格子が完全に規則的で
 ** nxn 一つから決まるため、diffusion / nbody と同様にファイル不要にした。
 ** 生成されるデータは旧 cube.0 (nxn=65) と完全に同一。
 **
 **   節点番号 (1 origin): in = 1 + i + j*nxn + k*nxn*nxn   (x が最内)
 **   節点座標            : (i, j, k)  格子間隔 1.0
 **   要素結合            : 下面 4 節点 (反時計回り) → 上面 4 節点
 **   節点グループ        : Xmin(i=0) / Ymin(j=0) / Zmin(k=0) / Zmax(k=nxn-1)
 **                         MAT_ASS_BC が使うのは Zmax のみ
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfem_util.h"
#include "allocate.h"
void INPUT_GRID(int nxn)
{
  int i,j,k,icel,ib,n0;
  int nxe, nn;

  if( nxn < 2 ){
    fprintf(stdout,"N must be 2 or larger (given: %d)\n", nxn);
    exit(1);
  }
  nxe= nxn - 1;      /* 1 辺あたりの要素数 */
  nn = nxn * nxn;    /* z 方向の節点ストライド */
/**
   NODE
**/
  N = nxn * nxn * nxn;

  NP=N;
  XYZ_g=View2D<KREAL>("XYZ",N,3); //device
  auto XYZ_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, XYZ_g); //host(mirror)

  for(k=0;k<nxn;k++){
    for(j=0;j<nxn;j++){
      for(i=0;i<nxn;i++){
        n0= i + j*nxn + k*nn;
        XYZ_h(n0,0)=(KREAL)i;
        XYZ_h(n0,1)=(KREAL)j;
        XYZ_h(n0,2)=(KREAL)k;
      }
    }
  }
  Kokkos::deep_copy(XYZ_g,XYZ_h); //host to device

/**
   ELEMENT
**/
  ICELTOT= nxe * nxe * nxe;

  ICELNOD_g=View2D<KINT>("ICELNOD",ICELTOT,8); //device
  auto ICELNOD_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, ICELNOD_g); //host(mirror)

  icel= 0;
  for(k=0;k<nxe;k++){
    for(j=0;j<nxe;j++){
      for(i=0;i<nxe;i++){
        n0= 1 + i + j*nxn + k*nn;          /* 下面の原点側節点 (1 origin) */
        ICELNOD_h(icel,0)= n0;
        ICELNOD_h(icel,1)= n0 + 1;
        ICELNOD_h(icel,2)= n0 + 1 + nxn;
        ICELNOD_h(icel,3)= n0     + nxn;
        ICELNOD_h(icel,4)= n0          + nn;
        ICELNOD_h(icel,5)= n0 + 1      + nn;
        ICELNOD_h(icel,6)= n0 + 1 + nxn+ nn;
        ICELNOD_h(icel,7)= n0     + nxn+ nn;
        icel++;
      }
    }
  }
  Kokkos::deep_copy(ICELNOD_g,ICELNOD_h); //host to device

/**
   NODE grp. info.
**/
  NODGRPtot= 4;

  NODGRP_INDEX=(KINT*  )allocate_vector(sizeof(KINT),NODGRPtot+1);
  NODGRP_NAME =(CHAR80*)allocate_vector(sizeof(CHAR80),NODGRPtot);
  for(i=0;i<NODGRPtot+1;i++) NODGRP_INDEX[i]= i * nn;  /* 各面 nxn*nxn 節点 */

  NODGRP_ITEM_g=View1D<KINT>("NODGRP_ITEM",NODGRP_INDEX[NODGRPtot]);
  auto NODGRP_ITEM_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, NODGRP_ITEM_g); //host(mirror)

  strcpy(NODGRP_NAME[0].name,"Xmin");
  strcpy(NODGRP_NAME[1].name,"Ymin");
  strcpy(NODGRP_NAME[2].name,"Zmin");
  strcpy(NODGRP_NAME[3].name,"Zmax");

  ib= 0;
  for(k=0;k<nxn;k++) for(j=0;j<nxn;j++) NODGRP_ITEM_h(ib++)= 1 +     j*nxn + k*nn;
  for(k=0;k<nxn;k++) for(i=0;i<nxn;i++) NODGRP_ITEM_h(ib++)= 1 + i         + k*nn;
  for(j=0;j<nxn;j++) for(i=0;i<nxn;i++) NODGRP_ITEM_h(ib++)= 1 + i + j*nxn;
  for(j=0;j<nxn;j++) for(i=0;i<nxn;i++) NODGRP_ITEM_h(ib++)= 1 + i + j*nxn + (nxn-1)*nn;
  Kokkos::deep_copy(NODGRP_ITEM_g,NODGRP_ITEM_h);   //host to device
}
