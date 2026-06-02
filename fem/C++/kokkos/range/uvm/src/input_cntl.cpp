/**
 ** INPUT_CNTL
 **/
#include <stdio.h>
#include <stdlib.h>
#include "pfem_util.h"
/** **/
void INPUT_CNTL()
{
	FILE *fp;
	
	if( (fp=fopen("INPUT.DAT","r")) == NULL){
		fprintf(stdout,"input file cannot be opened!\n");
		exit(1);
	}
	fscanf(fp,"%s",fname);
	fscanf(fp,"%d",&ITER);
	fscanf(fp, "%lf %lf", &COND, &QVOL);
	fscanf(fp, "%lf",     &RESID);
	fclose(fp);

	pfemRarray[0]= RESID;
	pfemIarray[0]= ITER;
}


