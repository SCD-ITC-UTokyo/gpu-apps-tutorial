program main
  use diffusion
  use misc
  implicit none

  integer  :: nx, ny, nz, n, nt, icnt
  real(fp) :: lx, ly, lz, dx, dy, dz, dt, kappa

  real(dp) :: time, flop, elapsed_time, ferr, ferr_sum, faccuracy
  real(fp),pointer,dimension(:,:,:) :: f,fn
  character(len=1024) :: arg, filename
  real(dp) :: real_sec, user_sec, sys_sec
  integer(int64) :: ln_N

  if( command_argument_count() < 1 ) then
     write(*,'("ERROR: insufficient number of input parameters: ", i1)') &
          command_argument_count()     
     write(*,'(" (at least ",i1," inputs are required)")') 1
          
     call get_command_argument(0,arg)
     write(*,'("Usage is: ",a," N")') trim(arg)
     write(*,'("    N: number of grid points (cubic: nx=ny=nz=N) <integer>")')
     stop
  end if

  call get_command_argument(1,arg)
  read(arg,*) nx
  ny = nx
  nz = nx
  n  = nx * ny * nz
  
  lx = real(1.0,fp)
  ly = real(1.0,fp)
  lz = real(1.0,fp)

  dx = lx/real(nx,fp)
  dy = lx/real(ny,fp)
  dz = lx/real(nz,fp)

  kappa = real(0.1,fp)
  dt = real(0.1,fp) * min(min(dx * dx, dy * dy), dz * dz) / kappa
#ifndef BENCHMARK_MODE
  nt = 100000
#else
  nt = int(512.0*256.0*256.0/(nx*nx))
#endif

  time = 0.d0
  flop = 0.d0
  elapsed_time = 0.d0

  allocate(f(nx,ny,0:nz+1))
  allocate(fn(nx,ny,0:nz+1))
  !$omp target data map(alloc: f(1:nx,1:ny,0:nz+1),fn(1:nx,1:ny,0:nz+1))

  call init(nx, ny, nz, dx, dy, dz, f);
  
  call start_real_timer()
  call start_timer()

  do icnt = 0, nt-1
#ifndef BENCHMARK_MODE
     if(mod(icnt,nt/16) == 0) write (*,"(A5,I4,A4,F7.5)"), "time(",icnt,") = ",time
#endif
     flop = flop + diffusion3d(nx, ny, nz, dx, dy, dz, dt, kappa, f, fn)

     call swap(f, fn)

     time = time + dt
#ifndef BENCHMARK_MODE
     if(time + 0.5*dt >= 0.1) exit
#endif
  end do
  elapsed_time = get_elapsed_time();
  
  ferr = err(time, nx, ny, nz, dx, dy, dz, kappa, f)
  faccuracy = sqrt(ferr/dble(nx*ny*nz))
  
#ifndef BENCHMARK_MODE
  write (*,"(A5,I4,A4,F7.5)"), "time(",icnt,") = ",time
  write(*, "(A7,F8.3,A6)"), "Time = ",elapsed_time," [sec]"
  write(*, "(A13,F7.2,A16)"), "Performance = ",flop/elapsed_time*1.0e-09," [GFlops]"
  write(*, "(A6,ES13.6)"), "Error = ",faccuracy
  real_sec = get_real_time()
  user_sec = get_cpu_time_used()
  write(*, "(A7,F8.3,A12,F8.3,A12,F8.3,A6)"), "Real = ",real_sec,"   User = ",user_sec,"   Sys = ",0.0_dp," [sec]"
  ln_N = int(nx, int64) * int(ny, int64) * int(nz, int64)
  write(*, "(A24,I4,A20,ES13.6)"), "num_time_steps_logged = ",icnt,"   last_sim_time = ",time
#else
  call get_command_argument(0,arg)
  real_sec = get_real_time()
  user_sec = get_cpu_time_used()
  sys_sec  = 0.0_dp
  ln_N = int(nx, int64) * int(ny, int64) * int(nz, int64)
  ! CSV header (comment line, '#' で始まる)
  write(*,'("# binary,N(=nx*ny*nz),time_sec,performance_gflops,error,real_sec,user_sec,sys_sec,num_time_steps_logged,last_sim_time")')
  write(*,"(a,',',i13,',',es13.6,',',es13.6,',',es13.6,',',es13.6,',',es13.6,',',es13.6,',',i13,',',es13.6)") &
       trim(arg), ln_N, elapsed_time, flop/elapsed_time*1.0e-9, faccuracy, &
       real_sec, user_sec, sys_sec, icnt, time
#endif
  !$omp end target data
  deallocate(f,fn)
end program main
