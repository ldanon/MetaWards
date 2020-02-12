#define _CRTDBG_MAP_ALLOC
#define _CRT_SECURE_NO_DEPRECATE 1
#include <stdlib.h>
//#include <crtdbg.h>
//#include <string.h>

#include "wards_lib.h"
#include "globals.h"


#ifdef VTKWARDS
	#include "vtkwards.h"
#endif

#ifdef EXTRASEEDS
	int ADDSEEDS[1000][3];
	int NADDSEEDS;
#endif

#ifdef SELFISOLATE
	int IsDangerous[MAXSIZE];
#endif

// NOTE: the play matrix uses play[i].suscept to store information, but actively applies play[i].weight in Iterate()
// HOWEVER: the work matrix uses work[i].weight to store stuff, but applies work[i].suscept in Iterate()


void RemoveFromList(from_link *l){
	if(l->next!=NULL)RemoveFromList(l->next);
	free(l);
}


network *BuildWardsNetwork(parameters *par)

// Creates a network from a file (specified in par->WorkFname) with format:
// Node_1 Node_2 weight 1-2
// Node_3 Node_4 weight 3-4
// Node_4 Node_1 weight 4-1
// Node_2 Node_1 weight 2-1
// ...

// BE CAREFUL!! Weights may  not be symmetric, network is built with asymmetric links

// play=0 builds network from input file and NOTHING ELSE
// play=1 build the play matrix in net->play;

{
	
	node *nlist;
	to_link *llist;
	
	network *net; 
	int i, from, to;
	double weight;//x=0,y=0;
	int nnod,nlinks;
	
	FILE *inFile;
		
	nlist=(node *)calloc(sizeof(node),MAXSIZE);
	llist=(to_link *)calloc(sizeof(to_link),MAXLINKS);
	
	net=(network *)malloc(sizeof(network));
	
	net->nodes=nlist;
	net->to_links=llist;
	
	
	inFile=fopen(par->WorkName,"r");
	if( (inFile)==NULL){
		
		printf("ERROR: NETWORK FILE NOT FOUND %s... NETWORK NOT BUILT\n", par->WorkName );
		return NULL;
	
	}
	else
		printf( "The file %s was opened... excellent, carry on.\n", par->WorkName );
	
	for(i=0;i<MAXSIZE;i++){
		nlist[i].label=-1;
		nlist[i].Denominator_D=nlist[i].Denominator_N=nlist[i].Denominator_P=nlist[i].Denominator_PD=0.0;
	}

	for(i=0;i<MAXLINKS;i++)llist[i].ifrom=llist[i].ito=0;
	
	// Read the data and count nodes
	nnod=0;
	nlinks=0;
	while(!feof(inFile)){
		
		fscanf(inFile,"%d %d %lf\n",&from,&to,&weight);
//		if(from!=to){
//			weight=0;
//		}
		nlinks++;

		if(from==0||to==0){
			printf("ERROR: Zero in link list\n node1:%d node2:%d\n",from,to);
			printf("Renumber Files and start again!!!\n");
			return NULL;
		}
		
		if(nlist[from].label==-1){
			nlist[from].label=from;
			nlist[from].begin_to=nlinks;
			nlist[from].end_to=nlinks;
			nnod++;
		}

		if(from==to)nlist[from].self_w=nlinks;

		nlist[from].end_to++;
		llist[nlinks].ifrom=from;

		llist[nlinks].ito=to;
		
		llist[nlinks].weight=(int)weight;
	  	
		llist[nlinks].suscept=(int)weight;
		
		nlist[from].Denominator_N+=(int)weight;
		nlist[to].Denominator_D+=(int)weight;
	}
	
	fclose(inFile);
	
	net->nnodes=nnod;
	net->nlinks=nlinks;
	FillInGaps(net);

	BuildPlayMatrix(net,par);

	return net;
	
}




network *BuildWardsNetworkDistance(parameters *par)
// Calls BuildWardsNetwork (as above), but adds extra bit, to read LocationFile and 
// calculate distances of links, put them in net->to_links[i].distance 
// Distances are not included in the play matrix. 
{
	int i1,i;
	double x,y,x1,y1,x2,y2;
//	double dist;
	node *wards;
	to_link *links,*plinks;
	
	
	FILE *locf=fopen(par->PositionName,"r");
	
	network *net=BuildWardsNetwork(par);


#ifdef WEEKENDS
	to_links *welinks;



	BuildWeekendMatrix(net,par);
	welinks=net->weekend;
#endif

	wards=net->nodes;
	links=net->to_links;
	plinks=net->play;
	
	if( (locf)==NULL ){
		
		printf("ERROR: LOCATION FILE NOT FOUND %s ... NETWORK NOT BUILT\n", par->PositionName );
		return NULL;
		
	}
	else printf("Location files %s found, great, carry on.\n",par->PositionName );
	
	while( !feof(locf) ){
		fscanf(locf,"%d %lf %lf\n",&i1,&x,&y);
		if(i1>net->nnodes){
			printf("problems, input in location file is out of range: \n i1 = %d and net->nnodes = %d\n ",
				   i1,net->nnodes);
		}
		wards[i1].x=x;
		wards[i1].y=y;		
	}
	
	for(i=0;i<net->nlinks;i++){
		x1=wards[links[i].ifrom].x;
		y1=wards[links[i].ifrom].y;

		x2=wards[links[i].ito].x;
		y2=wards[links[i].ito].y;
		
		links[i].distance=plinks[i].distance=sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
#ifdef WEEKENDS
		welinks[i].distance=links[i].distance;
#endif
	}
	fclose(locf);
	return net;
}

network *BuildWardsNetworkDistanceIdentifiers(parameters *par){
	FILE *inF=fopen(par->IdentifierName,"r");
	char temp[10];
	int i=1;
	int id;

	network *net=BuildWardsNetworkDistance(par);

	node *wards=net->nodes;

	if( (inF)==NULL ){
		
		printf("ERROR: IDENTIFIER FILE NOT FOUND %s ... NETWORK NOT BUILT\n", par->IdentifierName );
		return NULL;
		
	}
	else printf("Identifier file %s found, great, carry on.\n",par->IdentifierName );


	while(!feof(inF)){
		fscanf(inF,"%s\n",temp);
		strcpy(wards[i].id,temp);
		i++;
	}
	
	fclose(inF);
	
	inF=fopen(par->IdentifierName2,"r");
	if( (inF)==NULL ){
		
		printf("ERROR: IDENTIFIER 2 FILE NOT FOUND %s ... NETWORK NOT BUILT\n", par->IdentifierName2 );
		return NULL;
		
	}
	else printf("Identifier 2 file %s found, great, carry on.\n",par->IdentifierName2 );
	i=1;
	while(!feof(inF)){
		fscanf(inF,"%s %d\n",temp,&id);	
		wards[i].vacid=id;
		i++;
	}
	

	return net;
}

int ApplyStaticDistanceCutoff(network *net, parameters *par){
	// this must be done after RescalePlayMatrix!!!
	int i,j;
	node *wards=net->nodes;
	to_link *links=net->to_links;
	to_link	*play=net->play;
	to_link *weekend=net->weekend;

	double cutoff=par->DataDistCutoff;
	double ToDistribute=0;
	double pprop;
	double checksum;
	double to_move,temp;
	double p,countrem;
	double tot_work_weight=0;
	int total_halted=0;

	for(i=1;i<=net->nnodes;i++){ // loop through nodes
		pprop=checksum=0.0;
		for(j=wards[i].begin_p;j<wards[i].end_p;j++){ //loop through links of node i
			checksum+=play[j].weight;
			if(play[j].distance>cutoff){		//	collect proportion of players to redistribute
										// and set weight of those links to 0
				pprop+=play[j].weight;
				play[j].weight=0.0;

			}
		}

		checksum=0.0;
		for(j=wards[i].begin_p;j<wards[i].end_p;j++){
			if(play[j].distance<=cutoff){		//	redistribute play matrix
				play[j].weight /= (1.0-pprop);

			}
			checksum+=play[j].weight;
		}
		total_halted+=(int)ceil(pprop*wards[i].play_suscept);
		
		for(j=wards[i].begin_to;j<wards[i].end_to;j++){
			tot_work_weight+=links[j].weight;
			if(links[j].distance>cutoff){		//	collect proportion of workers to redistribute
												// and set weight of those links to 0
				pprop+=links[j].weight;
				links[j].suscept=0.0;
				
			}
		}
		total_halted+=(int)pprop;
		countrem=0.0;
		for(j=wards[i].begin_to;j<wards[i].end_to;j++){
		
			if(links[j].distance<=cutoff){		//redistribute workers
				
				
				temp=pprop*links[j].weight/tot_work_weight;
 				to_move = floor(temp);
				p = temp - to_move;

				countrem+=p;

				if(countrem>=1.0){
					to_move+=1.0; countrem-=1.0;
				}

				links[j].suscept+=to_move;

			}
		}

#ifdef WEEKENDS
		pprop=checksum=0.0;
		for(j=wards[i].begin_we;j<wards[i].end_we;j++){ //loop through links of node i
			checksum+=weekend[j].weight;
			if(weekend[j].distance>cutoff){		//	collect proportion of players to redistribute
										// and set weight of those links to 0
				pprop+=weekend[j].weight;
				weekend[j].weight=0.0;

			}
		}

		checksum=0.0;
		for(j=wards[i].begin_we;j<wards[i].end_we;j++){
			if(weekend[j].distance<=cutoff){		//	redistribute play matrix
				weekend[j].weight /= (1.0-pprop);

			}
			checksum+=weekend[j].weight;
		}
#endif

	}
	return total_halted;
}

void ResetPlayMatrix(network *net){
	to_link *play=net->play;
	int i;
	for(i=1;i<=net->plinks;i++)play[i].weight=play[i].suscept;
	return;
}

void ResetWeekendMatrix(network *net){
	to_link *play=net->weekend;
	int i;
	for(i=1;i<=net->plinks;i++)play[i].weight=play[i].suscept;
	return;
}

void ResetWorkMatrix(network *net){
	to_link *links=net->to_links;
	int i;

	for(i=1;i<=net->nlinks;i++)links[i].suscept=links[i].weight;
	return;
}


void ResetPlaySusceptibles(network *net){
	node *wards=net->nodes;
	int i;
	
	for(i=1;i<=net->nnodes;i++){
		wards[i].play_suscept=wards[i].save_play_suscept;
	}
	
	return;
}

void ResetEverything(network *net,parameters *par){
	int i;
	ResetWorkMatrix(net);
	ResetPlayMatrix(net);
	ResetPlaySusceptibles(net);
#ifdef WEEKENDS
	ResetWeekendMatrix(net);
#endif	
	for(i=0;i<N_INF_CLASSES-1;i++)par->ContribFOI[i]=1;
	return;
}


