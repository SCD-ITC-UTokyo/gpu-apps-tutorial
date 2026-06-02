/**
 ** MAT_ASS_MAIN
 **/
#include <stdio.h>
#include <math.h>
#include "pfem_util.h"
#include "allocate.h"
#include "inline_func.h"
extern FILE *fp_log;
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
  double SHi;
  double QP1,QM1,EP1,EM1,TP1,TM1;
  double X1,X2,X3,X4,X5,X6,X7,X8;
  double Y1,Y2,Y3,Y4,Y5,Y6,Y7,Y8;
  double Z1,Z2,Z3,Z4,Z5,Z6,Z7,Z8;
  double PNXi,PNYi,PNZi,PNXj,PNYj,PNZj;
  double COND0, QV0, QVC, COEFij;
  double coef;

  KINT nodLOCAL[8];
  
  AMAT_g=View1D<KREAL>("AMAT",NPLU); //device
  B_g =View1D<KREAL>("B",NP); //device
  D_g =View1D<KREAL>("D",NP); //device
  X_g =View1D<KREAL>("X",NP); //device
  
  Kokkos::deep_copy(AMAT_g,0.0);
  Kokkos::deep_copy(B_g,0.0);
  Kokkos::deep_copy(D_g,0.0);
  Kokkos::deep_copy(X_g,0.0);
  
  View1D<KREAL> WEI_d("WEI",2); //device
  auto WEI_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, WEI_d); //host(mirror)
  WEI_h(0)= 1.0000000000e0;
  WEI_h(1)= 1.0000000000e0;
  Kokkos::deep_copy(WEI_d,WEI_h);

  POS[0]= -0.5773502692e0;
  POS[1]=  0.5773502692e0;
  
/***
    ELEMENT Coloring
***/
  IWKX=(KINT**)allocate_matrix(sizeof(KINT),NP+1,3);
  ELMCOLORindex=(KINT*)allocate_vector(sizeof(KINT),NP+1);
  ELMCOLORitem_g =View1D<KINT>("ELMCOLORitem",ICELTOT); //device
  auto ELMCOLORitem_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, ELMCOLORitem_g); //host(mirror)

  ELMCOLORindex[0]=0;
  for(i=0;i<NP+1;i++) {
    IWKX[i][0]= 0;
    IWKX[i][1]= 0;
    IWKX[i][2]= 0;
  }
  icou=0;
  
  auto ICELNOD_h=Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{}, ICELNOD_g); //host(mirror)

  for(icol=1;icol<NP;icol++) {
    for(i=0;i<NP;i++) {
      IWKX[i][0]=0;
    }
    for(icel=0;icel<ICELTOT;icel++) {
    if (IWKX[icel][1]== 0){
      in1=ICELNOD_h(icel,0)-1;
      in2=ICELNOD_h(icel,1)-1;
      in3=ICELNOD_h(icel,2)-1;
      in4=ICELNOD_h(icel,3)-1;
      in5=ICELNOD_h(icel,4)-1;
      in6=ICELNOD_h(icel,5)-1;
      in7=ICELNOD_h(icel,6)-1;
      in8=ICELNOD_h(icel,7)-1;

      ip1=IWKX[in1][0];
      ip2=IWKX[in2][0];
      ip3=IWKX[in3][0];
      ip4=IWKX[in4][0];
      ip5=IWKX[in5][0];
      ip6=IWKX[in6][0];
      ip7=IWKX[in7][0];
      ip8=IWKX[in8][0];

      isum= ip1+ip2+ip3+ip4+ip5+ip6+ip7+ip8;

      if (isum==0) {
        IWKX[icol][2]= icou + 1;
        IWKX[icel][1]= icol;	
        ELMCOLORitem_h(icou)= icel;
        icou= icou + 1;

        IWKX[in1][0]= 1;
        IWKX[in2][0]= 1;
        IWKX[in3][0]= 1;
        IWKX[in4][0]= 1;
        IWKX[in5][0]= 1;
        IWKX[in6][0]= 1;
        IWKX[in7][0]= 1;
        IWKX[in8][0]= 1;	
        if (icou==ICELTOT) goto expoint;
      }
    }
    }   
  }

 expoint:  
  ELMCOLORtot= icol;
  IWKX[0][2]= 0;
  IWKX[ELMCOLORtot][2]= ICELTOT;

  for(icol=0;icol<ELMCOLORtot+1;icol++) {
    ELMCOLORindex[icol]= IWKX[icol][2];
  }
  deallocate_vector(IWKX);
  Kokkos::deep_copy(ELMCOLORitem_g,ELMCOLORitem_h); //host to device

