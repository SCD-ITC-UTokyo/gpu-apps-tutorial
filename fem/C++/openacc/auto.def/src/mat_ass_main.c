/**
 ** MAT_ASS_MAIN
 **/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pfem_util.h"

extern FILE *fp_log;


/**
 ** JACOBI
 **/
static inline void JACOBI(
	    KREAL DETJ[2][2][2],
	    KREAL PNQ[2][2][8],KREAL PNE[2][2][8],KREAL PNT[2][2][8],
	    KREAL PNX[2][2][2][8],KREAL PNY[2][2][2][8],KREAL PNZ[2][2][2][8],
	    KREAL X1,KREAL X2,KREAL X3,KREAL X4,KREAL X5,KREAL X6,KREAL X7,KREAL X8,
	    KREAL Y1,KREAL Y2,KREAL Y3,KREAL Y4,KREAL Y5,KREAL Y6,KREAL Y7,KREAL Y8,
	    KREAL Z1,KREAL Z2,KREAL Z3,KREAL Z4,KREAL Z5,KREAL Z6,KREAL Z7,KREAL Z8)
{
/**
	calculates JACOBIAN & INVERSE JACOBIAN
	             dNi/dx, dNi/dy & dNi/dz         
**/ 
  int ip,jp,kp;
  KREAL dXdQ,dYdQ,dZdQ,dXdE,dYdE,dZdE,dXdT,dYdT,dZdT;
  KREAL coef;
  KREAL a11,a12,a13,a21,a22,a23,a31,a32,a33;
  
  for(ip=0;ip<2;ip++){
    for(jp=0;jp<2;jp++){
      for(kp=0;kp<2;kp++){
	PNX[ip][jp][kp][0]=0.0;
	PNX[ip][jp][kp][1]=0.0;
	PNX[ip][jp][kp][2]=0.0;
	PNX[ip][jp][kp][3]=0.0;
	PNX[ip][jp][kp][4]=0.0;
	PNX[ip][jp][kp][5]=0.0;
	PNX[ip][jp][kp][6]=0.0;
	PNX[ip][jp][kp][7]=0.0;
	
	PNY[ip][jp][kp][0]=0.0;
	PNY[ip][jp][kp][1]=0.0;
	PNY[ip][jp][kp][2]=0.0;
	PNY[ip][jp][kp][3]=0.0;
	PNY[ip][jp][kp][4]=0.0;
	PNY[ip][jp][kp][5]=0.0;
	PNY[ip][jp][kp][6]=0.0;
	PNY[ip][jp][kp][7]=0.0;
	
	PNZ[ip][jp][kp][0]=0.0;
	PNZ[ip][jp][kp][1]=0.0;
	PNZ[ip][jp][kp][2]=0.0;
	PNZ[ip][jp][kp][3]=0.0;
	PNZ[ip][jp][kp][4]=0.0;
	PNZ[ip][jp][kp][5]=0.0;
	PNZ[ip][jp][kp][6]=0.0;
	PNZ[ip][jp][kp][7]=0.0;
		
/**    
       DETERMINANT of the JACOBIAN
**/
	dXdQ = PNQ[jp][kp][0]*X1 + PNQ[jp][kp][1]*X2                               
	  + PNQ[jp][kp][2]*X3 + PNQ[jp][kp][3]*X4
	  + PNQ[jp][kp][4]*X5 + PNQ[jp][kp][5]*X6  
	  + PNQ[jp][kp][6]*X7 + PNQ[jp][kp][7]*X8;
	dYdQ = PNQ[jp][kp][0]*Y1 + PNQ[jp][kp][1]*Y2                               
	  + PNQ[jp][kp][2]*Y3 + PNQ[jp][kp][3]*Y4
	  + PNQ[jp][kp][4]*Y5 + PNQ[jp][kp][5]*Y6  
	  + PNQ[jp][kp][6]*Y7 + PNQ[jp][kp][7]*Y8;
	dZdQ = PNQ[jp][kp][0]*Z1 + PNQ[jp][kp][1]*Z2                               
	  + PNQ[jp][kp][2]*Z3 + PNQ[jp][kp][3]*Z4
	  + PNQ[jp][kp][4]*Z5 + PNQ[jp][kp][5]*Z6  
	  + PNQ[jp][kp][6]*Z7 + PNQ[jp][kp][7]*Z8;
	dXdE = PNE[ip][kp][0]*X1 + PNE[ip][kp][1]*X2                               
	  + PNE[ip][kp][2]*X3 + PNE[ip][kp][3]*X4
	  + PNE[ip][kp][4]*X5 + PNE[ip][kp][5]*X6  
	  + PNE[ip][kp][6]*X7 + PNE[ip][kp][7]*X8;			
	dYdE = PNE[ip][kp][0]*Y1 + PNE[ip][kp][1]*Y2                               
	  + PNE[ip][kp][2]*Y3 + PNE[ip][kp][3]*Y4
	  + PNE[ip][kp][4]*Y5 + PNE[ip][kp][5]*Y6  
	  + PNE[ip][kp][6]*Y7 + PNE[ip][kp][7]*Y8;
	dZdE = PNE[ip][kp][0]*Z1 + PNE[ip][kp][1]*Z2                               
	  + PNE[ip][kp][2]*Z3 + PNE[ip][kp][3]*Z4
	  + PNE[ip][kp][4]*Z5 + PNE[ip][kp][5]*Z6  
	  + PNE[ip][kp][6]*Z7 + PNE[ip][kp][7]*Z8;
	dXdT = PNT[ip][jp][0]*X1 + PNT[ip][jp][1]*X2                               
	  + PNT[ip][jp][2]*X3 + PNT[ip][jp][3]*X4
	  + PNT[ip][jp][4]*X5 + PNT[ip][jp][5]*X6  
	  + PNT[ip][jp][6]*X7 + PNT[ip][jp][7]*X8;			
	dYdT = PNT[ip][jp][0]*Y1 + PNT[ip][jp][1]*Y2                               
	  + PNT[ip][jp][2]*Y3 + PNT[ip][jp][3]*Y4
	  + PNT[ip][jp][4]*Y5 + PNT[ip][jp][5]*Y6  
	  + PNT[ip][jp][6]*Y7 + PNT[ip][jp][7]*Y8;
	dZdT = PNT[ip][jp][0]*Z1 + PNT[ip][jp][1]*Z2                               
	  + PNT[ip][jp][2]*Z3 + PNT[ip][jp][3]*Z4
	  + PNT[ip][jp][4]*Z5 + PNT[ip][jp][5]*Z6  
	  + PNT[ip][jp][6]*Z7 + PNT[ip][jp][7]*Z8;
	DETJ[ip][jp][kp]= dXdQ*(dYdE*dZdT-dZdE*dYdT) +
	  dYdQ*(dZdE*dXdT-dXdE*dZdT) + 
	  dZdQ*(dXdE*dYdT-dYdE*dXdT);
/**
   INVERSE JACOBIAN
**/
	
	coef=1.0 / DETJ[ip][jp][kp];
	
	a11= coef * ( dYdE*dZdT - dZdE*dYdT );
	a12= coef * ( dZdQ*dYdT - dYdQ*dZdT );
	a13= coef * ( dYdQ*dZdE - dZdQ*dYdE );
	
	a21= coef * ( dZdE*dXdT - dXdE*dZdT );
	a22= coef * ( dXdQ*dZdT - dZdQ*dXdT );
	a23= coef * ( dZdQ*dXdE - dXdQ*dZdE );
	
	a31= coef * ( dXdE*dYdT - dYdE*dXdT );
	a32= coef * ( dYdQ*dXdT - dXdQ*dYdT );
	a33= coef * ( dXdQ*dYdE - dYdQ*dXdE );

#if   FP ==  32
	DETJ[ip][jp][kp]=fabsf(DETJ[ip][jp][kp]);
#elif FP ==  64
	DETJ[ip][jp][kp]=fabs (DETJ[ip][jp][kp]);
#elif FP == 128
	DETJ[ip][jp][kp]=fabsl(DETJ[ip][jp][kp]);
#endif
	
	
/**
	set the dNi/dX, dNi/dY & dNi/dZ components
**/
	PNX[ip][jp][kp][0]=a11*PNQ[jp][kp][0]+a12*PNE[ip][kp][0]+a13*PNT[ip][jp][0];
	PNX[ip][jp][kp][1]=a11*PNQ[jp][kp][1]+a12*PNE[ip][kp][1]+a13*PNT[ip][jp][1];
	PNX[ip][jp][kp][2]=a11*PNQ[jp][kp][2]+a12*PNE[ip][kp][2]+a13*PNT[ip][jp][2];
	PNX[ip][jp][kp][3]=a11*PNQ[jp][kp][3]+a12*PNE[ip][kp][3]+a13*PNT[ip][jp][3];
	PNX[ip][jp][kp][4]=a11*PNQ[jp][kp][4]+a12*PNE[ip][kp][4]+a13*PNT[ip][jp][4];
	PNX[ip][jp][kp][5]=a11*PNQ[jp][kp][5]+a12*PNE[ip][kp][5]+a13*PNT[ip][jp][5];
	PNX[ip][jp][kp][6]=a11*PNQ[jp][kp][6]+a12*PNE[ip][kp][6]+a13*PNT[ip][jp][6];
	PNX[ip][jp][kp][7]=a11*PNQ[jp][kp][7]+a12*PNE[ip][kp][7]+a13*PNT[ip][jp][7];
	
	PNY[ip][jp][kp][0]=a21*PNQ[jp][kp][0]+a22*PNE[ip][kp][0]+a23*PNT[ip][jp][0];
	PNY[ip][jp][kp][1]=a21*PNQ[jp][kp][1]+a22*PNE[ip][kp][1]+a23*PNT[ip][jp][1];
	PNY[ip][jp][kp][2]=a21*PNQ[jp][kp][2]+a22*PNE[ip][kp][2]+a23*PNT[ip][jp][2];
	PNY[ip][jp][kp][3]=a21*PNQ[jp][kp][3]+a22*PNE[ip][kp][3]+a23*PNT[ip][jp][3];
	PNY[ip][jp][kp][4]=a21*PNQ[jp][kp][4]+a22*PNE[ip][kp][4]+a23*PNT[ip][jp][4];
	PNY[ip][jp][kp][5]=a21*PNQ[jp][kp][5]+a22*PNE[ip][kp][5]+a23*PNT[ip][jp][5];
	PNY[ip][jp][kp][6]=a21*PNQ[jp][kp][6]+a22*PNE[ip][kp][6]+a23*PNT[ip][jp][6];
	PNY[ip][jp][kp][7]=a21*PNQ[jp][kp][7]+a22*PNE[ip][kp][7]+a23*PNT[ip][jp][7];
	
	PNZ[ip][jp][kp][0]=a31*PNQ[jp][kp][0]+a32*PNE[ip][kp][0]+a33*PNT[ip][jp][0];
	PNZ[ip][jp][kp][1]=a31*PNQ[jp][kp][1]+a32*PNE[ip][kp][1]+a33*PNT[ip][jp][1];
	PNZ[ip][jp][kp][2]=a31*PNQ[jp][kp][2]+a32*PNE[ip][kp][2]+a33*PNT[ip][jp][2];
	PNZ[ip][jp][kp][3]=a31*PNQ[jp][kp][3]+a32*PNE[ip][kp][3]+a33*PNT[ip][jp][3];
	PNZ[ip][jp][kp][4]=a31*PNQ[jp][kp][4]+a32*PNE[ip][kp][4]+a33*PNT[ip][jp][4];
	PNZ[ip][jp][kp][5]=a31*PNQ[jp][kp][5]+a32*PNE[ip][kp][5]+a33*PNT[ip][jp][5];
	PNZ[ip][jp][kp][6]=a31*PNQ[jp][kp][6]+a32*PNE[ip][kp][6]+a33*PNT[ip][jp][6];
	PNZ[ip][jp][kp][7]=a31*PNQ[jp][kp][7]+a32*PNE[ip][kp][7]+a33*PNT[ip][jp][7];
      }
    }
  }
}

