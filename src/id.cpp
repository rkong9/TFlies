#include <iostream>
#include <vector>
#include <cmath>
#include "id.hpp"

const static uint64_t lens[20] = {
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000,
    1000000000,
    10000000000,
    100000000000,
    1000000000000,
    10000000000000,
    100000000000000,
    1000000000000000,
    10000000000000000,
    100000000000000000,
    1000000000000000000,
};

int digits10(int num) {
  if (num == 0) {
    return 1;
  } else {
    num = std::abs(num);
    return static_cast<int>(std::log10(num)) + 1;
  }
}

SID SID::getParentID() {
  // return 
  std::string currID = std::to_string(mID);
  size_t markIndex(0);
  int nums(0);
  while(markIndex < currID.length()) {
    int nums = currID[markIndex] - '0';
    markIndex += nums + 1;
  }

  if (markIndex != currID.length() || markIndex == nums + 1) {
    return SID(10);
  }
  uint64_t p_id = std::atoi(currID.substr(0, markIndex - nums - 1).c_str());

  return SID(p_id);
}

SID SID::createNewID(int subIndex) {
  return SID::createNewID(*this, subIndex);
}

SID SID::createNewID(SID sid, int subIndex) {
  if (subIndex <= 0 || !sid.isValid()) {
    return SID(0);
  }
  int digits = digits10(subIndex);
  if (digits > 9) {
    return SID(0);
  }
  uint64_t offset = std::pow(10, digits);
  uint64_t newID = sid.getID() * offset * 10;
  newID += digits * offset + subIndex;
  return SID(newID);
}

int SID::assetID(uint64_t id) {
  if (id == 0) {
    return -1;
  }
  std::string currID = std::to_string(id);
  size_t markIndex(0);
  int nums(0);
  while(markIndex < currID.length()) {
    int nums = currID[markIndex] - '0';
    markIndex += nums + 1;
  }

  if (markIndex != currID.length()) {
    return -1;
  }
  return 0;
}

std::vector<SID> SID::getPath() {
  std::vector<SID> vPath;
  do {
    if (mID == 0) {
      break;
    }

    uint64_t tempID(mID);
    vPath.clear();
    for (int i = digits10(tempID); i > 0;) {
      int subLen = i - 1;
      if (subLen <= 0) {
        break;
      }
      int bits = tempID / lens[subLen];
      i = i - bits - 1;
      if (i > 0) {
        tempID = tempID % lens[i];
      } else if (i < 0){
        break;
      }
      vPath.push_back(mID / lens[i]);
    }

  } while(0);
  return vPath;
}

int SID::getNodeLevel() {
  int level(0);

  std::string currID = std::to_string(mID);
  size_t markIndex(0);
  int nums(0);
  while(markIndex < currID.length()) {
    int nums = currID[markIndex] - '0';
    level++;
    markIndex += nums + 1;
  }

  if (markIndex != currID.length()) {
    return 0;
  }
  return level;
}