void FillInGaps(network *net){
	int i;
	to_link *llinks=net->to_links;
	node *nodes=net->nodes;
	
	for(i=1;i<=net->nlinks;i++){
				
		if(nodes[llinks[i].ito].label!=llinks[i].ito){
			nodes[llinks[i].ito].label=llinks[i].ito;
			net->nnodes++;
		}
	}
}

void BuildPlayMatrix(network *net,parameters *par){

	int j, nlinks;
	int i1, i2;
	int from,to;
	double weight;
	node *nlist=net->nodes;
	to_link *llist;
	double cum=0;
	
	FILE *inFile=fopen(par->PlayName,"r");
	FILE *inFile2=fopen(par->PlaySizeName,"r");
	
	if( (inFile)==NULL){
		
		printf("ERROR: PLAY FILE NOT FOUND \"%s\"... NETWORK NOT BUILT\n", par->PlayName );
		return;
		
	}
	else
		printf( "The file %s was opened... excellent, carry on.\n", par->PlayName );
	
	if( (inFile2)==NULL){
		
		printf("ERROR: PlaySize FILE NOT FOUND \"%s\"... NETWORK NOT BUILT correctly\n", par->PlaySizeName );
		return;
		
	}
	else
		printf( "The PlaySize file %s was opened... excellent, carry on.\n", par->PlaySizeName );
	
	
	llist = (to_link *)calloc(sizeof(to_link),MAXLINKS); // assuming that there will be no more play links than work links
	
	nlinks=0;
	
	for(j=1;j<=net->nnodes;j++)nlist[j].label=-1;


	while(!feof(inFile)){

		fscanf(inFile,"%d %d %lf\n",&from,&to,&weight);
		//if(from!=to){weight=0.0;}
		nlinks++;
		if(nlinks>2413000){
      printf("%d\n",nlinks);
		}
		if(from==0||to==0){
			printf("ERROR: Zero in link list\n node1:%d node2:%d\n",from,to);
			printf("Renumber Files and start again!!!\n");
			return;
		}
		if(nlist[from].label==-1){
			nlist[from].label=from;
			nlist[from].begin_p=nlinks;
			nlist[from].end_p=nlinks;
		}
		if(from==to)nlist[from].self_p=nlinks;

		nlist[from].end_p++;

		llist[nlinks].ifrom=from;

		llist[nlinks].ito=to;

		llist[nlinks].weight=weight;

		nlist[from].Denominator_P+=weight; //not denominator_p

		nlist[from].play_suscept+=weight;
	}



	for(j=1;j<=nlinks;j++){ //rescale appropriately!

		if(!strcmp(par->PlayName,par->WorkName)){ // if the input file is the same as the PlayMatrix, normalise!
			llist[j].weight=llist[j].weight/nlist[llist[j].ifrom].Denominator_P;
//			llist[j].suscept=llist[j].weight;
		}

		llist[j].suscept=llist[j].weight; // and save the weights, once they have been rescaled.

		//if(par->StaticPlayAtHome>0.0){
		//	if(llist[j].ifrom!=llist[j].ito){
		//		llist[j].weight=llist[j].weight*(1-par->StaticPlayAtHome);
		//	}
		//	else{
		//		llist[j].weight=(1-llist[j].weight)*(par->StaticPlayAtHome)+llist[j].weight;
		//	}
		//}
	}

	fclose(inFile);
	
	FillInGaps(net);

	while(!feof(inFile2)){ // Read in the real number of play susceptibles.

		fscanf(inFile2,"%d %d\n",&i1,&i2);

		nlist[i1].play_suscept=nlist[i1].Denominator_P=nlist[i1].save_play_suscept=i2;
		

	}
	fclose(inFile2);
	
	
	net->plinks=nlinks;
	net->play=llist;

	
	return;
}




void BuildWeekendMatrix(network *net,parameters *par){

	int j, nlinks;
	int from,to;
	double weight;
	node *nlist=net->nodes;
	to_link *llist;
	double cum=0;
	
	FILE *inFile=fopen(par->WeekendName,"r");
	
	if( (inFile)==NULL){
		
		printf("ERROR: WEEKEND FILE NOT FOUND \"%s\"... NETWORK NOT BUILT\n", par->WeekendName );
		return;
		
	}
	else
		printf( "The file %s was opened... excellent, carry on.\n", par->WeekendName );
	

	llist = (to_link *)calloc(sizeof(to_link),MAXLINKS); // assuming that there will be no more play links than work links
	
	nlinks=0;
	
	for(j=1;j<=net->nnodes;j++)nlist[j].label=-1;


	while(!feof(inFile)){

		fscanf(inFile,"%d %d %lf\n",&from,&to,&weight);
		//if(from!=to){weight=0.0;}
		nlinks++;

		if(from==0||to==0){
			printf("ERROR: Zero in link list\n node1:%d node2:%d\n",from,to);
			printf("Renumber Files and start again!!!\n");
			return;
		}
		if(nlist[from].label==-1){
			nlist[from].label=from;
			nlist[from].begin_we=nlinks;
			nlist[from].end_we=nlinks;
		}
		if(from==to)nlist[from].self_we=nlinks;

		nlist[from].end_we++;

		llist[nlinks].ifrom=from;

		llist[nlinks].ito=to;

		llist[nlinks].weight=weight;
		llist[j].suscept=llist[j].weight; // and save the weights, once they have been rescaled.

	}





	fclose(inFile);
	
	FillInGaps(net);

	net->welinks=nlinks;
	net->weekend=llist;

	
	return;
}

void RescalePlayMatrix(network *net,	parameters *par){
	//Static Play At Home rescaling. 
	//for 1, everyone stays at home.
	// for 0 a lot of people move around. 

	int j;
	to_link *llist=net->play;
	node *nlist=net->nodes;

	if(par->StaticPlayAtHome>0.0){ //if we are making people stay at home, then do this loop through nodes
	  for(j=1;j<=net->plinks;j++){ //rescale appropriately!

			if(llist[j].ifrom!=llist[j].ito){ // if it's not the home ward, then reduce the number of play movers
				llist[j].weight=llist[j].suscept*(1-par->StaticPlayAtHome); // 
			}
			else{  // if it is the home ward 
				llist[j].weight=(1-llist[j].suscept)*(par->StaticPlayAtHome)+llist[j].suscept;
			}
		}
	}

	RecalculatePlayDenominatorDay(net,par);
}


void RescaleWeekendMatrix(network *net,	parameters *par){
	//Static Play At Home rescaling. 
	//for 1, everyone stays at home.
	// for 0 a lot of people move around. 

	int j;
	to_link *llist=net->weekend;
	node *nlist=net->nodes;

	for(j=1;j<=net->welinks;j++){ //rescale appropriately!

		if(par->StaticPlayAtHome>0.0){
			if(llist[j].ifrom!=llist[j].ito){

				llist[j].weight=llist[j].suscept*(1-par->StaticPlayAtHome);

			}

			else{

				llist[j].weight=(1-llist[j].suscept)*(par->StaticPlayAtHome)+llist[j].suscept;

			}

		}

	}

}

void TestPlayMatrix(network *net){

	int i,j;
	node *wards=net->nodes;
	to_link *links=net->play;
	double cum=0;
	
	for(i=1;i<net->nnodes;i++){
		for(j=wards[i].begin_p;j<wards[i].end_p;j++){
		//	printf("%lf  ",links[j].weight);
			cum+=links[j].weight;
		}
		printf("cum: %lf\n",cum);
		cum=0;
	}

	return;
}


void TestWeekendMatrix(network *net){

	int i,j;
	node *wards=net->nodes;
	to_link *links=net->weekend;
	double cum=0;
	
	for(i=1;i<net->nnodes;i++){
		for(j=wards[i].begin_we;j<wards[i].end_we;j++){
		//	printf("%lf  ",links[j].weight);
			cum+=links[j].weight;
		}
		printf("cum: %lf\n",cum);
		cum=0;
	}

	return;
}


void MovePopulationFromPlayToWork(network *net, parameters *par, gsl_rng *r){
	
	// And Vice Versa From Work to Play
	// The relevant parameters are par->PlayToWork 
	//						   and par->WorkToPlay
	//
	// When both are 0, don't do anything;
	// When PlayToWork > 0 move par->PlayToWork proportion from Play to Work.
	// When WorkToPlay > 0 move par->WorkToPlay proportion from Work to Play.
	
	
	int i;
	double to_move,temp,p;
	
	double countrem=0.0;
	double check=0.0;
	
	to_link *links=net->to_links; // workers (regular movements) 
	node *wards=net->nodes;		// wards
	to_link *play=net->play;	// links of players 
	
	
	if(par->WorkToPlay > 0.0){
		for(i=1;i<=net->nlinks;i++){
			
			to_move = ceil(links[i].suscept * par->WorkToPlay);
			if(to_move > links[i].suscept){
				printf("to_move > links[i].suscept\n");
			}

			links[i].suscept -= to_move;
			
			wards[links[i].ifrom].play_suscept += to_move;
		}
	}
	
	if(par->PlayToWork > 0.0){
		
		for(i=1;i<=net->plinks;i++){
			
			temp =   par->PlayToWork * (play[i].weight * wards[play[i].ifrom].save_play_suscept);
			
			to_move = floor(temp);
			p = temp - to_move;
			
			countrem += p;
			
			if(countrem>=1.0){
				to_move+=1.0; countrem-=1.0;
			}
			
			if(wards[play[i].ifrom].play_suscept<to_move){
				to_move=wards[play[i].ifrom].play_suscept;			
			}
			wards[play[i].ifrom].play_suscept -= to_move;
			links[i].suscept += to_move;
		}
		
	}

	RecalculateWorkDenominatorDay(net,par);
	RecalculatePlayDenominatorDay(net,par);

	return;

}
void RecalculatePlayDenominatorDay(network *net, parameters *par){
	
	node *wards=net->nodes;
	to_link *plist=net->play;

	int i,j;
	double sum=0;

	for(i=1;i<=net->nnodes;i++)wards[i].Denominator_PD=wards[i].Denominator_P=0.0;

	//for(i=1;i<=net->nnodes;i++){
	for(j=1;j<=net->plinks;j++){
		wards[plist[j].ito].Denominator_PD += plist[j].weight * wards[plist[j].ifrom].play_suscept;
		
		sum += plist[j].weight * wards[plist[j].ifrom].play_suscept;
	}


	//printf("%f ",sum);

	sum=0;

	for(i=1;i<=net->nnodes;i++){

		wards[i].Denominator_PD=(int)floor(wards[i].Denominator_PD + 0.5);
		wards[i].Denominator_P=wards[i].play_suscept;
		if(wards[i].play_suscept<0.0){
			printf("gonnorrhea");
		}
		sum += wards[i].Denominator_P;


	}

	//printf("%f \n",sum);

}

