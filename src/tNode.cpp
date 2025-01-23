#include "tNode.hpp"
#include <chrono>

int64_t getCurrentTimeMs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    return millis.count();
}

void TNode::setData(const Item &item) {
  mData = item;
}

void TNode::setID(const SID &sid) {
  msID = sid;
}

const Item &TNode::getData() {
  return mData;
}

int64_t TNode::getID() {
  return msID.getID();
}

int TNode::getLevel() {
  return msID.getNodeLevel();
}

const std::shared_ptr<TNode> TNode::createSubNode(Item &item, int index) {
  SID subSID(0);
  int subNodes = static_cast<int>(mqSubTNode.size());
  std::shared_ptr<TNode> pSubNode(nullptr);
  if (index > 0 && index <= subNodes) {
    mqSubTNode[index]->setData(item);
    pSubNode = mqSubTNode[index];
  } else {
    subSID = msID.createNewID(index + 1);
    item.createTime = getCurrentTimeMs();
    pSubNode = std::make_shared<TNode>(item, subSID);

    if (index <= 0) {
      mqSubTNode.push_back(pSubNode);
    } else {
      mqSubTNode.resize(index + 1);
      for (int i = subNodes; i < index + 1; i++) {
        mqSubTNode[i] = nullptr;
      }
      mqSubTNode[index] = pSubNode;
    }
  }
  return pSubNode;
}

void TNode::backUpdate(const Item &item) {
  if (mData.updateTime < item.updateTime) {
    mData.updateTime = item.updateTime;
  }
  mpParent->backUpdate(mData);
}
