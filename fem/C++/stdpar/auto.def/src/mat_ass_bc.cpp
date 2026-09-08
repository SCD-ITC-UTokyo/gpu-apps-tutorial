/**
 ** MAT_ASS_BC
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pfem_util.h"

#include <numeric>   // std::transform_reduce
#include <iterator>  // std::begin, std::end
#include <algorithm> // std::for_each
#include <execution> // std::execution::par
#include <boost/iterator/counting_iterator.hpp> // boost::iterators::counting_iterator

extern FILE *fp_log;
void MAT_ASS_BC()
{
  /*
  int icel;
  int in1,in2,in3,in4,in5,in6,in7,in8;
  int iq1,iq2,iq3,iq4,iq5,iq6,iq7,iq8;
  int iS,iE;
  KREAL STRESS,VAL;
  */
  int i,j,ib0;
  KINT *IWKX;
  
  IWKX=(KINT*)malloc(sizeof(KINT)*NP*2);
  for(i=0;i<NP;i++) for(j=0;j<2;j++) IWKX[i*2+j]=0;

  /** stdpar GPU offload: local bindings of the namespace-scope objects **/
  KINT  *const NODGRP_ITEM= ::NODGRP_ITEM;
  KINT  *const indexLU    = ::indexLU;
  KINT  *const itemLU     = ::itemLU;
  KREAL *const AMAT       = ::AMAT;
  KREAL *const B          = ::B;
  KREAL *const D          = ::D;

  
/**
   Z=Zmax
**/
  std::for_each_n
  ( std::execution::par,
    boost::iterators::counting_iterator<int32_t>(0), NP,
    [=](int in) {
    IWKX[in*2+0]=0;
  });
  
  ib0=-1;
  for( ib0=0;ib0<NODGRPtot;ib0++){
    if( strcmp(NODGRP_NAME[ib0].name,"Zmax") == 0 ) break;
  }
  std::for_each_n
  ( std::execution::par,
    boost::iterators::counting_iterator<int32_t>(NODGRP_INDEX[ib0]),
    NODGRP_INDEX[ib0+1]-NODGRP_INDEX[ib0],
    [=](int ib) {
    int in=NODGRP_ITEM[ib];
    IWKX[(in-1)*2+0]=1;
  });

  std::for_each_n
  ( std::execution::par,
    boost::iterators::counting_iterator<int32_t>(0), NP,
    [=](int in) {
    if( IWKX[in*2+0] == 1 ){
      B[in]= 0.e0;
      D[in]= 1.e0;
      for(int k=indexLU[in];k<indexLU[in+1];k++){
	AMAT[k]= 0.e0;
      }
    }
  });

  std::for_each_n
  ( std::execution::par,
    boost::iterators::counting_iterator<int32_t>(0), NP,
    [=](int in) {
    for(int k=indexLU[in];k<indexLU[in+1];k++){
      if (IWKX[itemLU[k]*2+0] == 1 ) {
	AMAT[k]= 0.e0;
      }
    }
  });

  free(IWKX); 
}
