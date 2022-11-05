#include "CacheSim.h"
#include <string>
#include <fstream>
#include <iostream>
#include <cmath>
#include <math.h>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <queue>
using namespace std;

CacheSim::CacheSim() {
}

CacheSim::~CacheSim() {
}

int CacheSim::getParent(int index) {
  return (index-1)/2;
}

int CacheSim::getLeftChild(int index) {
  return index*2+1;
}

int CacheSim::getRightChild(int index) {
  return index*2+2;
}

bool CacheSim::hasParent(int index) {
  if(getParent(index) >= 1) return true;
  return false;
}

void CacheSim::directMapped() {
  int sizes[] = {1, 4, 16, 32};
  for(int i = 0; i < 4; i++) {
    //cache params
    int currentSize = sizes[i] * 1024;
    int sets = currentSize / 32;
    int indexBits = log2(sets);
    int index = 0;
    int offsetBits = log2(32);
    int tagBits = 32 - indexBits - offsetBits;
    int tag = 0;
    //the cache
    unsigned long long *cache = new unsigned long long[currentSize];
    for(int j = 0; j < currentSize; j++) {
      cache[j] = 0;
    }
    //track number of hits
    int hits = 0;
    for(int j = 0; j < this->allData.size(); j++) {
      int address = allData.at(j).addr;
      index = (address >> offsetBits) % sets;
      tag = address >> (indexBits + offsetBits);
      if(cache[index] == tag) {
	hits++;
      }
      else if(cache[index] != tag) {
	cache[index] = tag;
      }
    }
    this->everythingOutput += to_string(hits) + "," + to_string(this->allData.size()) + "; ";
  }
  this->everythingOutput += "\n";
}

void CacheSim::setAssociative() {
  int associativity[] = {2, 4, 8, 16};
  for(int i = 0; i < 4; i++) {
    //cache params
    int currentSize = 16 * 1024;
    int blocks = currentSize / 32;
    int sets = currentSize / (associativity[i] * 32);
    int indexBits = log2(sets);
    int index = 0;
    int offsetBits = log2(32);
    int tagBits = 32 - indexBits - offsetBits;
    int tag = 0;
    //the cache
    //use array of vectors
    //element in last position of associativity is LRU
    //i.e. in 4 way set associatitve in index 3 of vector is LRU for each set
    vector<unsigned long long> cache[sets];
    for(int j = 0; j < sets; j++) {
      //insert 0's into every set to begin
      cache[j].insert(cache[j].begin(), associativity[i], 0);
    }
    //track number of hits
    int hits = 0;
    //loop through all instructions
    for(int j = 0; j < this->allData.size(); j++) {
      //get address of instruction
      int address = allData.at(j).addr;
      //get set index for address
      index = (address >> offsetBits) % sets;
      //get tag of address
      tag = address >> (indexBits + offsetBits);
      //did we get a cache hit or miss?
      bool hit = false;
      //loop through every block in the set
      int k = 0;
      while(k < associativity[i] && !hit) {
	//if cache hit
	if(cache[index].at(k) == tag) {
	  //indicate cache hit, increment amount of total hits
	  hit = true;
	  hits++;
	  //update LRU by removing wherever tag was in the vector and moving it to the front
	  cache[index].erase(cache[index].begin()+k);
	  cache[index].insert(cache[index].begin(), 1, tag);
	}
	k++;
      }
      //if cache miss
      if(!hit) {
	//insert tag into front of set
	cache[index].insert(cache[index].begin(), 1, tag);
	//update LRU remove least recently used element
	if(cache[index].size() > associativity[i]) {
	    cache[index].pop_back();
	}
      }
    }
    this->everythingOutput += to_string(hits) + "," + to_string(this->allData.size()) + "; ";
  }
  this->everythingOutput += "\n";
}

