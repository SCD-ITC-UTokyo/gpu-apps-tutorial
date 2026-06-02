!!!
!!! @file nbody.f90
!!! @author Yohei MIKI (Information Technology Center, The University of Tokyo)
!!! @brief sample implementation of direct N-body simulation (orbit integration: 2nd-order leapfrog scheme)
!!!
!!! @copyright Copyright (c) 2022 Yohei MIKI
!!!
!!! The MIT License is applied to this software, see LICENSE.txt
!!!
!!! Code type: D2
!!!  - Programming Language: F90
!!!  - GPU Acceleration: OpenACC
!!!  - Memory Management: Auto(controlled by hardware or runtime)
!!!  - Optimization: -Mfprelaxed=rsqrt vector_length(128)

module nbody
  use cfg
  use io
  use conservatives
  use init
  use util_hdf5
  use util_timer
  implicit none

  real(flt_acc), parameter :: newton = 1.0  !! gravitational constant

contains
!!!
!!! @brief calculate gravitational acceleration
!!!
!!! @param[in] Ni number of i-particles
!!! @param[in] ipos position of i-particles
!!! @param[out] iacc acceleration of i-particles
!!! @param[in] Nj number of j-particles
!!! @param[in] jpos position of j-particles
!!! @param[in] eps2 square of softening length
!!! @param[in] ivel velocity of i-particles
!!!
  subroutine calc_acc( Ni, ipos, iacc, Nj, jpos, eps2 )
    integer, intent(in) :: Ni, Nj
    type(position), intent(in) :: ipos(Ni),  jpos(Ni)
    type(acceleration), intent(out) :: iacc(Ni)
    real(flt_pos), intent(in) :: eps2

    integer :: i, j
    type(position) :: pi, pj
    type(acceleration) :: ai
    real(flt_pos) :: dx, dy, dz, r2
    real(fp_l) :: r_inv, r2_inv
    real(flt_acc) :: alp

!$acc kernels vector_length(NTHREADS)
!$acc loop independent &
!$acc private(i,pi,ai,j,pj,dx,dy,dz,r2,r_inv,r2_inv,alp)
    do i=1, Ni
       !! initialization
       pi = ipos(i)
       ai = acceleration( 0.0, 0.0, 0.0, 0.0 )

       !! force evaluation
!$acc loop seq
       do j=1, Nj
          !! load j-particle
          pj = jpos(j)

          !! calculate particle-particle interaction
          dx = pj%x - pi%x
          dy = pj%y - pi%y
          dz = pj%z - pi%z
          r2 = eps2 + dx * dx + dy * dy + dz * dz
          r_inv  = real(1.0,fp_l) / sqrt(real(r2,fp_l))
          r2_inv = r_inv * r_inv
          alp = pj%w * real(r_inv*r2_inv,flt_acc)

          !! force accumulation
          ai%x = ai%x + alp * dx
          ai%y = ai%y + alp * dy
          ai%z = ai%z + alp * dz

#ifdef CALCULATE_POTENTIAL
          !! gravitational potential
          ai%w = ai%w + alp * r2
#endif  !! CALCULATE_POTENTIAL
       end do
       iacc(i) = ai
    end do
    !$acc end kernels
  end subroutine calc_acc


#ifndef BENCHMARK_MODE
!!!
!!! @brief finalize calculation of gravitational acceleration
!!!
!!! @param[in] Ni number of i-particles
!!! @param[in,out] acc acceleration of i-particles
!!! @param[in] pos position of i-particles
!!! @param[in] eps_inv inverse of softening length
!!!
  subroutine trim_acc( &
#ifdef CALCULATE_POTENTIAL
       Ni, acc, pos, eps_inv &
#else
       Ni, acc &
#endif !! CALCULATE_POTENTIAL
       )
    integer, intent(in) :: Ni
    type(acceleration), intent(inout) :: acc(Ni)
#ifdef CALCULATE_POTENTIAL
    type(position), intent(in) :: pos(Ni)
    real(flt_acc),  intent(in) :: eps_inv
#endif !! CALCULATE_POTENTIAL

    integer :: i
    type(acceleration) :: ai

!$acc kernels vector_length(NTHREADS)
!$acc loop independent &
!$acc private(i,ai)
    do i=1, Ni
       !! initialization
       ai = acc(i)

       ai%x = ai%x * newton
       ai%y = ai%y * newton
       ai%z = ai%z * newton

