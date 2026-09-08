/***
	pfem_util.h
***/

#include "precision.h"
#ifdef GLOBAL_VALUE_DEFINE
#define GLOBAL
#else
#define GLOBAL extern
#endif
/***
	+--------------+
	| MPI settings |
	+--------------+
***/
	GLOBAL char fname[80];
/***
	+-----------+
	| MESH FILE |
	+-----------+
***/

/***
	CONNECTIVITIES & BOUNDARY nodes
***/
	GLOBAL int ICELTOT, NODGRPtot;
	GLOBAL KREAL **XYZ;
	GLOBAL KINT  **ICELNOD;
        GLOBAL KINT  *NODGRP_INDEX, *NODGRP_ITEM;
        GLOBAL CHAR80 *NODGRP_NAME;
/***
	+-----------------+
	| MATRIX & SOLVER |
	+-----------------+
***/
/***
	MATRIX SCALARs 
***/
        GLOBAL KINT N, NP, N2, NLU, NPLU, ELMCOLORtot;
/***
	MATRIX arrays
***/

        GLOBAL KREAL *D, *B, *X;
        GLOBAL KREAL *AMAT;
        GLOBAL KINT *indexLU, *itemLU;
        GLOBAL KINT *INLU, *ELMCOLORindex, *ELMCOLORitem;
        GLOBAL KINT **IALU;
	GLOBAL KINT **IWKX;
/***
	PARAMETER's for LINEAR SOLVER
***/
	GLOBAL KINT ITER, ITERactual;
	GLOBAL KREAL RESID, SIGMA_DIAG, SIGMA, FLOP;
/***
	+-------------+
	| PARAMETER's |
	+-------------+
***/
/***
	GENERAL PARAMETER's
***/
	GLOBAL KINT  pfemIarray[100];
	GLOBAL KREAL pfemRarray[100];
#ifdef GLOBAL_VALUE_DEFINE
	GLOBAL KREAL O8th= 0.125e0;
#else
	GLOBAL KREAL O8th;
#endif
/***
	PARAMETER's for FEM
***/
	GLOBAL KREAL PNQ[2][2][8], PNE[2][2][8], PNT[2][2][8];
	GLOBAL KREAL WEI[2], POS[2];
	GLOBAL KINT  NCOL1[100], NCOL2[100];
	GLOBAL KREAL SHAPE[2][2][2][8];
	GLOBAL KREAL PNX[2][2][2][8],PNY[2][2][2][8],PNZ[2][2][2][8];
	GLOBAL KREAL DETJ[2][2][2];
/***
	PROBLEM PARAMETER's
***/
	GLOBAL KREAL COND;
	GLOBAL KREAL QVOL;

