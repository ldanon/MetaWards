#define _CRTDBG_MAP_ALLOC
#define _CRT_SECURE_NO_DEPRECATE
#include <stdlib.h>
//#include <crtdbg.h>

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "wards_lib.h"
#include "globals.h"



int main(int argc, char *argv[]){
	network *net;
	
	int **inf, **playinf;
	parameters *par;
	int s,i;
	char command[100];
	
	int nseeds;
	int *to_seed;

	double *min,*max;

	double beta[N_INF_CLASSES];
	double scaling;
	
	gsl_rng *r; // random number generator stuff;
	
	
	printf("Input 1: random seed %d\n\n",atoi(argv[1]));
	
	r = gsl_rng_alloc (gsl_rng_default);
	gsl_rng_set (r, -1*(int)time(NULL) + atoi(argv[1]));
	
	
	
  printf("Input 2: parameter file name to be read %s\n",argv[2]);
	
	
	FILE *file = fopen(argv[2], "r"); //open file

	
	int linenumber;
	double b1,b2,s1,s2,s3;
	linenumber =atoi(argv[3]);
	printf("Input 3: line of parameter file to read %d\n\n",linenumber);

	i = 0;
	if ( file != NULL ) // if file is there do loop
	{
	  char line[256]; /* or other suitable maximum line size */
	  while (fgets(line, sizeof line, file) != NULL) /* read a line */
	  {
	    if (i == linenumber)
	    {
  	    sscanf(line,"%lf,%lf,%lf,%lf,%lf\n",&b1,&b2,&s1,&s2,&s3);
  	    printf("line number %d\n\n\n\n\n\n",i);
  	    //use line or in a function return it
	      //in case of a return first close the file with "fclose(file);"
	    }
	      i++;
	  }
	  fclose(file);
	}
	else
	{
	  printf("ERROR: File %s not found\n",argv[2]);//file doesn't exist
	}
	
	printf("Parameters used: b1: %lf b2:  %lf s1:  %lf s2:  %lf s3:  %lf\n",b1,b2,s1,s2,s3);
	  
	
	
	
	max=(double *)malloc(sizeof(double));
	min=(double *)malloc(sizeof(double));
	

	to_seed=(int *)calloc(sizeof(int),100);

	par=InitialiseParameters();
	
	SetInputFileNames(4,par);

	nseeds=ReadDoneFile(par->SeedName,to_seed);
	
	printf("2\n");
	
	net=BuildWardsNetworkDistance(par);
	if(net==NULL){
		printf("network not found, exiting\n");
		return 0;
	}
	
	else printf("network built\n");
		
	inf=InitialiseInfections(net);
	playinf=InitialisePlayInfections(net);
  
	GetMinMaxDistances(net,min,max);

	par->DynDistCutoff=*max+1;

//	for(i=0;i<N_INF_CLASSES;i++)beta[i]=par->beta[i];
//	for(i=0;i<N_INF_CLASSES;i++)par->beta[i]=beta[i]*(1.4/1.9);
	
	s=-1;
	par->StaticPlayAtHome=0.0;
	ResetEverything(net,par);
	RescalePlayMatrix(net,par);
	
	par->PlayToWork=0;
	par->WorkToPlay=0;

	MovePopulationFromPlayToWork(net,par,r);
	
	
	par->DailyImports=0;
	RunModel(net,par,inf,playinf,r,to_seed,s);
	
	
	
	
	FreeInfections(inf);
	FreeInfections(playinf);

	free(inf);
	free(playinf);
	
	
	free(net->nodes);
	free(net->to_links);
	free(net->play);
	free(net);

	free(to_seed);
	free(par);
	gsl_rng_free (r);
	free(min);
	free(max);
//		_CrtDumpMemoryLeaks();
	return 0;
}