#ifdef CALCULATE_POTENTIAL
       ai%w = (-ai%w + eps_inv * real(pos(i)%w,flt_acc)) * newton
#endif  !! CALCULATE_POTENTIAL

       acc(i) = ai
    end do
    !$acc end kernels
  end subroutine trim_acc

!!!
!!! @brief integrate velocity
!!!
!!! @param[in] num number of N-body particles
!!! @param[in,out] vel velocity of N-body particles
!!! @param[in] acc acceleration of N-body particles
!!! @param[in] dt time step
!!!
  subroutine kick( num, vel, acc, dt )
    integer, intent(in) :: num
    type(velocity),     intent(inout) :: vel(num)
    type(acceleration), intent(in)    :: acc(num)
    real(flt_vel),      intent(in)    :: dt

    integer :: i
    type(velocity) :: vi
    type(acceleration) :: ai

!$acc kernels vector_length(NTHREADS)
!$acc loop independent &
!$acc private(i,vi,ai)
    do i=1, num
       !! initialization
       vi = vel(i)
       ai = acc(i)

       vi%x = vi%x + dt * real(ai%x,flt_vel)
       vi%y = vi%y + dt * real(ai%y,flt_vel)
       vi%z = vi%z + dt * real(ai%z,flt_vel)

       vel(i) = vi
    end do
    !$acc end kernels
  end subroutine kick

!!!
!!! @brief integrate position
!!!
!!! @param[in] num number of N-body particles
!!! @param[in,out] pos position of N-body particles
!!! @param[in] vel velocity of N-body particles
!!! @param[in] dt time step
!!!
  subroutine drift( num, pos, vel, dt )
    integer, intent(in) :: num
    type(position), intent(inout) :: pos(num)
    type(velocity), intent(in)    :: vel(num)
    real(flt_pos),  intent(in)    :: dt

    integer :: i
    type(position) :: pi
    type(velocity) :: vi

!$acc kernels vector_length(NTHREADS)
!$acc loop independent &
!$acc private(i,pi,vi)
    do i=1, num
       !! initialization
       pi = pos(i)
       vi = vel(i)

       pi%x = pi%x + dt * real(vi%x,flt_pos)
       pi%y = pi%y + dt * real(vi%y,flt_pos)
       pi%z = pi%z + dt * real(vi%z,flt_pos)

       pos(i) = pi
    end do
    !$acc end kernels
  end subroutine drift

!!!
!!! @brief integrate velocity in half-step backward
!!!
!!! @param[in] num number of N-body particles
!!! @param[in] vel_src velocity of N-body particles
!!! @param[in] acc acceleration of N-body particles
!!! @param[out] vel velocity of N-body particles
!!! @param[in] dt time step
!!!
  subroutine kick_backward_half( num, vel_src, acc, vel, dt )
    integer, intent(in) :: num
    type(velocity),     intent(in)    :: vel_src(num)
    type(acceleration), intent(in)    :: acc(num)
    type(velocity),     intent(out)   :: vel(num)
    real(flt_vel), intent(in) :: dt

    real(flt_vel) :: dt_2
    integer :: i
    type(velocity) :: vi
    type(acceleration) :: ai

    dt_2 = real(0.5,flt_vel)*dt

!$acc kernels vector_length(NTHREADS)
!$acc loop independent &
!$acc private(i,vi,ai)
    do i=1, num
       !! initialization
       vi = vel_src(i)
       ai = acc(i)

       vi%x = vi%x - dt_2 * real(ai%x,flt_vel)
       vi%y = vi%y - dt_2 * real(ai%y,flt_vel)
       vi%z = vi%z - dt_2 * real(ai%z,flt_vel)

       vel(i) = vi
    end do
    !$acc end kernels
  end subroutine kick_backward_half
#endif  !! BENCHMARK_MODE

