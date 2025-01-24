#ifndef _SID_HPP_
#define _SID_HPP_

#include <iostream>
#include <vector>

class SID {
public:
  SID() : mID(-1) {
    parse();
  }
  SID(int64_t id) : mID(id) {
    parse();
  }
  ~SID() {}
  void setValue(int64_t id) {
    mID = id;
    parse();
  }

  bool isValid() {return mValid;}
  SID getParentID() {return SID(mParentID);};
  SID createNewID(int subIndex);
  static SID createNewID(SID sID, int subIndex);
  int64_t getID() const {return mID;}
  int getSubIndex() const {return mSubIndex; };
  int getNodeLevel() const { return mLevel; };
  static int64_t getParentID(int64_t id);
  static bool assertID(int64_t id);
  static int getSubIndex(int64_t id);
  static int getNodeLevel(int64_t id);
  std::vector<SID> getPath() const;

private:
  void parse();

private:
  int64_t mID;
  int64_t mParentID;
  bool mValid;
  int mLevel;
  int mSubIndex;
};

extern int digits10(int64_t num);

#endif
