#include <chrono>
#include "tNode.hpp"
#include "logger.hpp"
#include "utils.hpp"

std::atomic<int64_t> TNode::mPieceNums = 0;

void TNode::setData(const Item &item, bool update) {
  mData = item;
  if (update) {
    mStatus = true;
  }
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
        int64_t nowTime = getCurrentTimeMs();
        if (item.createTime <= 0) {
          item.createTime = nowTime;
        }
        item.updateTime = nowTime;
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
  pSubNode->mStatus = 1;
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

int TNode::exe_start(std::shared_ptr<TPieces> &pPieces) {
  if (mStatus < 0) {
    pLogger->warn("current status:{} is invalid", mStatus);
    return -1;
  }

  if (pPieces) {
    pLogger->warn("this pieces:{} is already stated", pPieces->piecesID);
    return 1;
  }
  mStatus = 1;

  pPieces = std::make_shared<TPieces>();
  pPieces->begintime = getCurrentTimeMs();
  pPieces->taskID = msID.getID();
  mData.status = static_cast<uint8_t>(TaskStatus::INPROGRESS);
  return 0;
}

int TNode::exe_halt(std::shared_ptr<TPieces> &pPieces,
    const std::string &desc, uint8_t efficiency, bool nowarn) {
  if (mStatus < 0) {
    return -1;
  }

  if (!pPieces) {
    if (!nowarn) {
      pLogger->warn("this pieces is already stoped");
    } else {
      pLogger->debug("this pieces is already stoped");
    }
    return 1;
  }

  pPieces->endtime = getCurrentTimeMs();
  pPieces->serialNumber = mqPieces.size();
  pPieces->piecesID = mPieceNums.load();
  pPieces->desc = desc;
  pPieces->efficiency = efficiency;
  pPieces->status = 1;
  mqPieces.push_back(pPieces);
  mPieceNums++;
  pLogger->debug("insert pieces success, nums:{}, mPieceNums:{}",
    mqPieces.size(), mPieceNums.load());

  int64_t costTime = pPieces->endtime - pPieces->begintime;
  mData.costTime += costTime;
  mData.updateTime = pPieces->endtime;
  mData.status = static_cast<uint8_t>(TaskStatus::PAUSE);

  return 0;
}

int TNode::setTaskStatus(uint8_t status) {
  if (mStatus < 0) {
    pLogger->warn("this node is already abandon");
    return -1;
  }
  mStatus = 1;
  if (status <= static_cast<uint8_t>(TaskStatus::DONE) &&
    status >= static_cast<uint8_t>(TaskStatus::TODO)) {
    mData.status = status;
  } else {
    pLogger->warn("invalid target status:{}", status);
    return -1;
  }
  return 0;
}

int TNode::setTaskDesc(const std::string &desc) {
  if (mStatus < 0) {
    pLogger->warn("this node is already abandon");
    return -1;
  }
  mData.desc = desc;
  pLogger->trace("set new desc:{} to task:{}", desc, msID.getID());
  return 0;
}

int TNode::setTaskEfficiency(uint8_t effic) {
  if (mStatus < 0) {
    pLogger->warn("this node is already abandon");
    return -1;
  }
  mData.efficiency = effic;
  pLogger->trace("set new effic:{} to task:{}", effic, msID.getID());
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
    mqPieces.resize(pieces->serialNumber + 1);
    mqPieces[pieces->serialNumber] = nullptr;
 }

 if (mqPieces[pieces->serialNumber]) {
    pLogger->warn("task:{}, pieces:{} is collision", msID.getID(), pieces->piecesID);
    return -2;
 }

 mqPieces[pieces->serialNumber] = pieces;
 mPieceNums++;
 pLogger->trace("task:{}, insert new pieces:{}, serial:{} ",
    msID.getID(), pieces->piecesID, pieces->serialNumber);
 return 0;
}