void CacheSim::fullyAssociativeHotCold() {
  //cache params
  int currentSize = 16 * 1024;
  int blocks = (currentSize / 32);
  int offsetBits = log2(32);
  int tagBits = 32 - offsetBits;
  int tag = 0;
  int hits = 0;
  //how many times we can split the cache into halves? 9 because lg(512) = 9
  //bits required for each successive halving
  //1+2+4+8+16+32+64+128+256 = 511
  //our binary tree will be size 511
  int arrHC[511];
  for(int i = 0; i < 511; i++) {
    arrHC[i] = 0;
  }
  //we will say if a bit is 0 then the left half is cold
  //cache
  unsigned long long *cache = new unsigned long long[blocks];
  //insert 0's into every block to begin
  for(int j = 0; j < blocks; j++) {
    cache[j] = 0;
  }
  //iterate through every instruction
  for(int j = 0; j < this->allData.size(); j++) {
    //get address of instruction
    int address = allData.at(j).addr;
    //get tag of address
    tag = address >> offsetBits;
    //did we get a cache hit or miss?
    bool hit = false;
    int k = 0;
    while(k < blocks && !hit) {
      //if cache hit
      if(cache[k] == tag) {
	//indicate cache hit, increment amount of total hits
	hit = true;
	hits++;
	//update our binary tree with hot/cold values
	//block 0 is 511 in our tree which is left most node
	int updateBit = k + 511;
	while(updateBit > 0) {
	  //if bit to be updated is divisible by 2 make left half cold because we would be accessing right node
	  //so if we are accessing block 1 node 512 which is a right leaf node, we need to make left cold because we are accessing right node.
	  if(updateBit % 2 == 0) arrHC[getParent(updateBit)] = 0;
	  //if bit to be updated is not divisible by 2 make right half is cold because we are accessing a left node
	  else arrHC[getParent(updateBit)] = 1;
	  //work up the tree to parent
	  updateBit = getParent(updateBit);
	}
	
      }
      k++;
    }
    //if cache miss
    if(!hit) {
        //find victim
        int victim = 0;
        for(int x = 0; x < 9; x++) {
	  //if left is cold we visit left child to find victim
	  if(arrHC[victim] == 0) {
	    //we can update tree values on the way down since when we replace the value it will be MRU
	    //so if we are making left node MRU we make value 1 to indicate right is cold
	    arrHC[victim] = 1;
	    victim = getLeftChild(victim);
  	  }
	  //if right is cold we visit right child to find victim
	  else {
	    //we can update tree values on the way down since when we replace the value it will be MRU
	    //so if we are making right node MRU we make value 0 to indicate left is cold
	    arrHC[victim] = 0;
	    victim = getRightChild(victim);
	  }
        }
	//now that we have retrieved our victim that we will replace, we will replace it.
	cache[victim-511] = tag;
    }
  }
  this->everythingOutput += to_string(hits) + "," + to_string(this->allData.size()) + ";\n";
}

void CacheSim::fullyAssociativeLRU() {
  //cache params
  int currentSize = 16 * 1024;
  int blocks = currentSize / 32;
  int offsetBits = log2(32);
  int tagBits = 32 - offsetBits;
  int tag = 0;
  int hits = 0;
  //cache
  vector<unsigned long long> cache;
  //insert 0's into every block to begin
  cache.insert(cache.begin(), blocks, 0);
  //iterate through every instruction
  for(int j = 0; j < this->allData.size(); j++) {
   //get address of instruction
    int address = allData.at(j).addr;
    //get tag of address
    tag = address >> offsetBits;
    //did we get a cache hit or miss?
    bool hit = false;
    //loop through every block in the cache
    int k = 0;
    while(k < blocks && !hit) {
      //if cache hit
      if(cache.at(k) == tag) {
	//indicate cache hit, increment amount of total hits
	hit = true;
	hits++;
	//update LRU by removing wherever tag was in the vector and moving it to the front
	cache.erase(cache.begin()+k);
	cache.insert(cache.begin(), 1, tag);
      }
      k++;
    }
    //if cache miss
    if(!hit) {
      //insert tag into front of cache
      cache.insert(cache.begin(), 1, tag);
      //update LRU remove least recently used element
      if(cache.size() > blocks) {
	cache.pop_back();
      }
    }
  }
  this->everythingOutput += to_string(hits) + "," + to_string(this->allData.size()) + ";\n";
}

