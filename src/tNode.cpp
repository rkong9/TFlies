#include <chrono>
#include "tNode.hpp"
#include "logger.hpp"
#include "utils.hpp"

void TNode::setData(const Item &item) {
  mData = item;
}

void TNode::setParentNode(std::shared_ptr<TNode> &pParent) {
  mpParent = pParent;
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
      int newIndex(subNodes);
      for (size_t i = 0; i < mqSubTNode.size(); i++) {
        if (mqSubTNode[i]->mStatus < 0) {
          newIndex = i;
          pLogger->trace("find an unused index:{} in node:{}", newIndex, msID.getID());
          break;
        }
      }

      subSID = msID.createNewID(newIndex);
      if (subSID.isValid()) {
        item.createTime = getCurrentTimeMs();
        item.updateTime = item.createTime;
        item.taskID = subSID.getID();
        pSubNode = std::make_shared<TNode>(item, subSID);
        if (newIndex < subNodes) {
          mqSubTNode[newIndex] = pSubNode;
        } else {
          mqSubTNode.push_back(pSubNode);
        }
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
  if (mpParent && SID::assertID(mpParent->getID())) {
    mpParent->backUpdate(mData);
  }
}

int TNode::getSubIndex() {
  return msID.getSubIndex();
}

int TNode::exe_start() {
  if (mStatus < 0) {
    pLogger->warn("current status:{} is invalid", mStatus);
    return -1;
  }

  if (mpCurrPieces) {
    pLogger->warn("this node is already stated");
    return 1;
  }

  mpCurrPieces = std::make_shared<TPieces>();
  mpCurrPieces->begintime = getCurrentTimeMs();
  mpCurrPieces->taskID = msID.getID();
  mData.status = static_cast<uint8_t>(TaskStatus::INPROGRESS);
  return 0;
}

int TNode::exe_stop(const std::string &desc, uint8_t efficiency) {
  if (mStatus < 0) {
    return -1;
  }

  if (!mpCurrPieces) {
    pLogger->warn("this node is already stoped");
    return 1;
  }
  mStatus = 1;

  mpCurrPieces->endtime = getCurrentTimeMs();
  mpCurrPieces->serialNumber = mqPieces.size();
  mpCurrPieces->piecesID = mPieceNums.load();
  mpCurrPieces->desc = desc;
  mpCurrPieces->efficiency = efficiency;
  mpCurrPieces->status = 1;
  mqPieces.push_back(mpCurrPieces);
  mpCurrPieces = nullptr;

  int64_t costTime = mpCurrPieces->endtime - mpCurrPieces->begintime;
  mData.costTime += costTime;
  mData.updateTime = mpCurrPieces->endtime;

  return 0;
}

int TNode::start() {
  if (mStatus < 0) {
    pLogger->warn("this node is already abandon");
    return -1;
  }
  mStatus = 1;
  mData.status = static_cast<uint8_t>(TaskStatus::INPROGRESS);
}

int TNode::pause() {
  if (mStatus < 0) {
    pLogger->warn("this node is already abandon");
    return -1;
  }
  mStatus = 1;
  mData.status = static_cast<uint8_t>(TaskStatus::PAUSE);
}

int TNode::done() {
  if (mStatus < 0) {
    pLogger->warn("this node is already abandon");
    return -1;
  }
  mStatus = 1;
  mData.status = static_cast<uint8_t>(TaskStatus::DONE);

  return 0;
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

int TNode::setTimePieces(std::shared_ptr<TPieces> &pieces) {
 if (!pieces) {
   pLogger->warn("insert invalid pieces");
   return -1;
 }

 if (pieces->serialNumber < 0) {
   pLogger->warn("pieces:{} invalid invalid pieces's serialNumber", pieces->piecesID);
   return -1;
 }

 if (pieces->serialNumber >= mqPieces.size()) {
    mqPieces.resize(pieces->serialNumber);
 }

 if (mqPieces[pieces->serialNumber]) {
    pLogger->warn("task:{}, pieces:{} is collision", msID.getID(), pieces->piecesID);
    return -2;
 }

 mqPieces[pieces->serialNumber] = pieces;
 mPieceNums++;
 return 0;
}
