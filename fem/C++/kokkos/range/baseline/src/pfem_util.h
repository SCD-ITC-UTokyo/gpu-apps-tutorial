/***
	pfem_util.h
***/

#include "precision.h"
#include "kokkos_settings.h"
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
	GLOBAL View2D<KREAL> XYZ_g;
	GLOBAL View2D<KINT> ICELNOD_g;
        GLOBAL KINT  *NODGRP_INDEX;
        GLOBAL View1D<KINT> NODGRP_ITEM_g;
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

        GLOBAL View1D<KREAL> D_g, B_g, X_g;
        GLOBAL View1D<KREAL> AMAT_g;
        GLOBAL View1D<KINT> indexLU_g, itemLU_g;
        GLOBAL KINT *INLU, *ELMCOLORindex;
        GLOBAL View1D<KINT> ELMCOLORitem_g;
        GLOBAL KINT **IALU;
	GLOBAL KINT **IWKX;
/***
	PARAMETER's for LINEAR SOLVER
***/
	GLOBAL KINT ITER, ITERactual;
	GLOBAL KREAL RESID, SIGMA_DIAG, SIGMA;
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
	GLOBAL KREAL POS[2];
	GLOBAL KINT  NCOL1[100], NCOL2[100];
	GLOBAL KREAL PNX[2][2][2][8],PNY[2][2][2][8],PNZ[2][2][2][8];
	GLOBAL KREAL DETJ[2][2][2];
/***
	PROBLEM PARAMETER's
***/
	GLOBAL KREAL COND;
	GLOBAL KREAL QVOL;

inline void view_clear()
{
  XYZ_g=View2D<KREAL>();
  ICELNOD_g=View2D<KINT>();
  NODGRP_ITEM_g=View1D<KINT>();
  indexLU_g=View1D<KINT>();
  itemLU_g=View1D<KINT>();
  D_g=View1D<KREAL>();
  B_g=View1D<KREAL>();
  X_g=View1D<KREAL>();
  AMAT_g=View1D<KREAL>();
  ELMCOLORitem_g=View1D<KINT>();
  Kokkos::fence();
}

