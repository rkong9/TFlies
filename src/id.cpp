#include <iostream>
#include <vector>
#include <cmath>
#include "id.hpp"
#include "logger.hpp"

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

int digits10(int64_t num) {
  if (num == 0) {
    return 1;
  } else {
    num = std::abs(num);
    return static_cast<int>(std::log10(num)) + 1;
  }
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

void SID::parse() {
  bool valid(false);
  int level(-1), subIndex(-1);
  int64_t parentID(-1);
  do {
    if (mID == 0) { // root id
      valid = true;
      level = 0;
      subIndex = 0;
      break;
    } else if (mID < 0) {
      valid = false;
      break;
    }

    std::string currID = std::to_string(mID);
    size_t markIndex(0);
    int nums(0);
    int level_temp(0);
    while(markIndex < currID.length()) {
      nums = currID[markIndex] - '0';
      level_temp++;
      markIndex += nums + 1;
    }

    if (markIndex == currID.length() && nums != 0) {
      subIndex = std::atoi(currID.substr(currID.length() - nums).c_str());
      if (digits10(subIndex) == nums) {
        parentID = std::atol(currID.substr(0, markIndex - nums - 1).c_str());
        valid = true;
        level = level_temp;
      } else {
        subIndex = -1;
      }
    } else {
      pLogger->warn("invalid id:{}, markIndex:{}, nums:{}, idLen:{}",
        mID, markIndex, nums, currID.length());
    }
  } while(0);
  mValid = valid;
  mParentID = parentID;
  mSubIndex = subIndex;
  mLevel = level;
  pLogger->trace("parse id:{}, parentID:{}, valid:{}, subIndex:{}, level:{}",
    mID, mParentID, mValid, mSubIndex, mLevel);
}

int64_t SID::getParentID(int64_t id) {
  SID temp(id);
  return temp.getParentID().getID();
}

bool SID::assertID(int64_t id) {
  SID temp(id);
  return temp.isValid();
}

int SID::getSubIndex(int64_t id) {
  SID temp(id);
  return temp.getSubIndex();
}

int SID::getNodeLevel(int64_t id) {
  SID temp(id);
  return temp.getNodeLevel();
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
