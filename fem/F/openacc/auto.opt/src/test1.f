      program heat3D

      use solver11
      use pfem_util
      implicit none

      character(len=1024) :: arg
#ifndef BENCHMARK_MODE
      integer(kind=kint) :: i
#endif
      REAL(dp) :: Stime, Etime
      REAL(dp) :: real_start, real_end, real_sec, user_sec, sys_sec
      REAL(dp) :: mat_sec, solver_sec, perf_gflops

      real_start = omp_get_wtime ()

      if( command_argument_count() < 1 ) then
         call get_command_argument(0,arg)
         write(*,'("Usage is: ",a," INPUT.DAT")') trim(arg)
         stop
      end if

      call get_command_argument(1,arg)
!C
!C +-------+
!C | INIT. |
!C +-------+
!C===
      call INPUT_CNTL( arg )
      call INPUT_GRID
!C===

!C
!C +---------------------+
!C | matrix connectivity |
!C +---------------------+
!C===
      call MAT_CON0
      call MAT_CON1
!C===

!C
!C +-----------------+
!C | MATRIX assemble |
!C +-----------------+
!C===
      Stime= omp_get_wtime ()
      call MAT_ASS_MAIN
      call MAT_ASS_BC
      Etime= omp_get_wtime ()
      mat_sec = Etime - Stime
#ifndef BENCHMARK_MODE
      write (*, '("*** matrix ass.  ", 1pe16.6, " sec.",/)') mat_sec
#endif
!C===

!C
!C +--------+
!C | SOLVER |
!C +--------+
!C===
      Stime= omp_get_wtime ()
      call SOLVE11
      Etime= omp_get_wtime ()
      solver_sec = Etime - Stime
#ifndef BENCHMARK_MODE
      write (*, '("*** solver       ", 1pe16.6, " sec.",/)') solver_sec
#else
      real_end = omp_get_wtime ()
      real_sec = real_end - real_start
      call cpu_time(user_sec)
      sys_sec  = 0.0_dp
      perf_gflops = FLOP / solver_sec * 1.0e-9_dp
!C 10 列 CSV (diffusion と同一スキーマ; 固定形式 Fortran で 72 列に収める)
      call get_command_argument(0,arg)
      write(*,'(a,a,a,a,a)')
     &  '# binary,NP,time_sec,performance_gflops,error,',
     &  'real_sec,user_sec,sys_sec,',
     &  'num_time_steps_logged,last_sim_time'
      write(*,1100) trim(arg), NP, solver_sec, perf_gflops, RESID,
     &              real_sec, user_sec, sys_sec, ITERactual, mat_sec
 1100 format(a,',',i8,',',1pe13.6,',',1pe13.6,',',1pe13.6,
     &       ',',1pe13.6,',',1pe13.6,',',1pe13.6,',',i8,
     &       ',',1pe13.6)
#endif
!C===

!C
!C +--------+
!C | OUTPUT |
!C +--------+
!C===
!      call OUTPUT_UCD
#ifndef BENCHMARK_MODE
      do i= 1, NP
        if (XYZ(i,1).eq.0.d0.and.XYZ(i,2).eq.0.d0
     &                      .and.XYZ(i,3).eq.0.d0) then
          write (*,'(i8,1pe16.6)') i, X(i)
        endif
      enddo
#endif
!C===

      call MAT_ASS_CLEAR
      end program heat3D
