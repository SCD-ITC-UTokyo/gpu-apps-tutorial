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
  XYZ=(KREAL**)allocate_matrix(sizeof(KREAL),N,3);

  for(k=0;k<nxn;k++){
    for(j=0;j<nxn;j++){
      for(i=0;i<nxn;i++){
        n0= i + j*nxn + k*nn;
        XYZ[n0][0]=(KREAL)i;
        XYZ[n0][1]=(KREAL)j;
        XYZ[n0][2]=(KREAL)k;
      }
    }
  }
/**
   ELEMENT
**/
  ICELTOT= nxe * nxe * nxe;

  ICELNOD=(KINT**)allocate_matrix(sizeof(KINT),ICELTOT,8);

  icel= 0;
  for(k=0;k<nxe;k++){
    for(j=0;j<nxe;j++){
      for(i=0;i<nxe;i++){
        n0= 1 + i + j*nxn + k*nn;          /* 下面の原点側節点 (1 origin) */
        ICELNOD[icel][0]= n0;
        ICELNOD[icel][1]= n0 + 1;
        ICELNOD[icel][2]= n0 + 1 + nxn;
        ICELNOD[icel][3]= n0     + nxn;
        ICELNOD[icel][4]= n0          + nn;
        ICELNOD[icel][5]= n0 + 1      + nn;
        ICELNOD[icel][6]= n0 + 1 + nxn+ nn;
        ICELNOD[icel][7]= n0     + nxn+ nn;
        icel++;
      }
    }
  }
/**
   NODE grp. info.
**/
  NODGRPtot= 4;

  NODGRP_INDEX=(KINT*  )allocate_vector(sizeof(KINT),NODGRPtot+1);
  NODGRP_NAME =(CHAR80*)allocate_vector(sizeof(CHAR80),NODGRPtot);
  for(i=0;i<NODGRPtot+1;i++) NODGRP_INDEX[i]= i * nn;  /* 各面 nxn*nxn 節点 */

  NODGRP_ITEM=(KINT*)allocate_vector(sizeof(KINT),NODGRP_INDEX[NODGRPtot]);

  strcpy(NODGRP_NAME[0].name,"Xmin");
  strcpy(NODGRP_NAME[1].name,"Ymin");
  strcpy(NODGRP_NAME[2].name,"Zmin");
  strcpy(NODGRP_NAME[3].name,"Zmax");

  ib= 0;
  for(k=0;k<nxn;k++) for(j=0;j<nxn;j++) NODGRP_ITEM[ib++]= 1 +     j*nxn + k*nn;
  for(k=0;k<nxn;k++) for(i=0;i<nxn;i++) NODGRP_ITEM[ib++]= 1 + i         + k*nn;
  for(j=0;j<nxn;j++) for(i=0;i<nxn;i++) NODGRP_ITEM[ib++]= 1 + i + j*nxn;
  for(j=0;j<nxn;j++) for(i=0;i<nxn;i++) NODGRP_ITEM[ib++]= 1 + i + j*nxn + (nxn-1)*nn;
}
