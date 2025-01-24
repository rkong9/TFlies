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
  TNode() : msID(-1) {}
  TNode(const Item& data, const SID &sid) : msID(sid), mData(data) {
    mData.taskID = sid.getID();
  }
  ~TNode() {}
  void setData(const Item &item);
  void setID(const SID &sid);
  const Item &getData();
  int64_t getID();
  int getLevel();
  int getSubIndex();

  const std::shared_ptr<TNode> createSubNode(Item &item, int index = -1);
  void backUpdate(const Item &item);
  int setSubNode(std::shared_ptr<TNode> &node);

  int start();
  int stop();
  int down();
  int reopen();

private:
  int updateValue();

public:
  std::deque<std::shared_ptr<TNode>> mqSubTNode;
  std::list<std::shared_ptr<TPieces>> mlPieces;

private:
  SID msID;
  Item mData;
  TPieces mCurrPieces;
  std::mutex mlSubMutex;
  std::shared_ptr<TNode> mpParent;
};

typedef std::shared_ptr<TNode> TNodePtr;

#endif

