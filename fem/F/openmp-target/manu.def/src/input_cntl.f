!C
!C***
!C*** INPUT_CNTL
!C***
!C  制御パラメータをコマンドライン引数から設定する。
!C  旧版は制御ファイル INPUT.DAT を読んでいたが、diffusion / nbody と同じく
!C  「入力ファイルを持たず引数だけで問題が決まる」形式に揃えるため引数方式に
!C  した。既定値は旧 input/INPUT.DAT の内容と同一。
!C
      subroutine INPUT_CNTL
      use pfem_util
      implicit none

      character(len=1024) :: arg

!C-- 既定値 (旧 INPUT.DAT: ITER=2000, COND=1.0, QVOL=1.0, RESID=1.0e-08)
        ITER = 2000
        COND = 1.d0
        QVOL = 1.d0
        RESID= 1.d-08

!C-- 引数 1 は節点数 N (INPUT_GRID が使う)。引数 2 以降は省略可
        if (command_argument_count().ge.2) then
          call get_command_argument(2,arg)
          read (arg,*) ITER
        endif
        if (command_argument_count().ge.3) then
          call get_command_argument(3,arg)
          read (arg,*) COND
        endif
        if (command_argument_count().ge.4) then
          call get_command_argument(4,arg)
          read (arg,*) QVOL
        endif
        if (command_argument_count().ge.5) then
          call get_command_argument(5,arg)
          read (arg,*) RESID
        endif

        pfemRarray(1)= RESID
        pfemIarray(1)= ITER

      return
      end