void RecalculateWorkDenominatorDay(network *net, parameters *par){

	node *wards=net->nodes;
	to_link *links=net->to_links;
	int i,j;
//	FILE *outF;
	int sum=0;
	for(i=1;i<=net->nnodes;i++)wards[i].Denominator_D=wards[i].Denominator_N=0.0;


	for(j=1;j<=net->nlinks;j++){
		wards[links[j].ito].Denominator_D+=links[j].suscept;
		wards[links[j].ifrom].Denominator_N+=links[j].suscept;
		sum += (int)links[j].suscept;
	}

	//	printf("%d ",sum);
	//outF=fopen("workd.dat","w");
	//for(i=1;i<=net->nnodes;i++){
	//	fprintf(outF,"%d %d\n",i,(int)wards[i].Denominator_D);
	//}
	//fclose(outF);
	//sum=0;
	//outF=fopen("workn.dat","w");
	//for(i=1;i<=net->nnodes;i++){
	//	fprintf(outF,"%d %d\n",i,(int)wards[i].Denominator_N);
	//	sum+=(int)wards[i].Denominator_N;
	//}
	//printf("%d ",sum);
	//fclose(outF);
}


void TestNetwork(network *net){
	int i,j;
	node *wards=net->nodes;
	to_link *links=net->to_links;
	double sum,bigsum=0;
	

	for(i=1;i<=net->nnodes;i++){
		//	while(wards[i].label!=-1){
		//printf("%d %d \n",i,wards[i].label);
		//printf("%lf %lf %lf \n",wards[i].Denominator_D,wards[i].Denominator_N,wards[i].Denominator_P); 

		sum=0;
		for(j=wards[i].begin_to;j<wards[i].end_to;j++){
			sum+=links[j].suscept;
			if(links[j].ifrom==links[j].ito){
	//			printf("j %d --- ",j);
	//			printf("from %d ", links[j].ifrom);
	//			printf("to %d ", links[j].ito);
	//			printf("weight %g\n",links[j].weight);
//				printf("play %lf\n",net->play[j]);
			}
		}
		//printf("SUM	%lf Denominator_N %lf Denominator_P %lf\n",sum,wards[i].Denominator_N,wards[i].Denominator_P);	
		bigsum+=sum+wards[i].play_suscept;
	}
	printf("bigsum %lf \n",bigsum);
}

void RemoveGraph(network *net){	//recursive removal of nodes
	// Only works with lists, not with array
	free(net->nodes);
	free(net->to_links);
	
	free(net);
}

// ---------------------------------------------------------------------/////////
//						DYNAMIC STUFF									/////////
// ---------------------------------------------------------------------/////////

int **InitialiseInfections(network *net){
	int i;
	int **inf=(int **)calloc(sizeof(int *),N_INF_CLASSES);
	
	for(i=0;i<N_INF_CLASSES;i++)inf[i]=(int *)calloc(sizeof(int),net->nlinks+1);
	
	return inf;
}

void FreeInfections(int **p){
	int i;
	for(i=0;i<N_INF_CLASSES;i++)free(p[i]);
	return;
}

int **InitialisePlayInfections(network *net){
	int i;
	int **inf=(int **)calloc(sizeof(int *),N_INF_CLASSES);
	
	for(i=0;i<N_INF_CLASSES;i++)inf[i]=(int *)calloc(sizeof(int),net->nnodes+1);
	
	return inf;
	
}


void SeedInfectionAtRandomLink(network *net,parameters *par, 
							   gsl_rng *r,char C,int **inf){
	// char is a switch which controls whether we start the infection at a random node or a random link
	// N for Node L for Link.
	
	int i=0;	
	int j=0;
	
	node *wards=net->nodes;
	
	to_link *links=net->to_links;
	
	
	
	if(C=='L'){	
		
		j=(int)ceil((net->nlinks-1)*gsl_rng_uniform(r));
		
		printf("j %d link from %d to %d\n",j,links[j].ifrom,links[j].ito);	
		
		printf("seeding here\n");
	}
	
	else if(C=='N'){
		
		i=(int)ceil((net->nnodes-1)*gsl_rng_uniform(r));
		
		while(links[j].ito!=i && links[j].ifrom!=i)j++;
		
		printf("j %d link from %d to %d\n",j,links[j].ifrom,links[j].ito);	
		
		printf("seeding here\n");
		
	}
	else {
		printf("NEEDS TO BE NODE OR LINK SEEDING\n");
		return;
	} 
	
	inf[0][j]=par->initial_inf;
	
	return;
	
}

#ifdef EXTRASEEDS
void LoadAdditionalSeeds(char *fname){
	FILE *inF=fopen(fname,"r");
	NADDSEEDS=0;
	int t,loc,num;
	
	while(!feof(inF)){
		fscanf(inF,"%d %d %d\n",&t,&num,&loc);
//		printf("fname %s t %d num %d loc %d\n",fname,t,num,loc);
		ADDSEEDS[NADDSEEDS][0]=t;
		ADDSEEDS[NADDSEEDS][1]=loc;
		ADDSEEDS[NADDSEEDS][2]=num;
		NADDSEEDS++;
	}
	fclose(inF);
}

void InfectAdditionalSeeds(network *net, parameters *par, int **inf,int **pinf,int t){
	int i=0;
	node *wards=net->nodes;
		
	// ADDSEEDS[i][0]=t of occurrence,...[][1]=location ward,[][2]=number of infecteds in ward;
	for(i=0;i<NADDSEEDS;i++){
		if(ADDSEEDS[i][0]==t){
			if(wards[ADDSEEDS[i][1]].play_suscept<ADDSEEDS[i][2]){
				printf("NOT ENOUGH SUSCEPTIBLES in WARD FOR SEEDING\n");
				
			}
			else {
				wards[ADDSEEDS[i][1]].play_suscept-=ADDSEEDS[i][2];
				pinf[0][ADDSEEDS[i][1]]+=ADDSEEDS[i][2];
			}
			
		}
	}
}

#endif

void SeedInfectionAtNode(network *net, parameters *par, int node_seed, int **inf, int **pinf){
	node *wards=net->nodes;
	to_link *links=net->to_links;
	
	int j=0;
	
	while(links[j].ito!=node_seed || links[j].ifrom!=node_seed)j++;
	
	//	printf("j %d link from %d to %d\n",j,links[j].ifrom,links[j].ito);	
	
	//	printf("seeding here\n");

	if(links[j].suscept<par->initial_inf){
		wards[node_seed].play_suscept-=par->initial_inf;
		pinf[0][node_seed]+=par->initial_inf;
		return;
	}

	inf[0][j]=par->initial_inf;
	links[j].suscept-=par->initial_inf;


	return;
}

void SeedAllWards(network *net, parameters *par,int **inf, int **pinf,double expected){
	node *wards=net->nodes;
	double temp=0;	
	int i;
	double total=57104043;
	int to_seed=0;
	double frac;
	
	frac=expected/total;
	
	for(i=0;i<=net->nnodes;i++){
	  temp=wards[i].Denominator_N+wards[i].Denominator_P;
	  to_seed=(int)(frac*temp + 0.5);
	  wards[i].play_suscept-=to_seed;
	  pinf[0][i]+=to_seed;		
	}

	return;
}
						 
						 
void ClearAllInfections(network *net, int **inf, int **pinf){
	int i,j;
	
	for(i=0;i<N_INF_CLASSES;i++){
		for(j=0;j<=net->nlinks;j++)
			inf[i][j]=0;
		
		for(j=0;j<=net->nnodes;j++)
			pinf[i][j]=0;
	}


};




double Rate2Prob(double R){
	if(R<1e-6) return R-R*R/2.0;
	else return 1.0-exp(-R); 
}

void RunModel(network *net, parameters *par, int **inf,
			  int **playinf, gsl_rng *r, int *to_seed,
			  int s){


	int i=0,day;
	int infecteds;
	int track_wards[MAXSIZE];


	FILE **files,**wfiles;
	FILE *Export=fopen("ForMattData.dat","w");

#ifdef VACCINATE
	int trigger=0;
	size_t vac[MAXSIZE];
	size_t wardsRA[MAXSIZE];
	double RiskRA[MAXSIZE];	
	size_t SortRA[MAXSIZE];
	FILE *vacF=fopen("Vaccinated.dat","w");
	for(i=0;i<MAXSIZE;i++){
		wardsRA[i]=vac[i]=SortRA[i]=0;
		RiskRA[i]=0.0;
	}
#endif

	i=0;

	files=OpenFiles();
	//	wfiles=OpenWardFiles(par,track_wards);

	ClearAllInfections(net,inf,playinf);
	
	//printf("node_seed %d\n \n",to_seed[s]);

#ifndef IMPORTS	
	if(s<0){
	  SeedAllWards(net,par,inf,playinf,par->DailyImports);
	}
	else SeedInfectionAtNode(net,par,to_seed[s],inf,playinf);
#endif
		
	infecteds=ExtractData(net,inf,playinf,i,files);
	day=1;

#ifdef EXTRASEEDS
	LoadAdditionalSeeds(par->AdditionalSeeding);
#endif
	
	
//#ifdef IMPORTS	
	while(infecteds!=0 || i<5){	
//#else
//	while(infecteds!=0 && i != 1000){
//#endif
			
			
#ifdef EXTRASEEDS
		InfectAdditionalSeeds(net,par,inf,playinf,i);
#endif

#ifdef WEEKENDS
		printf("day: %d\n",day);
		if(day>5){
			IterateWeekend(net,inf,playinf,par,r,i);
			printf("weekend\n");
		}
		else {	
			Iterate(net,inf,playinf,par,r,i);
			printf("normal day\n");
		}
		if(day==7)day=0;
		day++;
#else
		Iterate(net,inf,playinf,par,r,i);
#endif


		printf("\n%d %d\n",i,infecteds);

		infecteds=ExtractData(net,inf,playinf,i,files);
		ExtractDataForGraphicsToFile(net,inf,playinf,Export);



		i++;



#ifdef VACCINATE
//		VaccinateWards(net,wardsRA,inf,playinf,vac,par);
		if(infecteds>par->GlobalDetectionThresh)trigger=1;
		if(trigger==1){
		//	VaccinateCounty(net,RiskRA,SortRA,inf,playinf,vac,par);
			VaccinateSameID(net,RiskRA,SortRA,inf,playinf,vac,par);
		//	Vaccinate
			if(par->ContribFOI[0]==1.0){
				for(j=0;j<N_INF_CLASSES-1;j++){
					par->ContribFOI[j]=0.2;
				}
			}
		}
		fprintf(vacF,"%d %d\n",i,HowManyVaccinated(vac));
#endif

	
		//OutputWardData(wfiles,track_wards,net,inf,playinf,i);


#ifdef VTKWARDS
		vtkwards_draw(inf,playinf);
#endif

	}
	fclose(Export);
	printf("Infection died ... Ending at time %d\n",i);
	CloseFiles(files);
	//	CloseWardFiles(wfiles,track_wards);
#ifdef VACCINATE
	fclose(vacF);
#endif
}


