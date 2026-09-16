/**
 ** INPUT_CNTL
 **
 ** 制御パラメータをコマンドライン引数から設定する。
 ** 旧版は制御ファイル INPUT.DAT を読んでいたが、diffusion / nbody と同じく
 ** 「入力ファイルを持たず引数だけで問題が決まる」形式に揃えるため引数方式にした。
 ** 既定値は旧 input/INPUT.DAT の内容と同一。
 **/
#include <stdio.h>
#include <stdlib.h>
#include "pfem_util.h"
/** **/
void INPUT_CNTL( int argc, char *argv[] )
{
	/* 既定値 (旧 INPUT.DAT: ITER=2000, COND=1.0, QVOL=1.0, RESID=1.0e-08) */
	ITER  = 2000;
	COND  = 1.0;
	QVOL  = 1.0;
	RESID = 1.0e-08;

	/* argv[1] は節点数 N (INPUT_GRID が使う)。argv[2] 以降は省略可 */
	if( argc > 2 ) ITER  = (KINT )atoi(argv[2]);
	if( argc > 3 ) COND  = (KREAL)atof(argv[3]);
	if( argc > 4 ) QVOL  = (KREAL)atof(argv[4]);
	if( argc > 5 ) RESID = (KREAL)atof(argv[5]);

	pfemRarray[0]= RESID;
	pfemIarray[0]= ITER;
}
