#include <iostream>
#include <vector>
#include <cmath>
#include "id.hpp"
#include "logger.h"

const static int64_t lens[20] = {
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
  if (mID == 0) {
    return SID(-1);
  }

  std::string currID = std::to_string(mID);
  size_t markIndex(0);
  int nums(0);
  while(markIndex < currID.length()) {
    nums = currID[markIndex] - '0';
    markIndex += nums + 1;
  }

  pLogger->trace("sid:{}, markI:{}, nums:{}, idLen:{}",
      mID, markIndex, nums, currID.length());
  if (markIndex != currID.length()) {
    return SID(-1);
  } else if (markIndex == nums + 1) {
    pLogger->debug("parent id is root");
    return SID(0);
  }

  int64_t p_id = std::atoi(currID.substr(0, markIndex - nums - 1).c_str());
  return SID(p_id);
}

SID SID::createNewID(int subIndex) {
  return SID::createNewID(*this, subIndex);
}

SID SID::createNewID(SID sid, int subIndex) {
  if (subIndex < 0 || !sid.isValid()) {
    pLogger->warn("create new id failed, sid:{}, subIndex:{}", sid.getID(), subIndex);
    return SID(-1);
  }
  int digits = 1;
  if (subIndex > 0) {
    digits = digits10(subIndex);
  }

  if (digits > 9) {
    pLogger->warn("subIndex is overrange, sid:{}, subIndex:{}", sid.getID(), subIndex);
    return SID(-1);
  }
  int64_t offset = std::pow(10, digits);
  int64_t newID = sid.getID() * offset * 10;
  newID += digits * offset + subIndex;
  return SID(newID);
}

int SID::getSubIndex() const {
  int subIndex(0);
  if (mID == 0) {
    return -1;
  }

  std::string currID = std::to_string(mID);
  size_t markIndex(0);
  int nums(0);
  while(markIndex < currID.length()) {
    nums = currID[markIndex] - '0';
    markIndex += nums + 1;
  }

  if (markIndex != currID.length()) {
    pLogger->warn("invalid id:{}, markIndex:{}, nums:{}, idLen:{}",
      mID, markIndex, nums, currID.length());
    subIndex = -1;
  } else {
    subIndex = std::atoi(currID.substr(currID.length() - nums).c_str());
  }

  return subIndex;
}

int SID::assetID(int64_t id) {
  if (id == 0) { // root id
    return 0;
  } else if (id < 0) {
    return -1;
  }

  std::string currID = std::to_string(id);
  size_t markIndex(0);
  int nums(0);
  while(markIndex < currID.length()) {
    nums = currID[markIndex] - '0';
    markIndex += nums + 1;
  }

  if (markIndex != currID.length()) {
    pLogger->warn("invalid id:{}, markIndex:{}, nums:{}, idLen:{}",
      id, markIndex, nums, currID.length());
    return -1;
  }
  return 0;
}

std::vector<SID> SID::getPath() const {
  std::vector<SID> vPath;
  do {
    if (mID == 0 || !mValid) {
      break;
    }

    int64_t tempID(mID);
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

int SID::getNodeLevel() const {
  int level(0);
  if (mID <= 0) {
    return 0;
  }

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
