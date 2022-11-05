#include <iostream>
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
using namespace std;

struct info {
  unsigned long long addr;
  //1 for T 0 for NT
  int taken;
  //something else
};

//global variables
vector<info> allData;
string everythingOutput = "";
int tableSizes[7] = {16, 32, 128, 256, 512, 1024, 2048};
unsigned long long numBranches = 0;

void alwaysTaken() {
  unsigned long long correct = 0;
  for(int i = 0; i < allData.size(); i++) {
    if(allData[i].taken == 1) correct++;
  }
  everythingOutput += to_string(correct) + ", " + to_string(numBranches) + ";\n";
}

void neverTaken() {
  unsigned long long correct = 0;
  for(int i = 0; i < allData.size(); i++) {
    if(allData[i].taken == 0) correct++;
  }
  everythingOutput += to_string(correct) + ", " + to_string(numBranches) + ";\n";
}

void bimodalSingle() {
  for(int i = 0; i < 7; i++) {
    unsigned long long correct = 0;
    //selector counter for i sets
    int currentTableSize = tableSizes[i];
    int counter[currentTableSize];
    for(int k = 0; k < currentTableSize; k++) {
      counter[k] = 1;
    }
    for(int j = 0; j < allData.size(); j++) {
      int index = allData[j].addr % currentTableSize;
      if(counter[index] == allData[j].taken) correct++;
      //case where prediction is wrong
      else {
	if(counter[index] == 1) counter[index] = 0;
	else counter[index] = 1;
      }
    }
    everythingOutput += to_string(correct) + ", " + to_string(numBranches) + "; ";
  }
  everythingOutput += "\n";
}

void bimodalSaturated() {
  for(int i = 0; i < 7; i++) {
    unsigned long long correct = 0;
    int currentTableSize = tableSizes[i];
    int counter[currentTableSize];
    for(int k = 0; k < currentTableSize; k++) {
      counter[k] = 3;
    }
    for(int j = 0; j < allData.size(); j++) {
      int index = allData[j].addr % currentTableSize;
      //if prediction is correct reinforce the correct prediction
      //(if branch wasn't taken and state was 1, make it 0)
      //(if branch was taken and state was 2, make it 3)
      if(((counter[index] >> 1) & 1) == allData[j].taken) {
	correct++;
	if(counter[index] == 1) counter[index] = 0;
	else if(counter[index] == 2) counter[index] = 3;
      }
      //cases where prediction is wrong
      else {
        if(counter[index] >= 2) counter[index] -= 1;
	else if(counter[index] <= 1) counter[index] += 1;
      }
    }
    everythingOutput += to_string(correct) + ", " + to_string(numBranches) + "; ";
  }
  everythingOutput += "\n";
}

void gshare() {
  for(int i = 3; i <= 11; i++) {
    unsigned long long correct = 0;
    //selector counter for each of the 2048 sets
    int counter[2048];
    for(int k = 0; k < 2048; k++) {
      counter[k] = 3;
    }
    //we will need history1s to make sure history value gets updated with only least significant i bits being measured
    int history1s = pow(2, i) - 1;
    //history starts at 0 (no branch previously taken in global history)
    int history = 0;
    for(int j = 0; j < allData.size(); j++) {
      int index = (allData[j].addr ^ (history & history1s)) % 2048;
      //if prediction is correct reinforce the correct prediction
      //(if branch wasn't taken and state was 1, make it 0)
      //(if branch was taken and state was 2, make it 3)
      if(((counter[index] >> 1) & 1) == allData[j].taken) {
	correct++;
	if(counter[index] == 1) counter[index] = 0;
	else if(counter[index] == 2) counter[index] = 3;
      }
      //cases where prediction is wrong
      else {
        if(counter[index] >= 2) counter[index] -= 1;
	else if(counter[index] <= 1) counter[index] += 1;
      }
      //update history by left shifting history by 1 bit and inserting a 0 in LSB for not taken and 1 in LSB for taken
      if(allData[j].taken == 1) history = (history << 1) + 1;
      else if(allData[j].taken == 0) history = (history << 1);
    }
    everythingOutput += to_string(correct) + ", " + to_string(numBranches) + "; ";
  }
  everythingOutput += "\n";
}

