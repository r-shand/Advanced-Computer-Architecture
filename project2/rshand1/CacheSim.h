#ifndef _H_CACHE_SIM_
#define _H_CACHE_SIM_
#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
using namespace std;
class CacheSim {
  public:
    struct Info {
      int addr;
      char behavior;
    };
    vector<Info> allData;
    string everythingOutput;
    CacheSim();
    ~CacheSim();
    void parseInput(string);
    void output(string &);
    void directMapped();
    void setAssociative();
    void fullyAssociativeLRU();
    void fullyAssociativeHotCold();
    void setAssociativeNoAllocOnWriteMiss();
    void setAssociativePrefetching();
    void prefetchOnMiss();
    //binary tree stuff
    int getParent(int);
    int getLeftChild(int);
    int getRightChild(int);
    bool hasParent(int);
};
#endif
