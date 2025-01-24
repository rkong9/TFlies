#include <chrono>
#include "tNode.hpp"
#include "logger.h"

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
  mData.taskID = sid.getID();
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
  if (index >= 0 && index < subNodes) {
    mqSubTNode[index]->setData(item);
    pSubNode = mqSubTNode[index];
    pLogger->trace("update node:{} values, id:{}", index, subSID.getID());
  } else {
    if (index < 0) { // direct insert
      subSID = msID.createNewID(subNodes);
      if (subSID.isValid()) {
        item.createTime = getCurrentTimeMs();
        item.updateTime = item.createTime;
        item.taskID = subSID.getID();
        pSubNode = std::make_shared<TNode>(item, subSID);
        mqSubTNode.push_back(pSubNode);
        pLogger->trace("dircet insert new node, id:{}", subSID.getID());
      } else {
        pLogger->warn("dircet insert new node failed, id:{}", subSID.getID());
      }
    } else {
      subSID = msID.createNewID(index);
      if (subSID.isValid()) {
        item.createTime = getCurrentTimeMs();
        item.updateTime = item.createTime;
        item.taskID = subSID.getID();
        pSubNode = std::make_shared<TNode>(item, subSID);
        mqSubTNode.resize(index);
        for (int i = subNodes; i < index; i++) {
          mqSubTNode[i] = nullptr;
        }
        mqSubTNode[index] = pSubNode;
        pLogger->trace("resize nodes to {}, id:{}", index, subSID.getID());
      } else {
        pLogger->warn("resize nodes to {} failed, id:{}", index, subSID.getID());
      }
    }
  }
  return pSubNode;
}

void TNode::backUpdate(const Item &item) {
  if (mData.updateTime < item.updateTime) {
    mData.updateTime = item.updateTime;
  }
  if (mpParent && SID::assetID(mpParent->getID() == 0)) {
    mpParent->backUpdate(mData);
  }
}

int TNode::getSubIndex() {
  return msID.getSubIndex();
}

int TNode::setSubNode(std::shared_ptr<TNode> &node) {
  if (!node) {
    pLogger->warn("node:{} insert invalid node", msID.getID());
    return -1;
  }

  int subIndex = node->getSubIndex();
  int subNodes = mqSubTNode.size();
  if (subIndex >= 0 && subIndex < mqSubTNode.size()) { // update value
    mqSubTNode[subIndex - 1] = node;
  } else if (subIndex >= mqSubTNode.size()){
    mqSubTNode.resize(subIndex + 1);
    for (int i = subNodes; i < mqSubTNode.size(); i++) {
      mqSubTNode[i] = nullptr;
    }
    mqSubTNode[subIndex] = node;
  } else {
    pLogger->warn("invalid subindex:{}", subIndex);
    return -1;
  }

  return 0;
}