!!!
!!! @brief allocate memory for N-body particles
!!!
!!! @param[out] pos position of N-body particles
!!! @param[out] vel velocity of N-body particles
!!! @param[out] vel_tmp tentative array for velocity of N-body particles
!!! @param[out] acc acceleration of N-body particles
!!! @param[in] num number of N-body particles
!!!
  subroutine allocate_Nbody_particles( pos, vel, vel_tmp, acc, num )
    type(position), pointer, intent(inout) :: pos(:)
    type(velocity), pointer, intent(inout) :: vel(:)
    type(velocity), pointer, intent(inout) :: vel_tmp(:)
    type(acceleration), pointer, intent(inout) :: acc(:)
    integer, intent(in) :: num

    !! zero-clear arrays (for safety of massless particles)
    type(position), parameter :: p_zero = position( 0.0, 0.0, 0.0, 0.0 )
    type(velocity), parameter :: v_zero = velocity( 0.0, 0.0, 0.0, 0.0 )
    type(acceleration), parameter :: a_zero = acceleration( 0.0, 0.0, 0.0, 0.0 )
    integer :: i

    allocate( pos(num), vel(num), vel_tmp(num), acc(num) )

    !$omp parallel do private(i)
    do i=1, num
       pos(i) = p_zero
       vel(i) = v_zero
       vel_tmp(i) = v_zero
       acc(i) = a_zero
    end do
  end subroutine allocate_Nbody_particles

!!!
!!! @brief deallocate memory for N-body particles
!!!
!!! @param[out] pos position of N-body particles
!!! @param[out] vel velocity of N-body particles
!!! @param[out] vel_tmp tentative array for velocity of N-body particles
!!! @param[out] acc acceleration of N-body particles
!!!
  subroutine release_Nbody_particles( pos, vel, vel_tmp, acc )
    type(position), pointer, intent(inout) :: pos(:)
    type(velocity), pointer, intent(inout) :: vel(:)
    type(velocity), pointer, intent(inout) :: vel_tmp(:)
    type(acceleration), pointer, intent(inout) :: acc(:)

    deallocate( pos, vel, vel_tmp, acc )
  end subroutine release_Nbody_particles
end module nbody

program main
  use nbody
  implicit none

  type(c_ptr) :: time_to_solution, config, error, timer

  integer :: argc
  integer, parameter :: arglen = 256
  character(len=arglen), allocatable :: argv(:)

  character(len=32) :: file
  real(flt_pos) :: eps, M_tot, rad, eps2
  real(flt_vel) :: virial
#ifdef BENCHMARK_MODE
  real(dp) :: num_min, num_max, num_logbin
#else
  real(fp_m) :: ft, snapshot_interval, dt
#endif
  integer  :: num, num_bin
  real(fp_m) :: time, time_from_snapshot
  integer :: step, present, previous, snp_fin
#ifdef CALCULATE_POTENTIAL
  real(fp_m) :: eps_inv
#endif  !! CALCULATE_POTENTIAL

  type(position), pointer :: pos(:)
  type(velocity), pointer :: vel(:)
  type(velocity), pointer :: vel_tmp(:)
  type(acceleration), pointer :: acc(:)

  real(dp) :: elapsed, minimum_elapsed
  integer :: iter, loop, i
  real(dp), parameter :: booster = 1.25 !! additional safety-parameter to reduce rejection rate

  !! record time-to-solution
  time_to_solution = util_timer_constructor()
  call util_timer_start( time_to_solution )

  !! initialize the simulation
  config = config_constructor()
  
  !! get argc and gather argv
  argc = command_argument_count()
  allocate( argv(0:argc) )
  do i=0, argc
     call get_command_argument(i,argv(i))
  end do
  call config_configure( config, argc, argv, arglen )

  !! read input parameters
  call config_get_file( config, file, 32 )
  call config_get_eps( config, eps )
  call config_get_mass( config, M_tot )
  call config_get_radius( config, rad )
  call config_get_virial( config, virial )
#ifdef BENCHMARK_MODE
  call config_get_num_min( config, num_min )
  call config_get_num_max( config, num_max )
  call config_get_num_bin( config, num_bin )
#else
  num_bin = 1  
  call config_get_num( config, num )
  call config_get_ft( config, ft )
  call config_get_interval( config, snapshot_interval )
  call config_get_dt( config, dt )
#endif !! BENCHMARK_MODE
  eps2 = eps**2

#ifndef BENCHMARK_MODE
  !! initialize N-body simulation
  time = 0.0
  time_from_snapshot = 0.0
  step = 0
  present = 0
  previous = present
  snp_fin = int(ceiling(ft/snapshot_interval))
#ifdef CALCULATE_POTENTIAL
  eps_inv = 1.0/eps
#endif

  call util_hdf5_commit_datatype_vec3()
  call util_hdf5_commit_datatype_vec4()
