/**
 ** INPUT_CNTL
 **/
#include <stdio.h>
#include <stdlib.h>
#include "pfem_util.h"
/** **/
void INPUT_CNTL( char* input )
{
	FILE *fp;
	if( (fp=fopen( input,"r")) == NULL){
		fprintf(stdout,"input file cannot be opened!\n");
		exit(1);
	}
	fscanf(fp,"%s",fname);
	fscanf(fp,"%d",&ITER);
#if   FP ==  32
	fscanf(fp, "%f   %f", &COND, &QVOL);
	fscanf(fp, "%f",      &RESID);
#elif FP ==  64
	fscanf(fp, "%lf %lf", &COND, &QVOL);
	fscanf(fp, "%lf",     &RESID);
#elif FP == 128
	fscanf(fp, "%Lf %Lf", &COND, &QVOL);
	fscanf(fp, "%Lf",     &RESID);
#endif    
	fclose(fp);

	pfemRarray[0]= RESID;
	pfemIarray[0]= ITER;
}


