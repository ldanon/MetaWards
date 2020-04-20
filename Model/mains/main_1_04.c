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
	int s,i,Xdays;
	char command[100],str[10];
	
	int nseeds;
	int *to_seed;

	double *min,*max;

	double beta[N_INF_CLASSES];
	double scaling;
	
	gsl_rng *r; // random number generator stuff;
	
	
	printf("Input 1: random seed %d\n\n",atoi(argv[1]));
	
	r = gsl_rng_alloc (gsl_rng_default);
	//gsl_rng_set (r, -1*(int)time(NULL) + atoi(argv[1]));
	gsl_rng_set (r,atoi(argv[1]));
	printf("Input 2: parameter file name to be read %s\n",argv[2]);
	
	max=(double *)malloc(sizeof(double));
	min=(double *)malloc(sizeof(double));
	

	to_seed=(int *)calloc(sizeof(int),100);

	par=InitialiseParameters();
	
	ReadParametersFile(par,argv[2],atoi(argv[3])); // function to read parameters from file and put into par. 
	
	par->UV=atof(argv[4]);
	
	
	par->n_restrict = 2;
//	par->controlScale[0] = 1; //R0=2.8
//	Xdays =27*7; //From 1 March - End August
	Xdays = 27*7 - 8;//From 8 March (our seed) - End August  
	par->controlsON[0] = 15; //23 March
	//par->controlsOFF[0] = Xdays;
//	par->controlsOFF[0] = par->controlsON[0]+21; // three weeks (four from start, Option 1) //13 April
//	par->controlsOFF[0] = par->controlsON[0]+21+21; // approx 1 May (actually 4 May)
	par->controlsOFF[0] = par->controlsON[0]+21+18; //  exactly 1 May 
//    par->controlsOFF[0] = par->controlsON[0]+21+18+31; //   1 June
//	par->controlsOFF[0] = par->controlsON[0]+184; // until September (longer period, Option 3)
  par->controlScale[0] = (0.6)/2.8; 

  par->controlsON[1]=par->controlsOFF[0];
  par->controlsOFF[1]=Xdays;
  par->controlScale[1] = 0.9/2.8;

  // for(i=1;i<par->n_restrict;i++){  
//    par->controlsON[i]=par->controlsOFF[i-1]+14;
//    par->controlsOFF[i]=par->controlsON[i]+14;
//    par->controlScale[i] = (0.8 + ((double) i /10.0))/2.8;
//    par->controlScale[i] = (par->controlScale[0]+((double)i)/10.0)/2.8; 
//    printf("Controls i %d: ON %lf OFF %lf \n", i, par->controlsON[i],par->controlsOFF[i]);
//  }
  
  //scanf("%s \n",str);
	SetInputFileNames(4,par);
	
	strcpy(par->AdditionalSeeding,argv[5]);
  printf("%s\n",par->AdditionalSeeding);
	
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

	s=-1;
	par->StaticPlayAtHome=0.7;
	ResetEverything(net,par);
	RescalePlayMatrix(net,par);
	
	par->PlayToWork=0;
	par->WorkToPlay=0;

	MovePopulationFromPlayToWork(net,par,r);
	
	
	par->DailyImports=0;
//	RunModel(net,par,inf,playinf,r,to_seed,s);
	RunModelForXDays(net,par,inf,playinf,r,to_seed,s,Xdays);
	
	
	
	
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