void MAT_ASS_MAIN()
{
  int i,k,kk;
  int ip,jp,kp;
  int ipn,jpn,kpn;
  int icel,icou,icol,icel0;
  int ie,je;
  int iiS,iiE;
  int in1,in2,in3,in4,in5,in6,in7,in8;
  int ip1,ip2,ip3,ip4,ip5,ip6,ip7,ip8,isum;
  KREAL SHi;
  KREAL QP1,QM1,EP1,EM1,TP1,TM1;
  KREAL X1,X2,X3,X4,X5,X6,X7,X8;
  KREAL Y1,Y2,Y3,Y4,Y5,Y6,Y7,Y8;
  KREAL Z1,Z2,Z3,Z4,Z5,Z6,Z7,Z8;
  KREAL PNXi,PNYi,PNZi,PNXj,PNYj,PNZj;
  KREAL QV0, QVC, COEFij;
  KREAL coef;

  KINT nodLOCAL[8];
  KINT *IWKX;
  KINT *ELMCOLORindex, *ELMCOLORitem;
  
/***
    ELEMENT Coloring
***/
  IWKX=(KINT*)malloc(sizeof(KINT)*(NP+1)*3);
  ELMCOLORindex=(KINT*)malloc(sizeof(KINT)*(NP+1));
  ELMCOLORitem =(KINT*)malloc(sizeof(KINT)*ICELTOT);

  ELMCOLORindex[0]=0;
  for(i=0;i<NP+1;i++) {
    IWKX[i*3+0]= 0;
    IWKX[i*3+1]= 0;
    IWKX[i*3+2]= 0;
  }
  icou=0;
  
  for(icol=1;icol<NP;icol++) {
    for(i=0;i<NP;i++) {
      IWKX[i*3+0]=0;
    }
    for(icel=0;icel<ICELTOT;icel++) {
    if (IWKX[icel*3+1]== 0){
      in1=ICELNOD[icel*8+0]-1;
      in2=ICELNOD[icel*8+1]-1;
      in3=ICELNOD[icel*8+2]-1;
      in4=ICELNOD[icel*8+3]-1;
      in5=ICELNOD[icel*8+4]-1;
      in6=ICELNOD[icel*8+5]-1;
      in7=ICELNOD[icel*8+6]-1;
      in8=ICELNOD[icel*8+7]-1;

      ip1=IWKX[in1*3+0];
      ip2=IWKX[in2*3+0];
      ip3=IWKX[in3*3+0];
      ip4=IWKX[in4*3+0];
      ip5=IWKX[in5*3+0];
      ip6=IWKX[in6*3+0];
      ip7=IWKX[in7*3+0];
      ip8=IWKX[in8*3+0];

      isum= ip1+ip2+ip3+ip4+ip5+ip6+ip7+ip8;

      if (isum==0) {
        IWKX[icol*3+2]= icou + 1;
        IWKX[icel*3+1]= icol;	
        ELMCOLORitem[icou]= icel;
        icou= icou + 1;

        IWKX[in1*3+0]= 1;
        IWKX[in2*3+0]= 1;
        IWKX[in3*3+0]= 1;
        IWKX[in4*3+0]= 1;
        IWKX[in5*3+0]= 1;
        IWKX[in6*3+0]= 1;
        IWKX[in7*3+0]= 1;
        IWKX[in8*3+0]= 1;	
        if (icou==ICELTOT) goto expoint;
      }
    }
    }   
  }

 expoint:  
  ELMCOLORtot= icol;
  IWKX[0*3+2]= 0;
  IWKX[ELMCOLORtot*3+2]= ICELTOT;

  for(icol=0;icol<ELMCOLORtot+1;icol++) {
    ELMCOLORindex[icol]= IWKX[icol*3+2];
  }
/***
    INIT.
    PNQ   - 1st-order derivative of shape function by QSI
    PNE   - 1st-order derivative of shape function by ETA
    PNT   - 1st-order derivative of shape function by ZET
***/
  
  AMAT=(KREAL*) malloc(sizeof(KREAL)*NPLU);
  B =(KREAL*) malloc(sizeof(KREAL)*NP);
  D =(KREAL*) malloc(sizeof(KREAL)*NP);
  X =(KREAL*) malloc(sizeof(KREAL)*NP);

#pragma acc parallel loop \
  private(i)
  for(i=0;i<NPLU;i++){
    AMAT[i]=0.0;
  }
#pragma acc parallel loop \
  private(i)
  for(i=0;i<NP;i++){
    B[i]=0.0;
    D[i]=0.0;
    X[i]=0.0;
  }

  WEI[0]= 1.0000000000e0;
  WEI[1]= 1.0000000000e0;
  
  POS[0]= -0.5773502692e0;
  POS[1]=  0.5773502692e0;

  for(ip=0;ip<2;ip++){
    for(jp=0;jp<2;jp++){
      for(kp=0;kp<2;kp++){
	QP1= 1.e0 + POS[ip];
	QM1= 1.e0 - POS[ip];
	EP1= 1.e0 + POS[jp];
	EM1= 1.e0 - POS[jp];
	TP1= 1.e0 + POS[kp];
	TM1= 1.e0 - POS[kp];
	SHAPE[ip][jp][kp][0]= O8th * QM1 * EM1 * TM1;
	SHAPE[ip][jp][kp][1]= O8th * QP1 * EM1 * TM1;
	SHAPE[ip][jp][kp][2]= O8th * QP1 * EP1 * TM1;
	SHAPE[ip][jp][kp][3]= O8th * QM1 * EP1 * TM1;
	SHAPE[ip][jp][kp][4]= O8th * QM1 * EM1 * TP1;
	SHAPE[ip][jp][kp][5]= O8th * QP1 * EM1 * TP1;
	SHAPE[ip][jp][kp][6]= O8th * QP1 * EP1 * TP1;
	SHAPE[ip][jp][kp][7]= O8th * QM1 * EP1 * TP1;
	PNQ[jp][kp][0]= - O8th * EM1 * TM1;
	PNQ[jp][kp][1]= + O8th * EM1 * TM1;
	PNQ[jp][kp][2]= + O8th * EP1 * TM1;
	PNQ[jp][kp][3]= - O8th * EP1 * TM1;
	PNQ[jp][kp][4]= - O8th * EM1 * TP1;
	PNQ[jp][kp][5]= + O8th * EM1 * TP1;
	PNQ[jp][kp][6]= + O8th * EP1 * TP1;
	PNQ[jp][kp][7]= - O8th * EP1 * TP1;
	PNE[ip][kp][0]= - O8th * QM1 * TM1;
	PNE[ip][kp][1]= - O8th * QP1 * TM1;
	PNE[ip][kp][2]= + O8th * QP1 * TM1;
	PNE[ip][kp][3]= + O8th * QM1 * TM1;
	PNE[ip][kp][4]= - O8th * QM1 * TP1;
	PNE[ip][kp][5]= - O8th * QP1 * TP1;
	PNE[ip][kp][6]= + O8th * QP1 * TP1;
	PNE[ip][kp][7]= + O8th * QM1 * TP1;
	PNT[ip][jp][0]= - O8th * QM1 * EM1;
	PNT[ip][jp][1]= - O8th * QP1 * EM1;
	PNT[ip][jp][2]= - O8th * QP1 * EP1;
	PNT[ip][jp][3]= - O8th * QM1 * EP1;
	PNT[ip][jp][4]= + O8th * QM1 * EM1;
	PNT[ip][jp][5]= + O8th * QP1 * EM1;
	PNT[ip][jp][6]= + O8th * QP1 * EP1;
	PNT[ip][jp][7]= + O8th * QM1 * EP1;
      }
    }
  }

  {
  
  for( icol=1; icol< ELMCOLORtot+1; icol++){
#pragma acc parallel loop		     \
  private(icel0,icel)			     \
  private(in1,in2,in3,in4,in5,in6,in7,in8)   \
  private(X1,X2,X3,X4,X5,X6,X7,X8)	     \
  private(Y1,Y2,Y3,Y4,Y5,Y6,Y7,Y8)	     \
  private(Z1,Z2,Z3,Z4,Z5,Z6,Z7,Z8)	     \
  private(QVC,DETJ,PNX,PNY,PNZ)		     \
  private(nodLOCAL)			     \
  private(ie,je,ip,jp,kk,iiS,iiE,k)	     \
  private(QV0,COEFij,ipn,jpn,kpn,coef)	     \
  private(PNXi,PNYi,PNZi,PNXj,PNYj,PNZj,SHi)
    for( icel0=ELMCOLORindex[icol-1]; icel0< ELMCOLORindex[icol]; icel0++){
      icel = ELMCOLORitem[icel0];
    
      in1=ICELNOD[icel*8+0];
      in2=ICELNOD[icel*8+1];
      in3=ICELNOD[icel*8+2];
      in4=ICELNOD[icel*8+3];
      in5=ICELNOD[icel*8+4];
      in6=ICELNOD[icel*8+5];
      in7=ICELNOD[icel*8+6];
      in8=ICELNOD[icel*8+7];
      /**
       **
       ** JACOBIAN & INVERSE JACOBIAN
       **/
      X1=XYZ[(in1-1)*3+0];
      X2=XYZ[(in2-1)*3+0];
      X3=XYZ[(in3-1)*3+0];
      X4=XYZ[(in4-1)*3+0];
      X5=XYZ[(in5-1)*3+0];
      X6=XYZ[(in6-1)*3+0];
      X7=XYZ[(in7-1)*3+0];
      X8=XYZ[(in8-1)*3+0];
    
      Y1=XYZ[(in1-1)*3+1];
      Y2=XYZ[(in2-1)*3+1];
      Y3=XYZ[(in3-1)*3+1];
      Y4=XYZ[(in4-1)*3+1];
      Y5=XYZ[(in5-1)*3+1];
      Y6=XYZ[(in6-1)*3+1];
      Y7=XYZ[(in7-1)*3+1];
      Y8=XYZ[(in8-1)*3+1];
    
      Z1=XYZ[(in1-1)*3+2];
      Z2=XYZ[(in2-1)*3+2];
      Z3=XYZ[(in3-1)*3+2];
      Z4=XYZ[(in4-1)*3+2];
      Z5=XYZ[(in5-1)*3+2];
      Z6=XYZ[(in6-1)*3+2];
      Z7=XYZ[(in7-1)*3+2];
      Z8=XYZ[(in8-1)*3+2];
    
      QVC= O8th*(X1+X2+X3+X4+X5+X6+X7+X8+Y1+Y2+Y3+Y4+Y5+Y6+Y7+Y8);
    
      JACOBI(DETJ, PNQ, PNE, PNT, PNX, PNY, PNZ,     
	     X1, X2, X3, X4, X5, X6, X7, X8,
	     Y1, Y2, Y3, Y4, Y5, Y6, Y7, Y8,
	     Z1, Z2, Z3, Z4, Z5, Z6, Z7, Z8);
    
      /**
	 CONSTRUCT the GLOBAL MATRIX
      **/
      nodLOCAL[0]= in1;
      nodLOCAL[1]= in2;
      nodLOCAL[2]= in3;
      nodLOCAL[3]= in4;
      nodLOCAL[4]= in5;
      nodLOCAL[5]= in6;
      nodLOCAL[6]= in7;
      nodLOCAL[7]= in8;
    
      for(ie=0;ie<8;ie++){
	ip=nodLOCAL[ie];
	for(je=0;je<8;je++){
	  jp=nodLOCAL[je];
	
	  kk=-1;
	  if( jp != ip ){
	    iiS=indexLU[ip-1];
	    iiE=indexLU[ip  ];
	    for( k=iiS;k<iiE;k++){
	      if( itemLU[k] == jp-1 ){
		kk=k;
		break;
	      }
	    }
	  }
	  QV0= 0.e0;
	  COEFij= 0.e0;
	
	  for(kpn=0;kpn<2;kpn++){
	    for(jpn=0;jpn<2;jpn++){
	      for(ipn=0;ipn<2;ipn++){
		coef= WEI[ipn]*WEI[jpn]*WEI[kpn];
	      
		PNXi= PNX[ipn][jpn][kpn][ie];
		PNYi= PNY[ipn][jpn][kpn][ie];
		PNZi= PNZ[ipn][jpn][kpn][ie];
	      
		PNXj= PNX[ipn][jpn][kpn][je];
		PNYj= PNY[ipn][jpn][kpn][je];
		PNZj= PNZ[ipn][jpn][kpn][je];

		COEFij+= coef*COND*(PNXi*PNXj+PNYi*PNYj+PNZi*PNZj)*
#if   FP ==  32
		  fabsf(DETJ[ipn][jpn][kpn]);
#elif FP ==  64
		  fabs (DETJ[ipn][jpn][kpn]);
#elif FP == 128
		  fabsl(DETJ[ipn][jpn][kpn]);
#endif
		
		SHi= SHAPE[ipn][jpn][kpn][ie];
		QV0+= SHi * QVOL * coef *
#if   FP ==  32
		  fabsf(DETJ[ipn][jpn][kpn]);
#elif FP ==  64
		  fabs (DETJ[ipn][jpn][kpn]);
#elif FP == 128
		  fabsl(DETJ[ipn][jpn][kpn]);
#endif
	      }
	    }
	  }
	
	  if (jp==ip) { 
	    D[ip-1]+= COEFij;
	    B[ip-1]+= QV0*QVC;
	  }
	  if (jp != ip) { 
	    AMAT[kk]+= COEFij;
	  }
	
	} // je
      } // ie
    } // icel0
    // pragma omp end parallel for
  } // icol

  } // pragma acc end data

  free(IWKX);
  free(ELMCOLORindex);
  free(ELMCOLORitem);
}  

void MAT_ASS_CLEAR()
{
  free(AMAT);
  free(B);
  free(D);
  free(X);
}