/***
    INIT.
    PNQ   - 1st-order derivative of shape function by QSI
    PNE   - 1st-order derivative of shape function by ETA
    PNT   - 1st-order derivative of shape function by ZET
***/
  View3D<KREAL> PNQ_d("PNQ",2,2,8); //device
  View3D<KREAL> PNE_d("PNE",2,2,8); //device
  View3D<KREAL> PNT_d("PNT",2,2,8); //device
  View4D<KREAL> SHAPE_d("SHAPE",2,2,2,8); //device
  auto PNQ_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, PNQ_d); //host(mirror)
  auto PNE_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, PNE_d); //host(mirror)
  auto PNT_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, PNT_d); //host(mirror)
  auto SHAPE_h=Kokkos::create_mirror_view(Kokkos::HostSpace{}, SHAPE_d); //host(mirror)
  for(ip=0;ip<2;ip++){
    for(jp=0;jp<2;jp++){
      for(kp=0;kp<2;kp++){
	QP1= 1.e0 + POS[ip];
	QM1= 1.e0 - POS[ip];
	EP1= 1.e0 + POS[jp];
	EM1= 1.e0 - POS[jp];
	TP1= 1.e0 + POS[kp];
	TM1= 1.e0 - POS[kp];
	SHAPE_h(ip,jp,kp,0)= O8th * QM1 * EM1 * TM1;
	SHAPE_h(ip,jp,kp,1)= O8th * QP1 * EM1 * TM1;
	SHAPE_h(ip,jp,kp,2)= O8th * QP1 * EP1 * TM1;
	SHAPE_h(ip,jp,kp,3)= O8th * QM1 * EP1 * TM1;
	SHAPE_h(ip,jp,kp,4)= O8th * QM1 * EM1 * TP1;
	SHAPE_h(ip,jp,kp,5)= O8th * QP1 * EM1 * TP1;
	SHAPE_h(ip,jp,kp,6)= O8th * QP1 * EP1 * TP1;
	SHAPE_h(ip,jp,kp,7)= O8th * QM1 * EP1 * TP1;
	PNQ_h(jp,kp,0)= - O8th * EM1 * TM1;
	PNQ_h(jp,kp,1)= + O8th * EM1 * TM1;
	PNQ_h(jp,kp,2)= + O8th * EP1 * TM1;
	PNQ_h(jp,kp,3)= - O8th * EP1 * TM1;
	PNQ_h(jp,kp,4)= - O8th * EM1 * TP1;
	PNQ_h(jp,kp,5)= + O8th * EM1 * TP1;
	PNQ_h(jp,kp,6)= + O8th * EP1 * TP1;
	PNQ_h(jp,kp,7)= - O8th * EP1 * TP1;
	PNE_h(ip,kp,0)= - O8th * QM1 * TM1;
	PNE_h(ip,kp,1)= - O8th * QP1 * TM1;
	PNE_h(ip,kp,2)= + O8th * QP1 * TM1;
	PNE_h(ip,kp,3)= + O8th * QM1 * TM1;
	PNE_h(ip,kp,4)= - O8th * QM1 * TP1;
	PNE_h(ip,kp,5)= - O8th * QP1 * TP1;
	PNE_h(ip,kp,6)= + O8th * QP1 * TP1;
	PNE_h(ip,kp,7)= + O8th * QM1 * TP1;
	PNT_h(ip,jp,0)= - O8th * QM1 * EM1;
	PNT_h(ip,jp,1)= - O8th * QP1 * EM1;
	PNT_h(ip,jp,2)= - O8th * QP1 * EP1;
	PNT_h(ip,jp,3)= - O8th * QM1 * EP1;
	PNT_h(ip,jp,4)= + O8th * QM1 * EM1;
	PNT_h(ip,jp,5)= + O8th * QP1 * EM1;
	PNT_h(ip,jp,6)= + O8th * QP1 * EP1;
	PNT_h(ip,jp,7)= + O8th * QM1 * EP1;
      }
    }
  }
  Kokkos::deep_copy(PNQ_d,PNQ_h); //host to device
  Kokkos::deep_copy(PNE_d,PNE_h); //host to device
  Kokkos::deep_copy(PNT_d,PNT_h); //host to device
  Kokkos::deep_copy(SHAPE_d,SHAPE_h); //host to device

  // local copy (for KOKKOS_LAMBDA capture)
  KREAL COND_l=COND;
  KREAL O8th_l=O8th;
  KREAL QVOL_l=QVOL;
  auto ICELNOD_d=ICELNOD_g;
  auto ELMCOLORitem_d=ELMCOLORitem_g;
  auto XYZ_d=XYZ_g;
  auto indexLU_d=indexLU_g;
  auto itemLU_d=itemLU_g;
  auto AMAT_d=AMAT_g;
  auto D_d=D_g;
  auto B_d=B_g;

  for( icol=1; icol< ELMCOLORtot+1; icol++){
    //TeamPolicy
    int nn=ELMCOLORindex[icol]-ELMCOLORindex[icol-1];
    int ioff=ELMCOLORindex[icol-1];
    const int chunk = CHUNK_SIZE_ASS; //chunk size
    const int league_size = (nn + chunk - 1) / chunk; //num of teams
    team_policy policy_t(league_size, Kokkos::AUTO());
  Kokkos::parallel_for("mat_ass_main", policy_t, KOKKOS_LAMBDA(const member_type& team){
    const int begin = team.league_rank() * chunk;
    const int end   = (begin + chunk < nn) ? (begin + chunk) : nn;
  Kokkos::parallel_for(Kokkos::TeamThreadRange(team, begin+ioff, end+ioff), [&] (const int icel0){
    // thread local variables
    int i,j,k,kk;
    int ip,jp;
    int ipn,jpn,kpn;
    int icel;
    int ie,je;
    int iiS,iiE;
    int in1,in2,in3,in4,in5,in6,in7,in8;
    double SHi;
    double X1,X2,X3,X4,X5,X6,X7,X8;
    double Y1,Y2,Y3,Y4,Y5,Y6,Y7,Y8;
    double Z1,Z2,Z3,Z4,Z5,Z6,Z7,Z8;
    double PNXi,PNYi,PNZi,PNXj,PNYj,PNZj;
    double COND0, QV0, QVC, COEFij;
    double coef;
    KINT nodLOCAL[8];
    KREAL DETJ[2][2][2], PNX[2][2][2][8], PNY[2][2][2][8], PNZ[2][2][2][8];

    icel = ELMCOLORitem_d(icel0);
    COND0= COND_l;
    
    in1=ICELNOD_d(icel,0);
    in2=ICELNOD_d(icel,1);
    in3=ICELNOD_d(icel,2);
    in4=ICELNOD_d(icel,3);
    in5=ICELNOD_d(icel,4);
    in6=ICELNOD_d(icel,5);
    in7=ICELNOD_d(icel,6);
    in8=ICELNOD_d(icel,7);
/**
 **
 ** JACOBIAN & INVERSE JACOBIAN
**/
    nodLOCAL[0]= in1;
    nodLOCAL[1]= in2;
    nodLOCAL[2]= in3;
    nodLOCAL[3]= in4;
    nodLOCAL[4]= in5;
    nodLOCAL[5]= in6;
    nodLOCAL[6]= in7;
    nodLOCAL[7]= in8;
    
    X1=XYZ_d(in1-1,0);
    X2=XYZ_d(in2-1,0);
    X3=XYZ_d(in3-1,0);
    X4=XYZ_d(in4-1,0);
    X5=XYZ_d(in5-1,0);
    X6=XYZ_d(in6-1,0);
    X7=XYZ_d(in7-1,0);
    X8=XYZ_d(in8-1,0);
    
    Y1=XYZ_d(in1-1,1);
    Y2=XYZ_d(in2-1,1);
    Y3=XYZ_d(in3-1,1);
    Y4=XYZ_d(in4-1,1);
    Y5=XYZ_d(in5-1,1);
    Y6=XYZ_d(in6-1,1);
    Y7=XYZ_d(in7-1,1);
    Y8=XYZ_d(in8-1,1);
    
    QVC= O8th_l*(X1+X2+X3+X4+X5+X6+X7+X8+Y1+Y2+Y3+Y4+Y5+Y6+Y7+Y8);
    
    Z1=XYZ_d(in1-1,2);
    Z2=XYZ_d(in2-1,2);
    Z3=XYZ_d(in3-1,2);
    Z4=XYZ_d(in4-1,2);
    Z5=XYZ_d(in5-1,2);
    Z6=XYZ_d(in6-1,2);
    Z7=XYZ_d(in7-1,2);
    Z8=XYZ_d(in8-1,2);

    JACOBI(DETJ, PNQ_d, PNE_d, PNT_d, PNX, PNY, PNZ,     
	   X1, X2, X3, X4, X5, X6, X7, X8,
	   Y1, Y2, Y3, Y4, Y5, Y6, Y7, Y8,
	   Z1, Z2, Z3, Z4, Z5, Z6, Z7, Z8);

/**
   CONSTRUCT the GLOBAL MATRIX
**/
    for(ie=0;ie<8;ie++){
      ip=nodLOCAL[ie];
      for(je=0;je<8;je++){
	jp=nodLOCAL[je];
	
	kk=-1;
	if( jp != ip ){
	  iiS=indexLU_d(ip-1);
	  iiE=indexLU_d(ip  );
	  for( k=iiS;k<iiE;k++){
	    if( itemLU_d(k) == jp-1 ){
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
	      coef= WEI_d(ipn)*WEI_d(jpn)*WEI_d(kpn);
	      
	      PNXi= PNX[ipn][jpn][kpn][ie];
	      PNYi= PNY[ipn][jpn][kpn][ie];
	      PNZi= PNZ[ipn][jpn][kpn][ie];
	      
	      PNXj= PNX[ipn][jpn][kpn][je];
	      PNYj= PNY[ipn][jpn][kpn][je];
	      PNZj= PNZ[ipn][jpn][kpn][je];
	      
	      COEFij+= coef*COND0*(PNXi*PNXj+PNYi*PNYj+PNZi*PNZj)*fabs(DETJ[ipn][jpn][kpn]);

	      SHi= SHAPE_d(ipn,jpn,kpn,ie);
	      QV0+= SHi * QVOL_l * coef * fabs(DETJ[ipn][jpn][kpn]);
	    }
	  }
	}
	
	if (jp==ip) { 
	  D_d(ip-1)+= COEFij;
	  B_d(ip-1)+= QV0*QVC;
	}
	if (jp != ip) { 
	  AMAT_d(kk)+= COEFij;
	}
	
      }
    }
  });
  });
  Kokkos::fence();
}
}  

