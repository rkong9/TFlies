#ifndef _T_NODE_HPP_
#define _T_NODE_HPP_

#include <list>
#include <deque>
#include <memory>
#include <mutex>
#include <string>

#include "dataType.hpp"
#include "id.hpp"

class TNode
{
public:
  TNode() : msID(-1) {
    mStatus = 0;
  }
  TNode(const Item& data, const SID &sid) : msID(sid), mData(data) {
    mData.taskID = sid.getID();
    mStatus = 0;
  }
  ~TNode() {}
  void setData(const Item &item);
  void setID(const SID &sid);
  void setParentNode(std::shared_ptr<TNode> &pParent);
  const std::shared_ptr<TNode> &getParentNode() {
    return mpParent;
  }

  const Item &getData();
  int64_t getID();
  int getLevel();
  int getSubIndex();

  const std::shared_ptr<TNode> createSubNode(Item &item, int index = -1);
  void backUpdate(const Item &item);
  int setSubNode(std::shared_ptr<TNode> &node);
  int setTimePieces(std::shared_ptr<TPieces> &pieces);
  // void deleteSelfFromTree();

  int start();
  int stop();
  int done();
  int reopen();

private:
  int updateValue();

public:
  std::deque<std::shared_ptr<TNode>> mqSubTNode;
  std::deque<std::shared_ptr<TPieces>> mqPieces;
  int mStatus; // 0 normal, 1 updated, -1 deleted

private:
  SID msID;
  Item mData;
  TPieces mpCurrPieces;
  std::mutex mlSubMutex;
  std::shared_ptr<TNode> mpParent;
};

typedef std::shared_ptr<TNode> TNodePtr;

#endif