#endif !! not BENCHMARK_MODE

  do i=1, num_bin
#ifdef BENCHMARK_MODE
     num_logbin = log(num_max/num_min)/log(2.0d0)/MERGE(num_bin-1,num_bin,num_bin>1)
     num = nint( num_min* 2**((i-1)*num_logbin) )
#endif !! BENCHMARK_MODE

     !! memory allocation
     pos => null()
     vel => null()
     vel_tmp => null()
     acc => null()
     call allocate_Nbody_particles( pos, vel, vel_tmp, acc, num )

     !! generate initial-condition
     call init_set_uniform_sphere( num, pos, vel, M_tot, rad, virial, newton )

#ifndef BENCHMARK_MODE
     !!---- begin not BENCHMARK_MODE
    
     call calc_acc( num, pos, acc, num, pos, eps2 )
#ifdef CALCULATE_POTENTIAL
     call trim_acc( num, acc, pos, eps_inv )
#else
     call trim_acc( num, acc )
#endif !! CALCULATE_POTENTIAL

     !! write the first snapshot
     error = conservatives_constructor()
     call io_write_snapshot( num, pos, vel, acc, file, 32, present, time, error )

     !! half-step integration for velocity
     call kick( num, vel, acc, real(0.5,fp_m)*dt )

     do while( present < snp_fin )
        step = step+1
        time_from_snapshot = time_from_snapshot + dt
        if( time_from_snapshot >= snapshot_interval ) present = present + 1
#ifndef NDEBUG
        write(*,'("t = ",f10.6," step = ", i8)') time + time_from_snapshot, step
#endif

        !! orbit integration (2nd-order leapfrog scheme)
        call drift( num, pos, vel, dt )
        call calc_acc( num, pos, acc, num, pos, eps2 )
#ifdef CALCULATE_POTENTIAL
        call trim_acc( num, acc, pos, eps_inv )
#else
        call trim_acc( num, acc )
#endif !! CALCULATE_POTENTIAL
        call kick( num, vel, acc, dt )

        !! write snapshot
        if( present > previous ) then
           previous = present
           time_from_snapshot = 0.0
           time = time + snapshot_interval

           call kick_backward_half( num, vel, acc, vel_tmp, dt )
           call io_write_snapshot( num, pos, vel_tmp, acc, file, 32, present, time, error )
        end if ! present > previous
     end do ! while( present < snp_fin )
     !!---- end not BENCHMARK_MODE
#else
     !!---- begin BENCHMARK_MODE
    
     !! launch benchmark
     timer = util_timer_constructor()
     call util_timer_start( timer )
     call calc_acc( num, pos, acc, num, pos, eps2 )
     call util_timer_stop( timer )

     call util_timer_get_elapsed_wall( timer, elapsed )
     iter = 1

     !! increase iteration counts if the measured time is too short
     call config_get_minimum_elapsed_time( config, minimum_elapsed )
     
     do while( elapsed < minimum_elapsed )
        !! predict the iteration counts
        iter = int( 2**ceiling( log(iter*booster*minimum_elapsed/elapsed)/log(2.0d0) ) )

        !! re-execute the benchmark
        call util_timer_clear( timer )
        call util_timer_start( timer )
        do loop=1, iter
           call calc_acc( num, pos, acc, num, pos, eps2 )
        end do
        call util_timer_stop( timer )
        call util_timer_get_elapsed_wall( timer, elapsed )
     end do ! while( elapsed < minimum_elapsed )

     !! finalize benchmark
     call io_write_log( argv(0), arglen, elapsed, iter, num, file, 32 )

     !!---- end BENCHMARK_MODE    
#endif

     !! memory deallocation
     call release_Nbody_particles( pos, vel, vel_tmp, acc )
  end do ! i
  
#ifndef BENCHMARK_MODE
  call util_hdf5_remove_datatype_vec3()
  call util_hdf5_remove_datatype_vec4()

  call util_timer_stop( time_to_solution )

  call util_timer_get_elapsed_wall( time_to_solution, elapsed )
#if defined(CALCULATE_POTENTIAL)
  call io_write_log( argv(0), arglen, elapsed, step, num, file, 32, dt, error )
#else
  call io_write_log( argv(0), arglen, elapsed, step, num, file, 32 )
#endif !! CALCULATE_POTENTIAL
  
#endif !! not BENCHMARK_MODE
end program main
