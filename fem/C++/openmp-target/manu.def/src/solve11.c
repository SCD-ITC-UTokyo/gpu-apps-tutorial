#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pfem_util.h"

extern FILE *fp_log;
//extern void CG(); 
extern void  CG  (
	   KINT NP,KINT NPLU,KREAL D[],
	   KREAL AMAT[],KINT indexLU[], KINT itemLU[],
	   KREAL B[],KREAL X[],KREAL RESID,KINT ITER, KINT *ERROR); 
void SOLVE11()
{
  /*
  int i,j,k,ii,L;
  int  ICFLAG=0;
  CHAR_LENGTH BUF;
  */
  int  ERROR;
  
/**
   +------------+
   | PARAMETERs |
   +------------+
**/
  ITER      = pfemIarray[0];
  RESID     = pfemRarray[0];

/**
   +------------------+
   | ITERATIVE solver |
   +------------------+
**/
  CG (NP,NPLU, D, AMAT, indexLU, itemLU, B, X, RESID, ITER, &ERROR);
  /* ITERactual / RESIDactual は CG が実反復回数・到達残差を入れて返す。
     ここで ITERactual= ITER (= 最大反復数の設定値) を代入すると上書きして
     しまうため行わない。 */
}