void CacheSim::setAssociativeNoAllocOnWriteMiss() {
  int associativity[] = {2, 4, 8, 16};
  for(int i = 0; i < 4; i++) {
    //cache params
    int currentSize = 16 * 1024;
    int blocks = currentSize / 32;
    int sets = currentSize / (associativity[i] * 32);
    int indexBits = log2(sets);
    int index = 0;
    int offsetBits = log2(32);
    int tagBits = 32 - indexBits - offsetBits;
    int tag = 0;
    //the cache
    //use array of vectors
    //element in last position of associativity is LRU
    //i.e. in 4 way set associatitve in index 3 of vector is LRU for each set
    vector<unsigned long long> cache[sets];
    for(int j = 0; j < sets; j++) {
      //insert 0's into every set to begin
      cache[j].insert(cache[j].begin(), associativity[i], 0);
    }
    //track number of hits
    int hits = 0;
    //loop through all instructions
    for(int j = 0; j < this->allData.size(); j++) {
      //get instruction type of instruction
      char type = allData.at(j).behavior;
      //get address of instruction
      int address = allData.at(j).addr;
      //get set index for address
      index = (address >> offsetBits) % sets;
      //get tag of address
      tag = address >> (indexBits + offsetBits);
      //did we get a cache hit or miss?
      bool hit = false;
      //loop through every block in the set
      int k = 0;
      while(k < associativity[i] && !hit) {
	//if cache hit
	if(cache[index].at(k) == tag) {
	  //indicate cache hit, increment amount of total hits
	  hit = true;
	  hits++;
	  //update LRU by removing wherever tag was in the vector and moving it to the front
	  cache[index].erase(cache[index].begin()+k);
	  cache[index].insert(cache[index].begin(), 1, tag);
	}
	k++;
      }
      //if cache miss
      if(!hit && type != 'S') {
	//insert tag into front of set
	cache[index].insert(cache[index].begin(), 1, tag);
	//update LRU remove least recently used element
	if(cache[index].size() > associativity[i]) {
	    cache[index].pop_back();
	}
      }
    }
    this->everythingOutput += to_string(hits) + "," + to_string(this->allData.size()) + "; ";
  }
  this->everythingOutput += "\n";
}

void CacheSim::setAssociativePrefetching() {
  int associativity[] = {2, 4, 8, 16};
  for(int i = 0; i < 4; i++) {
    //cache params
    int currentSize = 16 * 1024;
    int blocks = currentSize / 32;
    int sets = currentSize / (associativity[i] * 32);
    int indexBits = log2(sets);
    int index = 0;
    int offsetBits = log2(32);
    int tagBits = 32 - indexBits - offsetBits;
    int tag = 0;
    //the cache
    //use array of vectors
    //element in last position of associativity is LRU
    //i.e. in 4 way set associatitve in index 3 of vector is LRU for each set
    vector<unsigned long long> cache[sets];
    for(int j = 0; j < sets; j++) {
      //insert 0's into every set to begin
      cache[j].insert(cache[j].begin(), associativity[i], 0);
    }
    //track number of hits
    int hits = 0;
    //loop through all instructions
    for(int j = 0; j < this->allData.size(); j++) {
      //get address of instruction
      int address = allData.at(j).addr;
      //get set index for address
      index = (address >> offsetBits) % sets;
      //get tag of address
      tag = address >> (indexBits + offsetBits);
      //did we get a cache hit or miss?
      bool hit = false;
      //loop through every block in the set
      int k = 0;
      while(k < associativity[i] && !hit) {
	//if cache hit
	if(cache[index].at(k) == tag) {
	  //indicate cache hit, increment amount of total hits
	  hit = true;
	  hits++;
	  //update LRU by removing wherever tag was in the vector and moving it to the front
	  cache[index].erase(cache[index].begin()+k);
	  cache[index].insert(cache[index].begin(), 1, tag);
	}
	k++;
      }
      //if cache miss
      if(!hit) {
	//insert tag into front of set
	cache[index].insert(cache[index].begin(), 1, tag);
	//update LRU remove least recently used element
	if(cache[index].size() > associativity[i]) {
	    cache[index].pop_back();
	}
      }
      hit = false;
      //update address to next block of memory
      address += 32;
      //get set index for new address
      index = (address >> offsetBits) % sets;
      //get tag of new address
      tag = address >> (indexBits + offsetBits);
      k = 0;
      while(k < associativity[i] && !hit) {
	//if cache hit
	if(cache[index].at(k) == tag) {
	  //indicate cache hit, increment amount of total hits
	  hit = true;
	  //update LRU by removing wherever tag was in the vector and moving it to the front
	  cache[index].erase(cache[index].begin()+k);
	  cache[index].insert(cache[index].begin(), 1, tag);
	}
	k++;
      }
      //if cache miss
      if(!hit) {
	//insert tag into front of set
	cache[index].insert(cache[index].begin(), 1, tag);
	//update LRU remove least recently used element
	if(cache[index].size() > associativity[i]) {
	    cache[index].pop_back();
	}
      }
    }
    this->everythingOutput += to_string(hits) + "," + to_string(this->allData.size()) + "; ";
  }
  this->everythingOutput += "\n";
}

