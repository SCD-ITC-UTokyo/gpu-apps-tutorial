#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pfem_util.h"
#include "allocate.h"
extern FILE *fp_log;
extern void  CG  (
           KINT N,KINT NPLU,KREAL D[],
           KREAL AMAT[],KINT indexLU[], KINT itemLU[],
           KREAL B[],KREAL X[],KREAL RESID,KINT ITER, KINT *ERROR);
void SOLVE11()
{
  int i,j,k,ii,L;
  
  int  ERROR, ICFLAG=0;
  CHAR_LENGTH BUF;
  
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
  CG (N,NPLU, D, AMAT, indexLU, itemLU, B, X, RESID, ITER, &ERROR);
  ITERactual= ITER;
}

