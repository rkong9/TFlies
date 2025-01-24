#ifndef _SID_HPP_
#define _SID_HPP_

#include <iostream>
#include <vector>

class SID {
public:
  SID() : mID(-1) {
    mValid = (assetID(mID) == 0);
  }
  SID(int64_t id) : mID(id) {
    mValid = (assetID(mID) == 0);
  }
  ~SID() {}
  bool isValid() {return mValid;}
  SID getParentID();
  SID createNewID(int subIndex);
  static SID createNewID(SID sID, int subIndex);
  int getSubIndex() const;
  int64_t getID() const {return mID;}
  int getNodeLevel() const;
  static int assetID(int64_t sid);
  std::vector<SID> getPath() const;

private:
  int64_t mID;
  bool mValid;
};

#endif
