#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

//TODO Copy this to project2-olsr.cc
int SingleExperiment(int seed, int nSources, int mobilitySpeed, int mode) {
  return 0;
}

//Used as a wrapper function around the project2.olsr.cc main method when
//doing large scale testing with various parameters
//Outputs a csv file which is fairly descriptive - each value is an average
int main1 (int argc, char *argv[]) //TODO remove the 1 to make this work
{
	int repeatedTrials 	= 5;  //how many times each experiment is seeded and executed

	int mobility 				= 10; //speed of nodes in m/s
	int mobilityInc 		= 1;  //distance between trials
	int mobilityMax 		= 20; //last value for trial

	int nSources 				= 20; //number of sources of flows
	int nSourcesInc 		= 1;  //distance between trials
	int nSourcesMax 		= 30; //last value for trial

	//Set up file for Output
	ofstream myfile;
  myfile.open ("results.csv");
	myfile << "Results\n";

	//Changing Sources/flow count with constant mobility speed
	for (int mode = 0; mode < 5; mode++) {
		//Print heading
		myfile << "\n" << (mode == 0 ? "Plain OLSR" : ("Modification " + std::to_string(mode))) << "\n";

		//Sources
		myfile << "nSources,";
		for (nSources = 1; nSources <= nSourcesMax; nSources+=nSourcesInc) {
			myfile << nSources << ",";
		}
		myfile << "\nThroughput,";
		//Get results and average;
		for (nSources = 1; nSources <= nSourcesMax; nSources+=nSourcesInc) {
			int result = 0;
			for (int seed = 0; seed < repeatedTrials; seed++) {
				result += SingleExperiment(seed, nSources, mobility, mode);
			}
			myfile << result/repeatedTrials << ",";
		}

		//Mobility
		myfile << "\nCBR (Mbps),";
		for (mobility = 1; mobility <= mobilityMax; mobility+=mobilityInc) {
			myfile << mobility << ",";
		}
		myfile << "\nThroughput,";
		nSources = 20; 									//reset number of Sources for the mobility tests
		for (mobility = 1; mobility <= mobilityMax; mobility+=mobilityInc) {
			int result = 0;
			for (int seed = 0; seed < repeatedTrials; seed++) {
				result += SingleExperiment(seed, nSources, mobility, mode);
			}
			myfile << result/repeatedTrials << ",";
		}
		myfile << "\n\n";
	}
	myfile.close();
}