void GetMinMaxDistances(network *net, double *min, double *max){
	int i;
	to_link *links=net->to_links;
//	FILE *temp=fopen("dist.dat","w");
	
	
	*min=1000000;
	*max=-1;
	
	for(i=1;i<=net->nlinks;i++){
//		fprintf(temp,"%lf\n",links[i].distance);
		
		if(links[i].distance>*max)*max=links[i].distance;
		if(links[i].distance<*min)*min=links[i].distance;
	}
	
	//printf("maxdist %lf mindist %lf\n\n\n",*max,*min);
//	fclose(temp);
	return ;
}



int ReadDoneFile(char *fname,int *nodes_seeded){
  printf("%s\n",fname);
	FILE *inF=fopen(fname,"r");
	int i=0,i1;
	
	printf("%p -- \n",inF);
	
	while(!feof(inF)){
	  fscanf(inF,"%d\n",&i1);
		nodes_seeded[i]=i1;
		i++;
	}
	fclose(inF);
	
	return i;
	
}




FILE **OpenFiles(){
	FILE **files=(FILE **)calloc(sizeof(FILE *),7);

	files[0]=fopen("WorkInfections.dat","w");
	files[1]=fopen("NumberWardsInfected.dat","w");
	files[2]=fopen("MeanXY.dat","w");
	files[3]=fopen("PlayInfections.dat","w");
	files[4]=fopen("TotalInfections.dat","w");
	files[5]=fopen("VarXY.dat","w");
	files[6]=fopen("Dispersal.dat","w");
	return files;
	
}

void CloseFiles(FILE **files){
	fclose(files[0]);
	fclose(files[1]);
	fclose(files[2]);
	fclose(files[3]);
	fclose(files[4]);
	fclose(files[5]);
	fclose(files[6]);
	free(files);
	return;
}

FILE **OpenWardFiles(parameters *par,int to_track[20]){
	FILE **files;
	int i,num;
	char str[30];

	FILE *inF=fopen(par->NodesToTrack,"r");

	for(i=0;i<MAXSIZE;i++)to_track[i]=0;

	if(inF!=NULL){
		printf("Tracking nodes specified in %s\n",par->NodesToTrack);
	}
	else {printf("Not tracking anything\n");return NULL;}


	num=0;
	while(!feof(inF)){
		
		fscanf(inF,"%d\n",&i);
		to_track[num]=i;
		num++;
	}
	//if(num>_getmaxstdio()){
	//	_setmaxstdio(2048);
	//	printf("%d \n",_getmaxstdio());
	//}

	files=(FILE **)calloc(sizeof(FILE *),num);
	for(i=0;i<num;i++){
		sprintf(str,"%d",to_track[i]);
		strcat(str,".dat");
		files[i]=fopen(str,"w");
		fprintf(files[i],"hmm\n");
	}

	return files;
}

void CloseWardFiles(FILE **files,int *to_track){
	int i=0;
	while(to_track[i]!=0){
		fclose(files[i]);
		i++;
	}
	free(files);
	return;
}

void OutputWardData(FILE **files,int *to_track,network *net,int **winf,int **pinf, int t){
	
	int i,j,w;
	int WardTotal;
	node *wards=net->nodes;
	
	w=0;
	while(to_track[w]!=0){
		WardTotal=0;
		for(i=0;i<N_INF_CLASSES-1;i++){ // collect data

			WardTotal+=pinf[i][to_track[w]];		// play infections sum

			for(j=wards[to_track[w]].begin_to;j<wards[to_track[w]].end_to;j++){
				WardTotal+=winf[i][j];				// work infections sum
			}

		}		

		fprintf(files[w],"%d %d\n",t,WardTotal);
		w++;
		
	}
	
	return;

}

int ExtractData(network *net,int **inf,int **pinf, int t, FILE **files){
	
	to_link *links=net->to_links;
	node *wards=net->nodes;
	
	int i,j;
	int InfWards[N_INF_CLASSES]; 
	int PInfWards[N_INF_CLASSES]; 
	int InfTot[N_INF_CLASSES];
	int PInfTot[N_INF_CLASSES];
//	int TotalInfWard[N_INF_CLASSES][MAXSIZE];
	int TotalInfWard[MAXSIZE];
	int TotalNewInfWard[MAXSIZE];

	int nInfWards[N_INF_CLASSES];
	int nInfWardsAllClasses=0;
	int Total=0;
	int TotalNew=0;
	int Recovereds,Susceptibles,Latent;
	double sumX,sumY,sumX2,sumY2;
	double meanX,meanY,varX,varY,Dispersal;

	sumX=sumY=sumX2=sumY2=meanX=meanY=varX=varY=Dispersal=0.0;

	Recovereds=Susceptibles=Latent=0;
	
	fprintf(files[0],"%d ",t);
	fprintf(files[1],"%d ",t);
	//fprintf(files[2],"%d ",t);
	fprintf(files[3],"%d ",t);

	for(j=1;j<=net->nnodes;j++)TotalNewInfWard[j]=TotalInfWard[j]=0;

	for(i=0;i<N_INF_CLASSES;i++){ 
		nInfWards[i]=0;
		InfTot[i]=PInfTot[i]=InfWards[i]=PInfWards[i]=0;	
		
		for(j=1;j<=net->nlinks;j++){
			if(i==0){
				Susceptibles+= (int)links[j].suscept;
				TotalNewInfWard[links[j].ifrom]+=inf[i][j];
			}
			if(inf[i][j]!=0){
#ifdef SELFISOLATE
				if(i>4&&i<10)IsDangerous[links[j].ito]+=inf[i][j];
#endif
				InfTot[i]+=inf[i][j]; // number of infected links in class  i 
				TotalInfWard[links[j].ifrom]+=inf[i][j];
			}
		}
		
		
		
		
		for(j=1;j<=net->nnodes;j++){
			if(i==0){
				Susceptibles+= (int)wards[j].play_suscept;
				if(pinf[i][j]>0)TotalNewInfWard[j]+=pinf[i][j];
				
				if(TotalNewInfWard[j]!=0){
					sumX+=TotalNewInfWard[j]*wards[j].x;
					sumY+=TotalNewInfWard[j]*wards[j].y;
					sumX2+=TotalNewInfWard[j]*wards[j].x*wards[j].x;
					sumY2+=TotalNewInfWard[j]*wards[j].y*wards[j].y;
					TotalNew+=TotalNewInfWard[j];
				}
			
			}
			if(pinf[i][j]>0){
				PInfTot[i]+=pinf[i][j];
				TotalInfWard[j]+=pinf[i][j];
#ifdef SELFISOLATE
				if(i>4&&i<10)IsDangerous[i]+=pinf[i][j];
#endif
			}
			if(i<(N_INF_CLASSES-1) && TotalInfWard[j]>0) nInfWards[i]++;
			
	//		if(i==0)fprintf(files[2],"%d ",TotalInfWard[i][j]);
			
		}
		
		fprintf(files[0],"%d ",InfTot[i]);
		fprintf(files[1],"%d ",nInfWards[i]);
		fprintf(files[3],"%d ",PInfTot[i]);
		
		if(i==1)
		  {Latent+=InfTot[i]+PInfTot[i];}
		else if(i<N_INF_CLASSES-1 & i>1){
			Total+=InfTot[i]+PInfTot[i];
		}
		else Recovereds+=InfTot[i]+PInfTot[i];
	
	
	}

	if(TotalNew>0){
		meanX=(double)sumX/TotalNew;
		meanY=(double)sumY/TotalNew;

		varX=(double)(sumX2-(double)sumX*meanX)/(TotalNew-1);
		varY=(double)(sumY2-(double)sumY*meanY)/(TotalNew-1);

		Dispersal=sqrt(varY+varX);
		fprintf(files[2],"%d %lf %lf\n",t,meanX,meanY);
		fprintf(files[5],"%d %lf %lf\n",t,varX,varY);
		fprintf(files[6],"%d %lf\n",t,Dispersal);
	
	}
	else {

		fprintf(files[2],"%d %lf %lf\n",t,0.0,0.0);
		fprintf(files[5],"%d %lf %lf\n",t,0.0,0.0);
		fprintf(files[6],"%d %lf\n",t,0.0);
	
	}

	fprintf(files[0],"\n");
	fprintf(files[1],"\n");
	fprintf(files[3],"\n");
	fprintf(files[4],"%d \n",Total);
	fflush(files[4]);

	printf("S: %d    ",Susceptibles);
	printf("E: %d    ",Latent);
	printf("I: %d    ",Total);
	printf("R: %d    ",Recovereds);
	printf("IW: %d   ",nInfWards[0]);
 	printf("TOTAL POPULATION %d\n",Susceptibles+Total+Recovereds);

	return (Total+Latent);
}











int ExtractDataForGraphicsToFile(network *net, int **inf,int **pinf,FILE *outF){
	
	to_link *links=net->to_links;
	node *wards=net->nodes;
	
	int i,j;

	int InfTot[N_INF_CLASSES];
	int TotalInfWard[N_INF_CLASSES][MAXSIZE];
		
	int Total=0;
	int Recovereds,Susceptibles;
	int TotalInfections[MAXSIZE];

	Recovereds=Susceptibles=0;
	
	for(j=1;j<=net->nnodes;j++)TotalInfections[j]=0;

	for(i=0;i<N_INF_CLASSES;i++){ 

		for(j=1;j<=net->nnodes;j++)TotalInfWard[i][j]=0;
		
		
		for(j=1;j<=net->nlinks;j++){
			if(inf[i][j]!=0){
				InfTot[i]+=inf[i][j]; // number of infected links in class  i 
				TotalInfWard[i][links[j].ifrom]+=inf[i][j];
				if(i<N_INF_CLASSES-1){
					TotalInfections[links[j].ifrom]+=inf[i][j];
					Total+=inf[i][j];
				}
			}
		}
		
		
		
		
		for(j=1;j<=net->nnodes;j++){
			TotalInfWard[i][j]+=pinf[i][j];
			if(pinf[i][j]!=0 && i<N_INF_CLASSES-1){
				TotalInfections[j]+=pinf[i][j];
				Total+=pinf[i][j];			
			}
			if(i==2)fprintf(outF,"%d ",TotalInfections[j]);// incidence
			//if(i==N_INF_CLASSES-1)fprintf(outF,"%d ",TotalInfections[j]); // prevalences
			//if(i==N_INF_CLASSES-1)Prevalence[j]=TotalInfections[j];
		}

		  //if(i==N_INF_CLASSES-1)fprintf(outF,"%d ",TotalInfections[j]);// incidence
		
	}
	
	fprintf(outF,"\n");
	return Total;
}







