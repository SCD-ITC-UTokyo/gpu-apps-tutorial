module io
  use type
  implicit none
  interface
     subroutine io_write_snapshot( num, pos, vel, acc, fileF, filesizeF, snp_id, time, error_ptr ) &
          bind(C,name="io_write_snapshot_F")
       import :: c_int, position, velocity, acceleration, c_char, fp_m, c_ptr
       integer(c_int), value :: num
       type(position), intent(in) :: pos(*)
       type(velocity), intent(in) :: vel(*)
       type(acceleration), intent(in) :: acc(*)
       character(c_char), intent(in) :: fileF(*)
       integer(c_int), value :: filesizeF
       integer(c_int), value :: snp_id
       real(fp_m),     value :: time
       type(c_ptr),    value :: error_ptr
     end subroutine io_write_snapshot

#if defined(CALCULATE_POTENTIAL) && !defined(BENCHMARK_MODE)
     subroutine io_write_log( execF, execsizeF, elapsed, step, num, fileF, filesizeF, dt, error_ptr ) &
          bind(C,name="io_write_log_F")
       import :: c_char, c_double, c_int, fp_m, c_ptr
       character(c_char), intent(in) :: execF(*)
       integer(c_int), value :: execsizeF
       real(c_double), value :: elapsed
       integer(c_int), value :: step
       integer(c_int), value :: num
       character(c_char), intent(in) :: fileF(*)
       integer(c_int), value :: filesizeF
       real(fp_m),     value :: dt
       type(c_ptr),    value :: error_ptr
     end subroutine io_write_log
#else
     subroutine io_write_log( execF, execsizeF, elapsed, step, num, fileF, filesizeF ) &
          bind(C,name="io_write_log_F")
       import :: c_char, c_double, c_int
       character(c_char), intent(in) :: execF(*)
       integer(c_int), value :: execsizeF
       real(c_double), value :: elapsed
       integer(c_int), value :: step
       integer(c_int), value :: num
       character(c_char), intent(in) :: fileF(*)
       integer(c_int), value :: filesizeF
     end subroutine io_write_log
#endif
  end interface

end module io
