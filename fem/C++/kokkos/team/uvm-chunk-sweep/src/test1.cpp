/**
	program heat3D
**/
#include <stdio.h>
#include <stdlib.h>
//#include <omp.h>
FILE* fp_log;
#define GLOBAL_VALUE_DEFINE
#include "pfem_util.h"
//#include "solver11.h"
extern void INPUT_CNTL();
extern void INPUT_GRID();
extern void MAT_CON0();
extern void MAT_CON1();
extern void MAT_ASS_MAIN();
extern void MAT_ASS_BC();
extern void SOLVE11();
extern void OUTPUT_UCD();
int main(int argc, char *argv[])
{
  Kokkos::ScopeGuard guard(argc, argv);

  int i;
  double Stime, Etime;
  Kokkos::Timer timer;
/** Logfile for debug **/
  if( (fp_log=fopen("log.log","w")) == NULL){
    fprintf(stdout,"input file cannot be opened!\n");
    exit(1);
  }

/**
   +-------+
   | INIT. |
   +-------+
**/ 
  INPUT_CNTL();
  INPUT_GRID();

/**
   +---------------------+
   | matrix connectivity |
   +---------------------+
**/
  MAT_CON0();
  MAT_CON1();
/**
   +-----------------+
   | MATRIX assemble |
   +-----------------+
**/
  timer.reset();
  MAT_ASS_MAIN();
  MAT_ASS_BC()  ;
  Etime= timer.seconds();
  fprintf(stdout,"*** matrix conn. %e sec.\n",Etime);
/**
   +--------+
   | SOLVER |
   +--------+
**/  
  timer.reset();
  SOLVE11();
  Etime= timer.seconds();
  fprintf(stdout,"*** solver       %e sec.\n",Etime);
/**
   +--------+
   | OUTPUT |
   +--------+
**/

  for(i=0;i<N;i++){
    if (XYZ_g(i,0)==0.e0) {
    if (XYZ_g(i,1)==0.e0) {
    if (XYZ_g(i,2)==0.e0) {
      printf("%8d%16.6e\n\n\n", i+1, X_g(i));}
    }}}

//  OUTPUT_UCD();

  // clear kokkos View
  view_clear();
}

      