parameters *InitialiseParameters(){
	
#ifdef FLU
	double beta[N_INF_CLASSES]={		0, 0, 0.5, 0.5, 0};
	double Progress[N_INF_CLASSES]={	1, 1, 0.5, 0.5, 0};
	double TooIllToMove[N_INF_CLASSES]={    0, 0, 0.5, 0.8, 1};
//	double TooIllToMove[N_INF_CLASSES]={    0, 0, 0.0, 0.0, 1};
	double ContribFOI[N_INF_CLASSES]={	1, 1, 1,   1,   0}; // set to 1 for the time being;
#endif
#ifdef FLU2
	double beta[N_INF_CLASSES]={		0,   1,  0.5, 0.2, 0.2,  0};
	double Progress[N_INF_CLASSES]={	0.95,   1,    1,   1,   1,  0};
	double TooIllToMove[N_INF_CLASSES]={    0, 0.1,  0.5, 0.8, 0.5,  1};
	double ContribFOI[N_INF_CLASSES]={	1,   1,    1,   1,   1,  0}; // set to 1 for the time being;
#endif

#ifdef POX
	double beta[N_INF_CLASSES]={        0,    0,    0,    0.1,  0.2,  0.8, 0.8, 0.05, 0.05, 0.05, 0};
	double Progress[N_INF_CLASSES]={    0.25, 0.25, 0.25, 0.66, 0.66, 0.5, 0.5, 0.2,  0.2,  0.2,  0};
	double TooIllToMove[N_INF_CLASSES]={0,    0,    0,    0.2,  0.2,  0.5, 0.8, 0.9,  0.9,  0.9,  1};
	double ContribFOI[N_INF_CLASSES]={  1,    1,    1,      1,    1,    1,   1,   1,    1,    1,  0};
#endif

#ifdef DEFAULT
	double beta[N_INF_CLASSES]={0,1,0};
	double Progress[N_INF_CLASSES]={1,1,0};
	double TooIllToMove[N_INF_CLASSES]={0,0,0};
	double ContribFOI[N_INF_CLASSES]={1,1,0};
#endif
	
#ifdef NCOV
	//double beta[N_INF_CLASSES]={	0, 0, 0.95, 0.95, 0};
	double beta[N_INF_CLASSES]={	0, 0, 0.95, 0.95, 0};
	//double Progress[N_INF_CLASSES]={	1, 1.0/5.2, 1.0/1.1, 1/1.1, 0};
	double Progress[N_INF_CLASSES]={	1, 0.1923, 0.909091, 0.909091, 0};
	double TooIllToMove[N_INF_CLASSES]={ 0, 0, 0, 0.0, 0};
 	double ContribFOI[N_INF_CLASSES]={1, 1, 1, 1, 0}; // set to 1 for the time being;
#endif
	
	int i;
	parameters *par;
	
	
	par=(parameters *)malloc(sizeof(parameters));

	par->initial_inf=5;

	par->LengthDay=0.7;
	
	par->PLengthDay=0.5;
	
	for(i=0;i<N_INF_CLASSES;i++){
		par->beta[i]=beta[i];
		par->Progress[i]=Progress[i];
		par->TooIllToMove[i]=TooIllToMove[i];
		par->ContribFOI[i]=ContribFOI[i];
	}	
	
	par->DynDistCutoff = 10000000;
	par->DataDistCutoff = 10000000;
	par->WorkToPlay=0.0;
	par->PlayToWork=0.0;
	par->StaticPlayAtHome=0;
	par->DynPlayAtHome=0;
	
	par->LocalVaccinationThresh = 4;
	par->GlobalDetectionThresh = 4;
	par->NeighbourWeightThreshold = 0.0;
	par->DailyWardVaccinationCapacity = 5;
	par->UV=0.0;

	return par;
}



