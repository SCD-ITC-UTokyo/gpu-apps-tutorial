!C
!C***
!C*** pfem_fem_util
!C***
!C
!C  pfem_util にすでに PNQ/PNE/PNT/WEI/POS/NCOL1/NCOL2/SHAPE/PNX/PNY/PNZ/DETJ が
!C  宣言されているため、ここで再宣言すると NVHPC が "use-associated" エラーを出す。
!C  use pfem_util のみで継承し、追加宣言は行わない。
!C  (元の hairdesc は GCC/Intel の緩い挙動に依存していたが NVHPC は厳格)
!C
      module pfem_fem_util
      use pfem_util
      end module pfem_fem_util
