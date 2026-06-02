#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pfem_util.h"
#include "allocate.h"
extern FILE *fp_log;
extern void  CG  (
           KINT N, KINT NPLU, View1D<KREAL> &D,
           View1D<KREAL> &AMAT, View1D<KINT> &indexLU, View1D<KINT> &itemLU,
           View1D<KREAL> &B, View1D<KREAL> &X, KREAL RESID, KINT ITER, KINT *ERROR);
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
  CG (N,NPLU, D_g, AMAT_g, indexLU_g, itemLU_g, B_g, X_g, RESID, ITER, &ERROR);
  ITERactual= ITER;
}