void CacheSim::prefetchOnMiss() {
  int associativity[] = {2, 4, 8, 16};
  for(int i = 0; i < 4; i++) {
    //cache params
    int currentSize = 16 * 1024;
    int blocks = currentSize / 32;
    int sets = currentSize / (associativity[i] * 32);
    int indexBits = log2(sets);
    int index = 0;
    int offsetBits = log2(32);
    int tagBits = 32 - indexBits - offsetBits;
    int tag = 0;
    //the cache
    //use array of vectors
    //element in last position of associativity is LRU
    //i.e. in 4 way set associatitve in index 3 of vector is LRU for each set
    vector<unsigned long long> cache[sets];
    for(int j = 0; j < sets; j++) {
      //insert 0's into every set to begin
      cache[j].insert(cache[j].begin(), associativity[i], 0);
    }
    //track number of hits
    int hits = 0;
    //loop through all instructions
    for(int j = 0; j < this->allData.size(); j++) {
      //get address of instruction
      int address = allData.at(j).addr;
      //get set index for address
      index = (address >> offsetBits) % sets;
      //get tag of address
      tag = address >> (indexBits + offsetBits);
      //did we get a cache hit or miss?
      bool hit = false;
      //loop through every block in the set
      int k = 0;
      while(k < associativity[i] && !hit) {
	//if cache hit
	if(cache[index].at(k) == tag) {
	  //indicate cache hit, increment amount of total hits
	  hit = true;
	  hits++;
	  //update LRU by removing wherever tag was in the vector and moving it to the front
	  cache[index].erase(cache[index].begin()+k);
	  cache[index].insert(cache[index].begin(), 1, tag);
	}
	k++;
      }
      k = 0;
      //if cache miss. also we are prefetching since we missed
      if(!hit) {
	//insert tag into front of set
	cache[index].insert(cache[index].begin(), 1, tag);
	//update LRU remove least recently used element
	if(cache[index].size() > associativity[i]) {
	    cache[index].pop_back();
	}
	//update address to next block of memory
        address += 32;
        //get set index for new address
        index = (address >> offsetBits) % sets;
        //get tag of new address
        tag = address >> (indexBits + offsetBits);
	//loop through every block in set to check if our new block of memory already exists in the cache
	while(k < associativity[i] && !hit) {
	  //if new block of memory has cache hit
	  if(cache[index].at(k) == tag) {
	    //indicate cache hit, increment amount of total hits
	    hit = true;
	    //update LRU by removing wherever tag was in the vector and moving it to the front
	    cache[index].erase(cache[index].begin()+k);
	    cache[index].insert(cache[index].begin(), 1, tag);
	  }
	  k++;
        }
	//if new block of memory misses cache, we will put it in the cache
	if(!hit) {
	  cache[index].insert(cache[index].begin(), 1, tag);
	  //update LRU remove least recently used element
	  if(cache[index].size() > associativity[i]) {
	    cache[index].pop_back();
	  }
	}
      }
    }
    this->everythingOutput += to_string(hits) + "," + to_string(this->allData.size()) + "; ";
  }
  this->everythingOutput += "\n";
}

void CacheSim::parseInput(string inputFileName) {
  char behavior;
  int address;
  ifstream infile("traces/"+inputFileName);
  if(!infile) {
    cout << "cannot locate file" << endl;
    exit(1);
  }
  while(infile >> behavior >> std::hex >> address) {
    Info thisInfo;
    thisInfo.behavior = behavior;
    thisInfo.addr = address;
    this->allData.push_back(thisInfo);
  }
  infile.close();  
}

void CacheSim::output(string & outputFileName) {
  ofstream outputFile;
  outputFile.open(outputFileName);
  outputFile << everythingOutput;
  outputFile.close();
}
