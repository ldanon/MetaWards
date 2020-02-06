#ifndef _WARD_LIB_H
#define _WARD_LIB_H

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
//#include <crtdbg.h>

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <string.h>


#include "globals.h"

typedef struct network{
	struct node *nodes;
	struct to_link *to_links;
	//struct from_link *from_links;
	int nnodes;
	int nlinks;

	struct to_link *play;
	int plinks;

	struct to_link *weekend;
	int welinks;
}network;


typedef struct node{
	int label;					// label of node 
   
	int begin_to;				// where to links begin in link vector;
	int end_to;					// how many to links in link vector
	int self_w;

	int begin_p;				// play matrix begin and end in link vector
	int end_p;					// 
	int self_p;

	int begin_we;				// weekend begin and end
	int end_we;					//
	int self_we;
	
//	struct to_link *neigh;			// neighbour link list
//	struct from_link *from_neigh; // incoming neightbour list

//	struct attribute *attrib; 

	double DayFOI;				// numerator only
	double NightFOI;			// numerator only
	double WeekendFOI;			// numerator only	
	
	double play_suscept;
	double save_play_suscept;
	
	double Denominator_N;			// Denominator only
	double Denominator_D;			// Maybe won't need
	
	double Denominator_P;
	double Denominator_PD;
	double x,y;
	double b;

	char id[10];
	int vacid;

}node;

typedef struct attribute{		// non essential attributes. 
	char name[NAMESIZEMAX];		// Name of node in string format (might not need
	float x,y;					// position of node
}attribute;

typedef struct to_link{
//	node *from;					// origin node for link
//	node *to;		// destination node for link
	
	int ifrom, ito;				// integers of origins and destinations
	double weight;				// weight of link, used to save the original number of susceptibles in work matrix	
	double suscept;				// number of susceptibles in the case of the work matrix, used to save the weight of the play matrix
								// 
	double distance;			// the distance between two wards
	int A;						// Age index...
}to_link;

typedef struct from_link{
	struct from_link *next;
	struct to_link *ref;
}from_link;


// NETWORK BUILDING STUFF
//node *CreateHeaderNode();
//node *CreateNode(node *net,int label,int x,int y,char *name);
//node *CreateNode(int label);
//to_link *CreateListHeader(node *net);
//to_link *CreateLink(node from, node to,double weight);

void RemoveGraph(network *net);
//void RemoveLinks(to_link *l);
void RemoveFromList(from_link *l);
//void RemoveSingleNode(node *n);

network *BuildWardsNetwork(parameters *par);
network *BuildWardsNetworkDistance(parameters *par);
network *BuildWardsNetworkDistanceIdentifiers(parameters *par);
int ApplyStaticDistanceCutoff(network *net, parameters *par);
void FillInGaps(network *net);

void BuildPlayMatrix(network *net,parameters *par);
void RecalculatePlayDenominatorDay(network *net, parameters *par);
void RecalculateWorkDenominatorDay(network *net, parameters *par);


void BuildWeekendMatrix(network *net,parameters *par);
void RescalePlayMatrix(network *net,parameters *par);
void RescaleWeekendMatrix(network *net,parameters *par);
void ResetPlayMatrix(network *net);
void ResetWeekendMatrix(network *net);
void ResetWorkMatrix(network *net);
void ResetEverything(network *net,parameters *par);

void TestPlayMatrix(network *net);
void TestWeekendMatrix(network *net);
void MovePopulationFromPlayToWork(network *net, parameters *par,gsl_rng *r);

void TestNetwork(network *net);
void RewireNetwork(node *list);

int **InitialiseInfections(network *net);
int **InitialisePlayInfections(network *net);
void FreeInfections(int **p);

void SeedInfectionAtRandomLink(network *net,parameters *par, 
							   gsl_rng *r,char C,int **inf);
void InfectAdditionalSeeds(network *net, parameters *par, int **inf,int **pinf,int t);
void LoadAdditionalSeeds(char *fname);



void SeedInfectionAtNode(network *net, parameters *par, int node_seed, int **inf, int **pinf);
void ClearAllInfections(network *net, int **inf, int **pinf);

parameters *InitialiseParameters();

void ReadParametersFile(parameters *par, char *fname,int lineno);

void SetInputFileNames(int choice,parameters *par);

void RunModel(network *net, parameters *par, int **inf, int **playinf, gsl_rng *r, int *to_seed, int s);

void GetMinMaxDistances(network *net, double *max, double *min);

void Iterate(network *net, int **inf, int **playinf, parameters *par, gsl_rng *r,int t);

void IterateWeekend(network *net, int **inf, int **playinf, parameters *par, gsl_rng *r,int t);


int ReadDoneFile(char *fname,int *nodes_seeded);

FILE **OpenFiles();
void CloseFiles(FILE **files);

FILE **OpenWardFiles(parameters *par,int to_track[20]);
void CloseWardFiles(FILE **files,int *to_track);
void OutputWardData(FILE **files,int *to_track,network *net,int **winf,int **pinf, int t);


int ExtractData(network *net,int **inf,int **pinf, int t, FILE **files);
int ExtractDataForGraphicsToFile(network *net,int **inf,int **pinf,FILE *outF);

double Rate2Prob(double R);

void ExtractPlayMatrix(network *net);


void VaccinateWards(network *net,int *wardsRA,int **inf, int **pinf, size_t *vac,parameters *par);
void VaccinateWards2(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf, size_t *vac,parameters *par);
void VaccinateLondon(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par);
void VaccinateCities(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par);
void VaccinateAll(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par);
void VaccinateSameID(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par);
void VaccinateCounty(network *net,double *RiskRA,size_t *SortRA,int **inf,int **pinf,size_t *vac,parameters *par);

int IsSameCounty(char *id1,char *id2);



void Vaccinate1(network *net, int iward, int **inf, int **pinf, size_t *vac,parameters *par);
void FindWardsToVaccinate(network *net,size_t *wardsRA,int **inf,int **pinf,parameters *par);
void FindWardsToVaccinate2(network *net,double RiskRA[MAXSIZE], size_t *wardsRA, 
						   int **inf, int **pinf, parameters *par);
void CalculateWardRisk(network *net, double *RiskRA, int **inf, int **pinf,parameters *par);
int compare_doubles (const double * a, const double * b);


void FindNeighboursToVaccinate(network *net,int iward,size_t *wardsRA, parameters *par);
int IsWardInfected(node ward,int iward,int **inf,int **pinf,parameters *par);
int TotalWardInfectious(node ward,int iward,int **inf,int **pinf,parameters *par);
size_t HowManyVaccinated(size_t *vac);


void LinkControlMeasures(network *net, int **inf, int **pinf, char cutFname);
void HaltMovementsInLink(network *net, int ilink, int **inf, int **pinf, double prop);


int ImportInfection(network *net, int **inf, int **playinf, parameters *par, gsl_rng *r);
#endif