void tournament() {
  //has selector states for gshare and bimodal and tournament separately
  //selector counter for 2048 sets
  int bCounter[2048];
  int gCounter[2048];
  int tCounter[2048];
  //each index needs it's own predictor state
  for(int k = 0; k < 2048; k++) {
    gCounter[k] = 3;
    bCounter[k] = 3;
    //2 or 3, bimodal; 0 or 1, gshare. default to prefer gshare
    tCounter[k] = 0;
  }
  //stores predictions of bimodal and gshare separately
  int* bimodalPredictions = new int[allData.size()];
  int* gsharePredictions = new int[allData.size()];
  //we will need history 1s to make sure history value gets updated with only least significant 11 bits being measured
  int history1s = pow(2, 11) - 1;
  //history starts at 0 (no branch previously taken in global history)
  int history = 0;
  int correct = 0;
  for(int j = 0; j < allData.size(); j++) {
    int indexBimodal = allData[j].addr % 2048;
    int indexGshare = (allData[j].addr ^ (history & history1s)) % 2048;
    int indexT = allData[j].addr % 2048;
    //0 for not taken 1 for taken
    bimodalPredictions[j] = (bCounter[indexBimodal] >> 1) & 1;
    gsharePredictions[j] = (gCounter[indexGshare] >> 1) & 1;
    //if branch not taken
    if(allData[j].taken == 0) {
      if(gCounter[indexGshare] >= 1) {
	gCounter[indexGshare]--;
      }
      if(bCounter[indexBimodal] >= 1) {
	bCounter[indexBimodal]--;
      }
    }
    //else if branch taken
    else if(allData[j].taken == 1) {
      if(gCounter[indexGshare] <= 2) {
	gCounter[indexGshare]++;
      }
      if(bCounter[indexBimodal] <= 2) {
	bCounter[indexBimodal]++;
      }
    }
    //update history
    if(allData[j].taken == 1) history = (history << 1) + 1;
    else if(allData[j].taken == 0) history = (history << 1);
    //=====================================================
    //compare when gshare and bimodal were correct, incorrect
    //mess with state table
    //=====================================================
    
    //gshare and bimodal have same prediction
    if(gsharePredictions[j] == bimodalPredictions[j]) {
      if(gsharePredictions[j] == allData[j].taken) correct++;
    }
    //using bimodal    
    else if(tCounter[indexT] >= 2) {
      //if bimodal is right we can prefer bimodal state
      if(bimodalPredictions[j] == allData[j].taken) {
	correct++;
	if(tCounter[indexT] <= 2) tCounter[indexT]++;
      }
      //if bimodal is not right we can prefer gshare state
      else {
	if(tCounter[indexT] >= 1) tCounter[indexT]--;
      }
    }
    //using gshare
    else if(tCounter[indexT] <= 1) {
      //if gshare is right we can prefer gshare state
      if(gsharePredictions[j] == allData[j].taken) {
	correct++;
	if(tCounter[indexT] >= 1) tCounter[indexT]--;
      }
      //if gshare is not right we can prefer bimodal state
      else {
	if(tCounter[indexT] <= 2) tCounter[indexT]++;
      }
    }    
  }
  everythingOutput += to_string(correct) + ", " + to_string(numBranches) + ";\n";
  //free space we made on heap
  //I only declared these on heap because I ran out of room on stack
  delete bimodalPredictions;
  delete gsharePredictions;
}

void btb() {
  //pass
}

void output(string & outputFileName) {
  ofstream outputFile;
  outputFile.open(outputFileName);
  outputFile << everythingOutput;
  outputFile.close();
}

int main(int argc, char *argv[]) {
  unsigned long long address;
  string behavior;
  unsigned long long target;
  string fileName = argv[1];
  //cout << fileName << endl;
  string outputFileName = argv[2];
  //cout << outputFileName << endl;
  ifstream infile("traces/"+fileName);
  if(!infile) {
    cout << "cannot locate file" << endl;
    exit(1);
  }
  while(infile >> std::hex >> address >> behavior >> std::hex >> target) {
    numBranches++;
    info thisInfo;
    //cout << address;
    thisInfo.addr = address;
    if(behavior == "T") {
      //cout << " -> taken, ";
      thisInfo.taken = 1;
    }
    else {
      //cout << " -> not taken, ";
      thisInfo.taken = 0;
    }
    //cout << "target=" << target << endl;
    allData.push_back(thisInfo);
  }
  //cout << allData.size() << endl;
  infile.close();
  alwaysTaken();
  neverTaken();
  bimodalSingle();
  bimodalSaturated();
  gshare();
  tournament();
  output(outputFileName);
  return 0;
}




