#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "diffusion.h"
#include "misc.h"

static inline double wall_time(void) {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1.0e-6;
}

int main(int argc, char *argv[])
{
    const double real_start = wall_time();
    if (argc < 2) {
        fprintf(stderr, "ERROR: insufficient number of input parameters: %d (at least %d inputs are required)\n", argc, 2);
	fprintf(stderr, "Usage is: %s N\n", argv[0]);
	fprintf(stderr, "\tN: number of grid points (cubic: nx=ny=nz=N) <int>\n");
	exit(1);
    }

    const int nx = atoi(argv[1]);
    const int ny = nx;
    const int nz = nx;

    const int mgn = 1;
    const int lnx = nx;
    const int lny = ny;
    const int lnz = nz + 2* mgn;
    const int ln  = lnx * lny * lnz;

    const flt lx = 1.0F;
    const flt ly = 1.0F;
    const flt lz = 1.0F;

    const flt dx = lx / (flt)nx;
    const flt dy = ly / (flt)ny;
    const flt dz = lz / (flt)nz;

    const flt kappa = 0.1F;
    const flt dt    = 0.1F * fminf(fminf(dx * dx, dy * dy), dz * dz) / kappa;

#ifndef BENCHMARK_MODE
    const int nt = 100000;
#else
    const int nt = (int)(512.0*256.0*256.0/(nx*nx));
#endif
    double time = 0.0;
    int    icnt = 0;
    double flop = 0.0;
    double elapsed_time = 0.0;

    flt *f  = (flt *)malloc(sizeof(flt) * ln);
    flt *fn = (flt *)malloc(sizeof(flt) * ln);

    {
      init(nx, ny, nz, mgn, dx, dy, dz, f);

      start_timer();
    
      for (; icnt < nt; icnt++) {
#ifndef BENCHMARK_MODE
	if (icnt % 100 == 0){
	  fprintf(stdout, "time(%4d) = %7.5f\n", icnt, time);
	}
#endif
	flop += diffusion3d(nx, ny, nz, mgn, dx, dy, dz, dt, kappa, f, fn);

	swap(&f, &fn);

	time += dt;
#ifndef BENCHMARK_MODE
	if (time + 0.5 * dt > 0.1 ) break;
#endif
      }
      elapsed_time = get_elapsed_time();

      const double real_end = wall_time();
      const double real_sec = real_end - real_start;

      struct rusage usage;
      getrusage(RUSAGE_SELF, &usage);
      const double user_sec = (double)usage.ru_utime.tv_sec + (double)usage.ru_utime.tv_usec * 1.0e-6;
      const double sys_sec  = (double)usage.ru_stime.tv_sec + (double)usage.ru_stime.tv_usec * 1.0e-6;

      const long long N = (long long)nx * (long long)ny * (long long)nz;
      const double perf_gflops = flop / elapsed_time * 1.0e-9;

      const double ferr = err(time, nx, ny, nz, mgn, dx, dy, dz, kappa, f);
      const double faccuracy = sqrt(ferr / (double)(nx * ny * nz));

#ifndef BENCHMARK_MODE
      fprintf(stdout, "time(%4d) = %7.5f\n", icnt, time);
      fprintf(stdout, "Time = %8.3f [sec]\n", elapsed_time);
      fprintf(stdout, "Performance = %7.2f [GFlops]\n", perf_gflops);
      fprintf(stdout, "Error = %10.6e\n", faccuracy);
      fprintf(stdout, "Real = %8.3f [sec]   User = %8.3f [sec]   Sys = %8.3f [sec]\n",
              real_sec, user_sec, sys_sec);
      fprintf(stdout, "num_time_steps_logged = %d   last_sim_time = %10.6e\n", icnt, time);
#else
      /* CSV header (comment line, '#' で始まる; tools/parse は '#' をスキップ) */
      printf("# binary,N(=nx*ny*nz),time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time\n");
      printf("%s,%lld,%13.6e,%13.6e,%13.6e,%13.6e,%13.6e,%13.6e,%d,%13.6e\n",
             argv[0], N, elapsed_time, perf_gflops, faccuracy,
             real_sec, user_sec, sys_sec, icnt, time);
#endif
    }
    free(f);  f  = NULL;
    free(fn); fn = NULL;

    return 0;
}