void ReadParametersFile(parameters *par, char *fname,int lineno){
  
  FILE *file = fopen(fname, "r"); //open file
  
  
  int linenumber,i;
  double b2,b3,s2,s3,s4;
  linenumber =lineno;
  printf("Input 3: line of parameter file to read %d\n\n",linenumber);
  
  i = 0;
  if ( file != NULL ) // if file is there do loop
  {
    char line[256]; /* or other suitable maximum line size */
    while (fgets(line, sizeof line, file) != NULL) /* read a line */
    {
      if (i == linenumber)
      {
        sscanf(line,"%lf,%lf,%lf,%lf,%lf\n",&b2,&b3,&s2,&s3,&s4);
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
    printf("ERROR: File %s not found\n",fname);//file doesn't exist
  }
  
  //printf("Parameters used: b2: %lf b3:  %lf s2:  %lf s3:  %lf s4:  %lf\n",b2,b3,s2,s3,s4);
  
  par->beta[2]=b2;
  par->beta[3]=b3;
  par->Progress[1]=s2;  
  par->Progress[2]=s3;
  par->Progress[3]=s4;  
  
//    printf("Parameters used: b0: %lf b1:  %lf b2:  %lf b3:  %lf b4:  %lf\n",par->beta[0],par->beta[1],par->beta[2],par->beta[3],par->beta[4]);
//    printf("prog0: %lf prog1:  %lf prog2:  %lf prog3:  %lf prog4:  %lf\n",par->Progress[0],par->Progress[1],par->Progress[2],par->Progress[3],par->Progress[4]);
  
  return;
  
}


void SetInputFileNames(int choice,parameters *par){
  char *filestring;
  char *dirstring;
  switch(choice){
  case 1:
    printf("Using files in Z:/data/\n"); // for development code WINDOWS
    strcpy(par->WorkName,"Z:/data/UK1.dat");
    //			strcpy(par->WorkName,"Z:\\data\\Gravity_Model.dat");
    //			strcpy(par->PlayName,"Z:/data/Weights_new.dat");
    strcpy(par->PlayName,par->WorkName);
    //strcpy(par->PlayName,"C:/data/combined_matrix.dat");
    strcpy(par->IdentifierName,"Z:/data/Identifiers.dat");
    strcpy(par->IdentifierName2,"Z:/data/level2.dat");
    
    strcpy(par->WeekendName,"Z:/data/WeekendMatrix.dat");
    strcpy(par->PlaySizeName,"Z:/data/PlaySize.dat");
    strcpy(par->PositionName,"Z:/data/CBB.dat");
    strcpy(par->SeedName,"Z:/data/seeds.dat");
    strcpy(par->NodesToTrack,"Z:/data/seeds.dat");
    
    strcpy(par->AdditionalSeeding,"Z:/data/ExtraSeeds.dat");
    
    //strcpy(par->NodesToTrack,"Z:/ldanon/data/AllWardInts.dat");
    return;
    break;
  case 2:
    printf("Using files in /Users/ldanon/data/ \n");//for Macs 
    strcpy(par->WorkName,"/Users/ldanon/data/UK1.dat");
    strcpy(par->PlayName,"/Users/ldanon/data/Weights_new.dat");
    
    strcpy(par->IdentifierName,"/Users/ldanon/data/Identifiers.dat");
    strcpy(par->IdentifierName2,"/Users/ldanon/data/level3.dat");
    
    strcpy(par->WeekendName,"/Users/ldanon/data/WeekendMatrix.dat");
    strcpy(par->PlaySizeName,"/Users/ldanon/data/PlaySize.dat");
    strcpy(par->PositionName,"/Users/ldanon/data/CBB.dat");
    strcpy(par->SeedName,"/Users/ldanon/data/seeds.dat");
    strcpy(par->NodesToTrack,"/Users/ldanon/data/seeds.dat");
    
    strcpy(par->AdditionalSeeding,"/Users/ldanon/data/ExtraSeeds.dat");
    return;
    break;
  case 3:
    printf("Using files in /users/leond/data/ \n");//for the cluster
    
    strcpy(par->WorkName,"/users/leond/data/UK1.dat");
    strcpy(par->PlayName,"/users/leond/data/Weights_new.dat");
    strcpy(par->IdentifierName,"/users/leond/data/Identifiers.dat");
    strcpy(par->IdentifierName2,"/users/leond/data/level3.dat");
    
    strcpy(par->WeekendName,"/users/leond/data/WeekendMatrix.dat");
    strcpy(par->PlaySizeName,"/users/leond/data/PlaySize.dat");
    strcpy(par->PositionName,"/users/leond/data/CBB.dat");
    strcpy(par->SeedName,"/users/leond/data/seeds.dat");
    strcpy(par->NodesToTrack,"/users/leond/data/seeds.dat");
    
    strcpy(par->AdditionalSeeding,"/users/leond/data/ExtraSeeds.dat");
    
    
    return;
    break;
  
  case 4:

    dirstring=getenv("HOME"); // home directory strin
    strcat(dirstring,"/GitHub/MetaWards/2011Data/"); // add to that the directory for 2011 data
    printf("Using files in %s \n", dirstring); //
    
    strcat(strcpy(par->WorkName, dirstring), "EW1.dat");
    strcat(strcpy(par->PlayName, dirstring), "PlayMatrix.dat");
    strcat(strcpy(par->PlaySizeName, dirstring), "PlaySize.dat");
    strcat(strcpy(par->PositionName,dirstring),"CBB2011.dat");
    strcat(strcpy(par->SeedName,dirstring),"seeds.dat");
    strcat(strcpy(par->NodesToTrack,dirstring),"seeds.dat");
    strcat(strcpy(par->AdditionalSeeding,dirstring),"ExtraSeedsBrighton.dat");
    strcat(strcpy(par->UVFilename,dirstring),"UVScaling.csv");
	  return;
	  break;
	  
  default:
    printf("WRONG WRONG WRONG\n");
  return;
  break;
  }
  
}



void Iterate(network *net, int **inf, int **playinf, parameters *par, gsl_rng *r,int t){

	int i,j,k;
	double temp,uv=par->UV,uvscale=(1-uv/2.0+uv*cos(2*M_PI*(t)/365.0)/2.0); // starting day = 41
	int staying, moving,playmove,l;
	double InfProb,Rate;

	//	int **TotalInfInWard;
	double cum_prob,ProbScaled;
	double cutoff=par->DynDistCutoff;

	double thresh=0.01;
//	int DayInfW,DayInfP,NightInfW,NightInfP;

	
	to_link *links=net->to_links;
	node *wards=net->nodes;
	to_link *plinks=net->play;

	for(i=1;i<=net->nnodes;i++){
		wards[i].DayFOI=wards[i].NightFOI=0.0;
	}
#ifdef IMPORTS
	printf("Day: %d Imports: expected %d actual %d\n",t,(int)par->DailyImports,ImportInfection(net,inf,playinf,par,r));
#endif
	for(i=0;i<N_INF_CLASSES;i++){ //setting up the day and nighttime FOI

		if(par->ContribFOI[i]>0) {


			for(j=1;j<=net->nlinks;j++){ // Deterministic movements (to work).
				
				if(inf[i][j]>0){	
					
					if(inf[i][j]>(int)links[j].weight){
						printf("inf[%d][%d] %d > links[j].weight %lf\n",i,j,inf[i][j],links[j].weight );
					}
					if(links[j].distance<cutoff){
#ifdef SELFISOLATE
						if((double)(IsDangerous[links[j].ito]/(wards[links[j].ito].Denominator_D + wards[links[j].ito].Denominator_P))>thresh){
							staying=inf[i][j];
						}
						else	
#endif							
							staying = gsl_ran_binomial(r,par->TooIllToMove[i],inf[i][j]);	// number staying. This is G_ij//


						if(staying<0){
							printf("staying<0 \n");
						}
						moving = inf[i][j] - staying;					// number moving. This is I_ij-G_ij

						wards[links[j].ifrom].DayFOI+=staying*par->ContribFOI[i]*par->beta[i]*uvscale;

						//Daytime Force of 
						//Infection is proportional to
						//numer of people staying 
						//in the ward (too ill to work)
						// this is the sum for all G_ij (including g_ii
						
						
						wards[links[j].ito].DayFOI += moving*par->ContribFOI[i]*par->beta[i]*uvscale;

						// Daytime FOI for destination is incremented (including self links, I_ii)

					}
					else {
					  wards[links[j].ifrom].DayFOI += inf[i][j]*par->ContribFOI[i]*par->beta[i]*uvscale;
					}

					wards[links[j].ifrom].NightFOI += inf[i][j]*par->ContribFOI[i]*par->beta[i]*uvscale;
					// Nighttime Force of Infection is 
					// prop. to the number of Infected individuals
					// in the ward 
					// This I_ii in Lambda^N
					//printf("%d   %d  %lf  %lf \n",t,temp->number,lt->to->DayFOI,lt->to->NightFOI);

				}	

			}// end of infectious class loop


			for(j=1;j<=net->nnodes;j++){ // playmatrix loop FOI loop (random movements)

				if(playinf[i][j]!=0){

				  wards[j].NightFOI += playinf[i][j] * par->ContribFOI[i] * par->beta[i]*uvscale;

					staying = gsl_ran_binomial(r,(par->DynPlayAtHome)*par->TooIllToMove[i],playinf[i][j]); // number of people staying gets bigger as PlayAtHome increases

					if(staying<0){
						printf("staying <0 \n");
					}

					moving = playinf[i][j] - staying;

					cum_prob=0;

					k=wards[j].begin_p;

					while(moving > 0 && k <= wards[j].end_p){ // distributing people across play wards

						if(plinks[k].distance < cutoff){

							ProbScaled=plinks[k].weight/(1-cum_prob);

							cum_prob += plinks[k].weight;

							playmove = gsl_ran_binomial(r,ProbScaled,moving);
#ifdef SELFISOLATE
							if((double)(IsDangerous[plinks[j].ito]/(wards[links[j].ito].Denominator_D + wards[links[j].ito].Denominator_P))>thresh){
								staying+=playmove;
							}	
							else {
#endif				
							  wards[plinks[k].ito].DayFOI += playmove * par->ContribFOI[i] * par->beta[i]*uvscale;
							
							
#ifdef SELFISOLATE							
							}
#endif
							moving -= playmove;

						}

						k++;					
					}

					wards[j].DayFOI+= (moving + staying) * par->ContribFOI[i] * par->beta[i]*uvscale;

				} 

			}// loop through nodes

		}// end of if par->Contrib[i]>0

	}// end of loop through classes


	for(i=N_INF_CLASSES-2;i>=0;i--){ // recovery, move through classes backwards.

		for(j=1;j<=net->nlinks;j++){

			if(inf[i][j]>0){

				l=gsl_ran_binomial(r,par->Progress[i],inf[i][j]);

				if(l>0){
					inf[i+1][j]+=l;

					inf[i][j]-=l;

				}

			}
			else if(inf[i][j]!=0){
				printf("PROBLEMs\n");

			}

		}

		for(j=1;j<=net->nnodes;j++){
			if(playinf[i][j]>0){
				l=gsl_ran_binomial(r,par->Progress[i],playinf[i][j]);

				if(l>0){

					playinf[i+1][j]+=l;
					playinf[i][j]-=l;

				}
			}
			else if(playinf[i][j]!=0){
				printf("PROBLEMS!!!");
			}
		}

	}



	i=0;

	InfProb=0.0;
	temp=0;

	for(j=1;j<=net->nlinks;j++){ // actual new infections for fixed movements
	
		InfProb=0.0;

		if(links[j].distance < cutoff){ // if distance is below cutoff (reasonable distance) infect in work ward
			
			if(wards[links[j].ito].DayFOI > 0){   // daytime infection of link j

#ifdef SELFISOLATE
				if((double)(IsDangerous[links[j].ito]/(wards[links[j].ito].Denominator_D + wards[links[j].ito].Denominator_P))>thresh){
					InfProb=0;
				}
				else {	
#endif
					Rate= (par->LengthDay)*(wards[links[j].ito].DayFOI)/
						(wards[links[j].ito].Denominator_D + wards[links[j].ito].Denominator_PD); 

					InfProb=Rate2Prob(Rate);

#ifdef SELFISOLATE
				}
#endif

			}	
		}

		else if(wards[links[j].ifrom].DayFOI>0){ // if distance is too large infect in home ward with day FOI

			Rate=(par->LengthDay)*(wards[links[j].ifrom].DayFOI)/
				(wards[links[j].ifrom].Denominator_D + wards[links[j].ifrom].Denominator_PD); 
			InfProb=Rate2Prob( Rate );

		}

		if(InfProb>0.0){ // Daytime Infection of workers
			
			l=gsl_ran_binomial(r,InfProb,(int)links[j].suscept); // actual infection
			
			if(l>0){
	//			printf("InfProb %lf, susc %lf, l %d\n",InfProb,links[j].suscept,l);
				inf[i][j] += l;
				links[j].suscept -= l;

			}

			InfProb=0.0;

		}


		InfProb=0.0;

		if(wards[links[j].ifrom].NightFOI>0){ //nighttime infection of workers j
			Rate = (1.0-par->LengthDay)*(wards[links[j].ifrom].NightFOI)/
				(wards[links[j].ifrom].Denominator_N + wards[links[j].ifrom].Denominator_P); 
			InfProb = Rate2Prob( Rate );



			l = gsl_ran_binomial(r,InfProb,(int)(links[j].suscept));
			if(l > links[j].suscept){
				printf("l > links[j].suscept nighttime\n");
			}
			if(l>0){
	//			printf("NIGHT InfProb %lf, susc %lf, l %d\n",InfProb,links[j].suscept,l);
				inf[i][j]+=l;
				links[j].suscept -= l;


			}

		}

	}

	for(j=1;j<=net->nnodes;j++){ // playmatrix loop.
		InfProb=0.0;

		if(wards[j].play_suscept<0.0){

			printf("play_suscept is less than 0, problem %d\n",wards[j].label);

		}

		staying = gsl_ran_binomial(r,par->DynPlayAtHome,(int)wards[j].play_suscept);

		moving = (int)wards[j].play_suscept - staying;

		cum_prob=0;
		// daytime infection of play matrix moves

		for(k=wards[j].begin_p;k<wards[j].end_p;k++){

			if(plinks[k].distance < cutoff){

				if(wards[plinks[k].ito].DayFOI>0.0){

					ProbScaled=plinks[k].weight/(1-cum_prob);

					cum_prob += plinks[k].weight;

#ifdef SELFISOLATE
					if((double)(IsDangerous[plinks[j].ito]/(wards[plinks[k].ito].Denominator_P+wards[plinks[k].ito].Denominator_D))>thresh){
						InfProb=0;
						playmove=0;
					}	
					else {
#endif		
					playmove = gsl_ran_binomial(r,ProbScaled,moving);

					InfProb = Rate2Prob( (par->LengthDay)*(wards[plinks[k].ito].DayFOI)/
						(wards[plinks[k].ito].Denominator_PD+wards[plinks[k].ito].Denominator_D) );

#ifdef SELFISOLATE
					}
#endif
					l=gsl_ran_binomial(r,InfProb,playmove);

					moving -= playmove;

					if(l>0){
//						printf("PLAY: InfProb %lf, susc %d, l %d\n",InfProb,playmove,l);
						playinf[i][j] += l;
						wards[j].play_suscept -= l;

					}

				}// end of DayFOI if statement

			}// end of Dynamics Distance if statement.

		} // end of loop over links of wards[j]

		if((staying+moving)>0){ // infect people staying at home damnit!!!!!!!!

			InfProb = Rate2Prob( (par->LengthDay)*(wards[j].DayFOI)/
				(wards[j].Denominator_PD+wards[j].Denominator_D) );

			l=gsl_ran_binomial(r,InfProb,(staying+moving));

			if(l>0){
				playinf[i][j] += l;
				wards[j].play_suscept-=l;

			}

		}

		//nighttime infections of play movements

		if(wards[j].NightFOI>0.0){

			InfProb = Rate2Prob( (1.0-par->LengthDay)*(wards[j].NightFOI)/
				(wards[j].Denominator_N + wards[j].Denominator_P) );

			l = gsl_ran_binomial(r,InfProb,(int)(wards[j].play_suscept));

			if(l>0){

				playinf[i][j]+=l;
				wards[j].play_suscept -= l;

			}

		}

	}

	return;
}









void IterateWeekend(network *net, int **inf, int **playinf, parameters *par, gsl_rng *r,int t){

	int i,j,k;
	int staying, moving,wemove,l;
	double InfProb;
	int wesuscept[MAXSIZE];
	int weinf[N_INF_CLASSES][MAXSIZE];
	int newly_infected[MAXSIZE];
	double cum_prob,ProbScaled;
	double cutoff=par->DynDistCutoff;

//	int DayInfW,DayInfP,NightInfW,NightInfP;

	
	to_link *links=net->to_links;
	node *wards=net->nodes;
	to_link *welinks=net->weekend;
	

	for(i=1;i<=net->nnodes;i++){ // set all relevant variables to 0;
		wards[i].DayFOI=wards[i].NightFOI=0.0;
		for(j=0;j<N_INF_CLASSES;j++)weinf[j][i]=0;
		wesuscept[i]=newly_infected[i]=0;
	}

	for(i=0;i<N_INF_CLASSES;i++){ //setting up the day and nighttime FOI and susceptibles

		if(par->ContribFOI[i]>0) {

			for(j=1;j<=net->nlinks;j++){ // Sum up work movements of infection.
				if(i==0)wesuscept[links[j].ifrom]+=(int)links[j].suscept; // sum up work susceptibles once
				if(inf[i][j]>0)		
					weinf[i][links[j].ifrom]+=inf[i][j]; // sum up infecteds in the work matrix (pool together)
							
			}

			for(j=1;j<=net->nnodes;j++){ // FOI and susceptibles loop 
				if(i==0)wesuscept[j]+=(int)wards[j].play_suscept; // sum up susceptibles once
				if(playinf[i][j]>0)
					weinf[i][j]+=playinf[i][j];  // add to work the infecteds in the play matrix (pool together)

				if(weinf[i][j]>0){ // distribute infecteds and update FOI 
				  wards[j].NightFOI += weinf[i][j] * par->ContribFOI[i] * par->beta[i]*(1-0.5+0.5*cos(2*M_PI*t/365.0));

					staying = gsl_ran_binomial(r,(par->DynPlayAtHome)*par->TooIllToMove[i],weinf[i][j]); // number of people staying gets bigger as PlayAtHome increases

					if(staying<0){
						printf("staying <0 \n");
					}

					moving = weinf[i][j] - staying;

					cum_prob=0;

					k=wards[j].begin_we;

					while(moving > 0 && k < wards[j].end_we){ // distributing people across play wards

						if(welinks[k].distance < cutoff){
							ProbScaled=welinks[k].weight/(1-cum_prob);
							
							cum_prob += welinks[k].weight;

							wemove = gsl_ran_binomial(r,ProbScaled,moving);

							wards[welinks[k].ito].DayFOI += wemove * par->ContribFOI[i] * par->beta[i]*(1-0.5+0.5*cos(2*M_PI*t/365.0));

							moving -= wemove;

						}

						k++;					
					}

					wards[j].DayFOI+= (moving + staying) * par->ContribFOI[i] * par->beta[i]*(1-0.5+0.5*cos(2*M_PI*t/365.0)); // whatever is left contributes to home ward

				} 

			}// loop through nodes

		}// end of if par->Contrib[i]>0

	}// end of loop through classes



	for(i=N_INF_CLASSES-2;i>=0;i--){ // recovery, move through classes backwards. this can stay the same for weekends

		for(j=1;j<=net->nlinks;j++){

			if(inf[i][j]>0){

				l=gsl_ran_binomial(r,par->Progress[i],inf[i][j]);

				if(l>0){
					inf[i+1][j]+=l;

					inf[i][j]-=l;

				}

			}
			else if(inf[i][j]!=0){
				printf("PROBLEMs\n");

			}

		}

		for(j=1;j<=net->nnodes;j++){
			if(playinf[i][j]>0){
				l=gsl_ran_binomial(r,par->Progress[i],playinf[i][j]);

				if(l>0){

					playinf[i+1][j]+=l;
					playinf[i][j]-=l;

				}
			}
			else if(playinf[i][j]!=0){
				printf("PROBLEMS!!!");
			}
		}

	}



	i=0;

	InfProb=0.0;
	
	for(j=1;j<=net->nnodes;j++){ // weekend loop where we do the "pooled" infecting

		if(wesuscept[j]<0.0){

			printf("wesuscept is less that 0, problem\n");

		}

		staying = gsl_ran_binomial(r,par->DynPlayAtHome,(int)wesuscept[j]);

		moving = wesuscept[j] - staying;
	
		cum_prob=0;

		// daytime infection of pooled susceptibles

		for(k=wards[j].begin_we; k<wards[j].end_we; k++){

			if(welinks[k].distance < cutoff){

				if(wards[welinks[k].ito].DayFOI>0.0){

					cum_prob += welinks[k].weight;

					if ( moving < 0 ) {
						printf("problems\n");
					}

					ProbScaled=welinks[k].weight/(1-cum_prob);

					wemove = gsl_ran_binomial(r,ProbScaled,moving); // calculate how many people go to ward welinks[k].ito

					InfProb = Rate2Prob( (par->LengthDay)*(wards[welinks[k].ito].DayFOI)/
						(wards[welinks[k].ito].Denominator_P+wards[welinks[k].ito].Denominator_D) );

					l = gsl_ran_binomial(r,InfProb,wemove);  // and how many of those are actually infected. 

					moving -= wemove;

					if(l>0){
						//						printf("PLAY: InfProb %lf, susc %d, l %d\n",InfProb,playmove,l);
						newly_infected[j]+=l;// we decide later where the newly infecteds are from

					}

				}// end of DayFOI if statement

			}// end of Dynamics Distance if statement.

		} // end of loop over links of wards[j]

		if((staying+moving)>0){ // infect people staying at home damnit!!!!!!!!

			InfProb = Rate2Prob( (par->LengthDay)*(wards[j].DayFOI)/
				(wards[j].Denominator_P+wards[j].Denominator_D) );

			l=gsl_ran_binomial(r,InfProb,(staying+moving));

			if(l>0){
				newly_infected[j]+=l;// we decide later where newly infecteds are from.

			}

		}

		//nighttime infections of weekend movements

		if(wards[j].NightFOI>0.0){

			InfProb = Rate2Prob( (1.0-par->LengthDay)*(wards[j].NightFOI)/
				(wards[j].Denominator_N + wards[j].Denominator_P) );

			l = gsl_ran_binomial(r,InfProb,(wesuscept[j]-newly_infected[j]));  // how many people get infected in home ward at night
																					// the number of susceptibles is reduced by those already infected elsewhere. 
			if(l>0){

				newly_infected[j]+=l;
			}

		}


	}


	// NOW WE DECIDE WHERE THE NEWLY INFECTEDS ARE FROM.
	for(j=1;j<=net->nnodes;j++){

		cum_prob=0;
		if(newly_infected[j]>0){
	//		printf("weekend newly infected in (before) j=%d:  %d \n",j,newly_infected[j]);
			cum_prob=(double)wards[j].play_suscept / wesuscept[j];
	
			wemove=gsl_ran_binomial(r,cum_prob,newly_infected[j]); // the total number that belong to the play matrix
			
			if(wemove>0){
				if(wemove>(int)wards[j].play_suscept){
					printf("wemove too large play\n");
					wemove=(int)wards[j].play_suscept;
				}
				newly_infected[j]-=wemove; // removed from susceptible pool

				wards[j].play_suscept-=wemove; // removed from the susceptibles of the play matrix

				playinf[0][j]+=wemove; // added to the infecteds in the play matrix
			
			}

			for(k=wards[j].begin_to;k<wards[j].end_to;k++){ // now for the work matrix
				//printf("ifrom %d \n",links[k].ifrom);
				if(links[k].suscept>0.0){
					if(links[k].distance < cutoff){
						ProbScaled = ((double)links[k].suscept / wesuscept[j])/(1.0-cum_prob);

						cum_prob += (double)links[k].suscept / wesuscept[j];

						wemove = gsl_ran_binomial(r,ProbScaled,newly_infected[j]);

						if(wemove>0){
							if(wemove>(int)links[k].suscept){ // its a DIRTY HACK!!
								//if(wemove>(int)links[k].suscept+1){
								//	printf("wemove %d > links.suscept %f \n",wemove,links[k].suscept);
								//}	
								wemove=(int)links[k].suscept;
							}
							//	else printf("ok");
							newly_infected[j] -= wemove;
							links[k].suscept -=wemove;
							inf[0][k] += wemove;

						}

					}// end of Dynamics Distance if statement.
				}// end of if links[k].suscept>0 statement.
			} // end of loop over links of wards[j]

			if(cum_prob>0.0){
//				printf("weekend Cum Prob %lf\n",cum_prob);
			}
			if(newly_infected[j]>1){
	//			printf("j=%d newly_infected[j] SHOULD BE 0 but is actually %d \n",j,(int)newly_infected[j]);
			printf("weekend newly infected in j=%d:  %d \n",j,newly_infected[j]);
			}
		} // end of newly_infected[j]>0 if statement
		

	}// end of loop through nodes
	return;
}






void ExtractPlayMatrix(network *net){

	int i;	
	FILE *outF;
	to_link *play=net->play;

	outF=fopen("PlayMatrix2.dat","w");

	for(i=1;i<=net->nlinks;i++){
		fprintf(outF,"%d %d %lf\n",play[i].ifrom,play[i].ito,play[i].weight);
	}
	fclose(outF);
	return ;
}




#ifdef VACCINATE

void VaccinateWards(network *net,size_t *wardsRA,int **inf, int **pinf,size_t *vac,parameters *par){
	int i;
	int cap=par->DailyWardVaccinationCapacity;

	FindWardsToVaccinate(net,wardsRA,inf,pinf,par);
	for(i=1;i<net->nnodes;i++){
		if(wardsRA[i]>0){
			wardsRA[i]++;
			if(cap>0){
				Vaccinate1(net,i,inf,pinf,vac,par);
				cap--;
				wardsRA[i]=-1;
			}
			
		}	
	}
	return;
}
void VaccinateWards2(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par){
	int i;
	int cap=par->DailyWardVaccinationCapacity;


	FindWardsToVaccinate2(net,RiskRA,SortRA,inf,pinf,par);
	for(i=net->nnodes;i>0;i--){ // or the other way round..
		if(RiskRA[SortRA[i]]>0.0){
			if(cap>0){
				printf("Risk %lf %d\n",RiskRA[SortRA[i]],SortRA[i]);
				Vaccinate1(net,(int)SortRA[i],inf,pinf,vac,par);
				cap--;
				RiskRA[SortRA[i]]=-2;
			}
		}
	}
	return;
}

void VaccinateLondon(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par){
	//Vaccinate Just London
	int i;
	int cap=par->DailyWardVaccinationCapacity;
	node *wards=net->nodes;

	for(i=1;i<=net->nnodes;i++){
		if(cap>0){
			if(wards[i].vacid==33){
				if(vac[i]==0){
				Vaccinate1(net,i,inf,pinf,vac,par);
				cap--;
				}
			}
		}
	}
}


void VaccinateSameID(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par){
//Vaccinate those wards with same id as infected wards
	int i,j;
	int cap=par->DailyWardVaccinationCapacity;
	node *wards=net->nodes;

	for(i=1;i<=net->nnodes;i++){
		if(TotalWardInfectious(wards[i],i,inf,pinf,par)>0){
			for(j=1;j<=net->nnodes;j++){
				if(cap>0){
					if(vac[j]==0&&wards[i].vacid==wards[j].vacid){
					Vaccinate1(net,j,inf,pinf,vac,par);
					cap--;
					}
				}
			}
		}
	}
}

void VaccinateCounty(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par){
	//Vaccinate Wards Within the same county (denoted by 4 first characters of id being equal)
	int i,j;
	int cap=par->DailyWardVaccinationCapacity;
	node *wards=net->nodes;

	for(i=1;i<=net->nnodes;i++){
		if(TotalWardInfectious(wards[i],i,inf,pinf,par)>0){
			for(j=1;j<=net->nnodes;j++){
				if(vac[j]==0&&IsSameCounty(wards[i].id,wards[j].id)){
					if(cap>0){
						Vaccinate1(net,j,inf,pinf,vac,par);
						cap--;
					}
				}
			}
		}
	}
}

int IsSameCounty(char *id1,char *id2){
	if(id1[6]=='\0') {
		if(strncmp(id1,id2,3)==0) return 1;
		else return 0;
	}
	else {
		if(strncmp(id1,id2,4)==0) return 1;
		return 0;

	}
}

void VaccinateCities(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par){
	//Vaccinate Just London
	int i;
	int cap=par->DailyWardVaccinationCapacity;
	node *wards=net->nodes;
	int id;

	for(i=1;i<=net->nnodes;i++){
		if(cap>0){
			id=wards[i].vacid;
			if(id>32&&id<40){ // cities
				if(vac[i]==0){
				Vaccinate1(net,i,inf,pinf,vac,par);
				cap--;
				}
			}
		}
	}
}

void VaccinateAll(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par){
	//Vaccinate Just London
	int i;
	int cap=par->DailyWardVaccinationCapacity;
	node *wards=net->nodes;
	int id;

	for(i=1;i<=net->nnodes;i++){
		if(cap>0){
			id=wards[i].vacid;
			if(vac[i]==0){
				Vaccinate1(net,i,inf,pinf,vac,par);
				cap--;
			}
		}
	}
}

void Vaccinate1(network *net, int iward, int **inf, int **pinf, size_t *vac,parameters *par){
	// this strategy one just removes the susceptibles from the model
	node *wards=net->nodes;
	to_link *links=net->to_links;
	int i,j;
	printf("vaccinating %d\n",iward);
//	for(i=0;i<N_INF_CLASSES-1;i++){
		if(vac[iward]!=0){
			printf("hums");
		}
		vac[iward]+=(int)wards[iward].play_suscept;
		wards[iward].play_suscept=0;
		//		vac[iward]+=pinf[i][iward];
//		pinf[i][iward]=0;
		for(j=wards[iward].begin_to;j<=wards[iward].end_to;j++){
			vac[iward]+=(int)links[j].suscept;
			links[j].suscept=0;
		}
		
//	}
	return;
}


void FindWardsToVaccinate2(network *net,double *RiskRA, size_t *wardsRA, 
						   int **inf, int **pinf, parameters *par){

	int i;
	int Local,Global;
	node *wards=net->nodes;


	Local=Global=0;
	CalculateWardRisk(net,RiskRA,inf,pinf,par);
	gsl_sort_index(wardsRA,RiskRA,1,net->nnodes);
	//	printf("//////////////n");
//	for(i=net->nnodes;i>0;i--){
//		if(RiskRA[wardsRA[i]]>0.0){
//			printf("%d %d %lf\n",i,wardsRA[i],RiskRA[wardsRA[i]]);
//		}
//	}


//	for(i=1;i<=net->nnodes;i++){

//		if(wardsRA[i]==0){ // if ward has not already been vaccinated or has not already been selected for vaccination
//			Local=TotalWardInfectious(wards[i],i,inf,pinf,par);		
//			Global+=Local;
//			if( Local > par->LocalVaccinationThresh){ // is ward infectious
//				
//				wardsRA[i]=1;
//				FindNeighboursToVaccinate(net,i,wardsRA,par);
//			}
//		}

//	}
//	if(Global>par->GlobalDetectionThresh&&Global>par->LocalVaccinationThresh){
//		for(i=0;i<N_INF_CLASSES;i++){
//		//	par->ContribFOI[i]=0.2;
//		}
//	}
	return;
}




void CalculateWardRisk(network *net, double *RiskRA, int **inf, int **pinf,parameters *par){

	int i,j;
	int *tot;
	node *wards=net->nodes;
	to_link *links=net->to_links;
	to_link *plinks=net->play;

	tot=(int *)calloc(MAXSIZE,sizeof(int));

	for(i=1;i<=net->nnodes;i++){
		tot[i]=TotalWardInfectious(wards[i],i,inf,pinf,par);
	}
	for(i=1;i<=net->nnodes;i++){
		if(tot[i]>0.0){
			for(j=wards[i].begin_to;j<=wards[i].end_to;j++){
				if(RiskRA[links[j].ito]>-1)RiskRA[links[j].ito]+=plinks[j].weight*tot[i];
				if(RiskRA[i]>-1)RiskRA[i]+=plinks[j].weight*tot[i];
			}
		}
	}
	free(tot);

}


void FindWardsToVaccinate(network *net,size_t *wardsRA,int **inf,int **pinf,parameters *par){

	int i;
	int Local,Global;
	node *wards=net->nodes;
	
	Local=Global=0;

	for(i=1;i<=net->nnodes;i++){

		if(wardsRA[i]==0){ // if ward has not already been vaccinated or has not already been selected for vaccination
			Local=TotalWardInfectious(wards[i],i,inf,pinf,par);		
			Global+=Local;
			if( Local > par->LocalVaccinationThresh){ // is ward infectious
				
				wardsRA[i]=1;
				FindNeighboursToVaccinate(net,i,wardsRA,par);
			}
		}

	}
	if(Global>par->GlobalDetectionThresh&&Global>par->LocalVaccinationThresh){
		for(i=0;i<N_INF_CLASSES;i++){
		//	par->ContribFOI[i]=0.2;
		}
	}
	return;
}

void FindNeighboursToVaccinate(network *net,int iward,size_t *wardsRA, parameters *par){

	int i;
	double sum=0;
	node *wards=net->nodes;
	to_link *links=net->to_links;
	double temp;

	for(i=wards[iward].begin_to;i<=wards[iward].end_to;i++){
		if(i!=wards[iward].self_w)sum+=links[i].weight;
	}

	for(i=wards[iward].begin_to;i<=wards[iward].end_to;i++){
		if(i!=wards[iward].self_w && wardsRA[links[i].ito]==0){
			temp=links[i].weight/sum;
			if(temp>par->NeighbourWeightThreshold)wardsRA[links[i].ito]=1;
		}
	}
	
	return;
}

int IsWardInfected(node ward,int iward,int **inf,int **pinf,parameters *par){
	
	int i,j;
	int total=0;
	
	for(i=0;i<N_INF_CLASSES-1;i++)total+=pinf[i][iward];
	if(total>par->LocalVaccinationThresh)return 1;

	for(j=ward.begin_to;j<=ward.end_to;j++){
		for(i=0;i<N_INF_CLASSES-1;i++){
			if(inf[i][j]!=0){
				total+=inf[i][j];
				if(total>par->LocalVaccinationThresh)return 1;
			}
		}
	}
	
	return 0;

}

int TotalWardInfectious(node ward,int iward,int **inf,int **pinf,parameters *par){
	
	int i,j;
	int total=0;
	
	for(i=START_SYMPTOM;i<N_INF_CLASSES;i++)total+=pinf[i][iward];

	for(j=ward.begin_to;j<=ward.end_to;j++){
		for(i=START_SYMPTOM;i<N_INF_CLASSES;i++){
			if(inf[i][j]!=0){
				total+=inf[i][j];
				
			}
		}
	}

	return total;
}

size_t HowManyVaccinated(size_t *vac){
	int i;
	size_t		tot=0;
	for(i=1;i<MAXSIZE;i++)tot+=vac[i];
	
	return tot;
}




void LinkControlMeasures(network *net, int **inf, int **pinf, char *cutFname){
	
	FILE *cutF=fopen(cutFname,"r");

	int link_id;
	double prob;

	while(!feof(cutF)){
		fscanf(cutF,"%d %lf\n",&link_id,&prob);
		HaltMovementsInLink(net,link_id,inf,pinf,prob);
	}
	return;
}

void HaltMovementsInLink(network *net, int ilink, int **inf, int **pinf, double prop){
	
	to_link *work=net->to_links;
	to_link *play=net->play;
	node *wards=net->nodes;

	int to_stay,i;
	double p_to_stay;

#ifdef WEEKENDS
	to_link *we=net->weekend;
	double we_to_stay;
	we_to_stay=we[ilink].weight*prop;   // calculate proportion to redistribute

	play[ilink].weight-=we_to_stay;		 // remove from link
	play[wards[play[ilink].ifrom].self_p].weight+=we_to_stay; // add to self link

#endif


	to_stay=(int)ceil(work[ilink].suscept*prop); // calculate number to move
	work[ilink].suscept-=to_stay; // remove from susceptibles in link

	work[wards[work[ilink].ifrom].self_w].suscept+=to_stay; // add to susceptibles in self link 
	// wards[work[ilink].ifrom].play_suscept+=to_stay; // -----add to susceptibles in ward play matrix
	// --------------------------------------------------------the above two lines should be equivalent.

	for(i=0;i<N_INF_CLASSES;i++){							// for each infectious class, siphon a proportion of people moving
		to_stay=(int)ceil(inf[i][ilink]*prop);				// calculate how many to move
		inf[i][ilink]-=to_stay;								// remove from infectious class i
		inf[i][wards[work[ilink].ifrom].self_w]+=to_stay;	// add to self link work
		//pinf[i][work[ilink].ifrom]+=stay;					// in principle, these two should be the same
	}


	p_to_stay=play[ilink].weight*prop;   // calculate proportion to redistribute
	play[ilink].weight-=p_to_stay;		 // remove from link
	play[wards[play[ilink].ifrom].self_p].weight+=p_to_stay; // add to self link

	return;
}
#endif
//#ifdef IMPORTS
int ImportInfection(network *net, int **inf, int **playinf, parameters *par, gsl_rng *r){
	to_link *links=net->to_links;
	node *wards=net->nodes;

	int to_seed=0;
	double frac=par->DailyImports/(double)57104043;;
	int tot=0;
	int i;

	for(i=0;i<=net->nnodes;i++){ // players
		to_seed=gsl_ran_binomial(r,frac,(int)wards[i].play_suscept);
		if(to_seed>0){
			wards[i].play_suscept-=to_seed;
			playinf[0][i]+=to_seed;		
			tot+=to_seed;
		}
	}
	for(i=0;i<=net->nlinks;i++){	// workers
		to_seed=gsl_ran_binomial(r,frac,(int)links[i].suscept);
		if(to_seed>0){
			links[i].suscept-=to_seed;
			inf[0][i]+=to_seed;
			tot+=to_seed;	
		}
	}
	return tot;
}
	
//#endif
