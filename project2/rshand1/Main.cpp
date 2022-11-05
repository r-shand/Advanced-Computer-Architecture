#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include "CacheSim.h"
using namespace std;

int main(int argc, char *argv[]) {
  string fileName = argv[1];
  //cout << fileName << endl;
  string outputFileName = argv[2];
  //cout << outputFileName << endl;
  CacheSim c;
  c.parseInput(fileName);
  c.directMapped();
  c.setAssociative();
  c.fullyAssociativeLRU();
  c.fullyAssociativeHotCold();
  c.setAssociativeNoAllocOnWriteMiss();
  c.setAssociativePrefetching();
  c.prefetchOnMiss();
  c.output(outputFileName);
  return 0;
}



