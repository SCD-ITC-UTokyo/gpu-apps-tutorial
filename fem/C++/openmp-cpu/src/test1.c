/**
	program heat3D (openmp-cpu baseline)
**/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <omp.h>
FILE* fp_log;
#define GLOBAL_VALUE_DEFINE
#include "pfem_util.h"

static inline double wall_time(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1.0e-6;
}

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
  const double real_start = wall_time();
#ifndef BENCHMARK_MODE
  int i;
#endif
  double Stime, Etime;
  double mat_sec = 0.0;
  double solver_sec = 0.0;

/** Logfile for debug **/
  if( (fp_log=fopen("log.log","w")) == NULL){
    fprintf(stdout,"input file cannot be opened!\n");
    exit(1);
  }

/** INIT **/
  INPUT_CNTL();
  INPUT_GRID();

/** matrix connectivity **/
  MAT_CON0();
  MAT_CON1();

/** MATRIX assemble **/
  Stime= omp_get_wtime();
  MAT_ASS_MAIN();
  MAT_ASS_BC();
  Etime= omp_get_wtime();
  mat_sec = Etime - Stime;
#ifndef BENCHMARK_MODE
  fprintf(stdout,"*** matrix conn. %e sec.\n", mat_sec);
#endif

/** SOLVER **/
  Stime= omp_get_wtime();
  SOLVE11();
  Etime= omp_get_wtime();
  solver_sec = Etime - Stime;
#ifndef BENCHMARK_MODE
  fprintf(stdout,"*** solver       %e sec.\n", solver_sec);
#else
  {
    const double real_end = wall_time();
    const double real_sec = real_end - real_start;
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    const double user_sec = (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec * 1.0e-6;
    const double sys_sec  = (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec * 1.0e-6;
    const double perf_gflops = FLOP / solver_sec * 1.0e-9;
    /* 10 列 CSV: openmp-cpu 旧コードは N (= NP に相当) を使用 */
    printf("# binary,NP,time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time\n");
    printf("%s,%d,%13.6e,%13.6e,%13.6e,%13.6e,%13.6e,%13.6e,%d,%13.6e\n",
           argv[0], N, solver_sec, perf_gflops, RESID,
           real_sec, user_sec, sys_sec, ITERactual, mat_sec);
  }
#endif

#ifndef BENCHMARK_MODE
  for(i=0;i<N;i++){
    if (XYZ[i][0]==0.e0) {
    if (XYZ[i][1]==0.e0) {
    if (XYZ[i][2]==0.e0) {
      printf("%8d%16.6e\n\n\n", i+1, X[i]);}
    }}}
#endif
}
