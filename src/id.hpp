#ifndef _SID_HPP_
#define _SID_HPP_

#include <iostream>
#include <vector>

class SID {
public:
  SID() : mID(10) {}
  SID(uint64_t id) : mID(id) {
    mValid = (assetID(mID) == 0);
  }
  ~SID() {}
  bool isValid() {return mValid;}
  SID getParentID();
  SID createNewID(int subIndex);
  static SID createNewID(SID sID, int subIndex);
  uint64_t & getSubIndex();
  uint64_t getID() {return mID;}
  int getNodeLevel();
  static int assetID(uint64_t sid);
  std::vector<SID> getPath();

private:
  uint64_t mID;
  bool mValid;
};

#endif
