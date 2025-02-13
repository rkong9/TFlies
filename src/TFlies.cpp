#include <cstdio>
#include <iostream>
#include <vector>
#include <memory>
#include <sstream>
#include <queue>
#include <stack>
#include <functional>
#include <unordered_map>
#include <regex>
#include <ctime>
#include <iomanip>
#include <stdexcept>

#include "utils.hpp"
#include "viewer.hpp"
#include "logger.hpp"
#include "dataType.hpp"
#include "tNode.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "cmdline.h"

#include <sqlite3.h>

typedef cmdline::parser CmdParser;
typedef std::shared_ptr<cmdline::parser> CmdParserPtr;
std::unordered_map<std::string, CmdParserPtr> gmCmdParser;
std::shared_ptr<TPieces> gpCurrPieces;
std::vector<std::shared_ptr<TPieces>> gvpPieces;

bool createDefaultDatabase(const std::string& dbPath) {
    sqlite3* db;

    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open or create database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const char* createTaskTableSQL =
        "CREATE TABLE IF NOT EXISTS Tasks ("
        "TaskID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "Name TEXT NOT NULL, "
        "ParentTaskID INTEGER, "
        "Status INTEGER, "
        "Priority INTEGER, "
        "CreateTime INTEGER, "
        "UpdateTime INTEGER, "
        "DueTime INTEGER, "
        "CostTime INTEGER, "
        "ExpectTime INTEGER, "
        "Efficiency INTEGER, "
        "TimePiecesTable TEXT, "
        "Description TEXT"
        ");";

    if (sqlite3_exec(db, createTaskTableSQL, nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to create Task table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    const char* createTimePiecesTableSQL =
        "CREATE TABLE IF NOT EXISTS TimePieces ("
        "PiecesID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "TaskID INTEGER, "
        "SerialNumber INTEGER, "
        "Efficiency INTEGER, "
        "BeginTime INTEGER, "
        "EndTime INTEGER, "
        "Description TEXT"
        ");";

    if (sqlite3_exec(db, createTimePiecesTableSQL, nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to create TimePieces table: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    std::cout << "Default database created successfully." << std::endl;
    sqlite3_close(db);
    return true;
}

bool loadOrCreateDatabase(const std::string& dbPath, std::unordered_map<int64_t, TNodePtr> &mNode) {
    sqlite3* db;

    // 打开数据库
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 检查是否存在 Task 和 TimePieces 表
    const char* checkTablesSQL = "SELECT 1 FROM sqlite_master WHERE type='table' AND name IN ('Tasks', 'TimePieces');";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, checkTablesSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    int tableCount = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        tableCount++;
    }

    sqlite3_finalize(stmt);


    // 如果表数量不足，则创建默认数据库
    if (tableCount < 2) {
        std::cout << "Required tables are missing. Creating default database..." << std::endl;
        sqlite3_close(db); // 关闭当前数据库
        return createDefaultDatabase(".tf.db"); // 创建默认数据库
    } else {
      std::string getAllTaskSql = "SELECT * FROM Tasks;";
      std::string getTimePieceSql = "SELECT * FROM TimePieces";

      int rc = sqlite3_prepare_v2(db, getAllTaskSql.c_str(), -1, &stmt, nullptr);
      if (SQLITE_OK != rc) {
        pLogger->critical("Get all task table failed, sql: {}, err:{}",
                         getAllTaskSql,
                         sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
      }

      Item ti;
      while (sqlite3_step(stmt) == SQLITE_ROW) {
        ti.taskID = sqlite3_column_int64(stmt, 0);
        SID sid(ti.taskID);
        std::string strID = std::to_string(ti.taskID);
        if (!sid.isValid()) {
          pLogger->warn("invalid taskID:{}", ti.taskID);
          continue;
        }

        ti.name =
          std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        ti.parentTaskID = sqlite3_column_int64(stmt, 2);
        ti.status = sqlite3_column_int(stmt, 3);
        ti.priority = sqlite3_column_int(stmt, 4);
        ti.createTime = sqlite3_column_int64(stmt, 5);
        ti.updateTime = sqlite3_column_int64(stmt, 6);
        ti.dueTime = sqlite3_column_int64(stmt, 7);
        ti.costTime = sqlite3_column_int64(stmt, 8);
        ti.expectTime = sqlite3_column_int64(stmt, 9);
        ti.efficiency = sqlite3_column_int(stmt, 10);
        ti.timePiecesTable =
          std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11)));
        ti.desc =
          std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12)));
        pLogger->trace("get task:{}, name:{}, parentTaskID:{}, createTime:{}",
                      ti.taskID,
                      ti.name,
                      ti.parentTaskID,
                      ti.createTime);

        SID temp_id = sid;
        if (mNode.find(sid.getID()) != mNode.end()) {
          mNode[sid.getID()]->setData(ti);
          mNode[sid.getID()]->backUpdate(ti);
          SID pid = sid.getParentID();
          if (mNode.find(pid.getID()) != mNode.end()) {
            mNode[sid.getID()]->setParentNode(mNode[pid.getID()]);
          }
          pLogger->info("reset task:{} values", sid.getID());
        } else {
          std::shared_ptr<TNode> pCurrentNode(new TNode(ti, sid));
          std::shared_ptr<TNode> pTempNode(pCurrentNode);
          while (mNode.find(temp_id.getID()) == mNode.end()) {
            mNode[temp_id.getID()] = pTempNode;
            pLogger->info("insert task:{}, name:{}, parentTaskID:{}",
                      temp_id.getID(), pTempNode->getData().name, temp_id.getParentID().getID());

            temp_id = sid.getParentID();
            if (mNode.find(temp_id.getID()) == mNode.end()) { // no parent node
              std::shared_ptr<TNode> pParent = std::make_shared<TNode>();
              pParent->setID(temp_id);
              pTempNode->setParentNode(pParent);
              pParent->setSubNode(pTempNode);
              pTempNode = pParent;
            } else {
              std::shared_ptr<TNode> pParent = mNode[temp_id.getID()];
              pTempNode->setParentNode(pParent);
              pParent->setSubNode(pTempNode);
            }
          }
          pCurrentNode->backUpdate(ti);
        }
      }
      sqlite3_finalize(stmt);

      rc = sqlite3_prepare_v2(db, getTimePieceSql.c_str(), -1, &stmt, nullptr);
      if (SQLITE_OK != rc) {
        pLogger->critical("Get all pieces table failed, sql: {}, err:{}",
                         getTimePieceSql, sqlite3_errmsg(db));
        sqlite3_close(db);
        return false;
      }

      while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::shared_ptr<TPieces> ptp(new TPieces);
        ptp->piecesID = sqlite3_column_int64(stmt, 0);
        ptp->taskID = sqlite3_column_int64(stmt, 1);

        if (mNode.find(ptp->taskID) != mNode.end()) {
          ptp->serialNumber = sqlite3_column_int(stmt, 2);
          ptp->efficiency = sqlite3_column_int(stmt, 3);
          ptp->begintime = sqlite3_column_int64(stmt, 4);
          ptp->endtime = sqlite3_column_int64(stmt, 5);
          ptp->desc = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
          mNode[ptp->taskID]->setTimePieces(ptp);
          gvpPieces.push_back(ptp);
          pLogger->trace("insert pieces:{},{} to task:{}", ptp->piecesID, ptp->serialNumber, ptp->taskID);
        }
      }
      sqlite3_finalize(stmt);
    }

    std::cout << "Database loaded successfully." << std::endl;
    sqlite3_close(db);
    return true;
}

uint8_t parserEfficiency(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    int eff(0);
    if (str == "extremely low" || str == "el" || str == "1") {
      eff = 1;
    } else if (str == "very low" || str == "vl" || str == "2") {
      eff = 2;
    } else if (str == "low" || str == "l" || str == "3") {
      eff = 3;
    } else if (str == "normal" || str == "n" || str == "4") {
      eff = 4;
    } else if (str == "high" || str == "h" || str == "5") {
      eff = 5;
    } else if (str == "very high" || str == "vh" || str == "6") {
      eff = 6;
    } else if (str == "extremely high" || str == "eh" || str == "7") {
      eff = 7;
    }

    return eff;
}

uint8_t parserPriority(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    int prio(0);
    if (str == "trival" || str == "tr" || str == "1") {
      prio = 1;
    } else if (str == "minor" || str == "min" || str == "2") {
      prio = 2;
    } else if (str == "major" || str == "maj" || str == "3") {
      prio = 3;
    } else if (str == "critical" || str == "cri" || str == "4") {
      prio = 4;
    } else if (str == "block" || str == "blk" || str == "5") {
      prio = 5;
    }

    return prio;
}

uint8_t parserStatus(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    int status(0);
    if (str == "todo" || str == "t" || str == "1") {
      status = 0;
    } else if (str == "progress" || str == "i" || str == "2") {
      status = 1;
    } else if (str == "pause" || str == "p" || str == "3") {
      status = 2;
    } else if (str == "done" || str == "d" || str == "4") {
      status = 3;
    }

    return status;
}

void initArgParser(std::unordered_map<std::string, CmdParserPtr> &mParser) {
    CmdParserPtr createParser(new CmdParser);
    createParser->add<int64_t>("parentID", 'P', "task parent ID", false, 0);
    createParser->add<std::string>("name", 'n', "task name", true);
    createParser->add<std::string>("dueDate", 'd', "task due date", false, "None");
    createParser->add<std::string>("description", 't', "task description", false, "None");
    createParser->add<std::string>("expectTime", 'e', "task expect time(ms/s/m/h)", false, "30m");
    createParser->add<std::string>("priority", 'p', "task priority(UD/TR/MIN/MAJ/CRI/BLK)", false, "UD");
    mParser["create"] = createParser;

    CmdParserPtr deleteParser(new CmdParser);
    deleteParser->add<int64_t>("ID", 'I', "task ID", true);
    mParser["delete"] = deleteParser;

    CmdParserPtr listParser(new CmdParser);
    listParser->add<int64_t>("ID", 'I', "task ID", false, 0);
    listParser->add<int>("level", 'l', "task list level", false, -1);
    mParser["list"] = listParser;

    CmdParserPtr showParser(new CmdParser);
    showParser->add<int64_t>("ID", 'I', "task ID", true);
    mParser["show"] = showParser;

    CmdParserPtr execTParser(new CmdParser);
    execTParser->add<int64_t>("ID", 'I', "task ID", true);
    mParser["exec"] = execTParser;

    CmdParserPtr haltTParser(new CmdParser);
    haltTParser->add<std::string>("efficiency", 'e', "efficiency(EL/VL/L/N/H/VH/EH)", true);
    haltTParser->add<std::string>("desc", 't', "pieces description", true);
    mParser["halt"] = haltTParser;

    CmdParserPtr setStatusParser(new CmdParser);
    setStatusParser->add<int64_t>("ID", 'I', "task ID", true);
    setStatusParser->add<std::string>("status", 's', "task status, start/stop/done", true);
    mParser["set_status"] = setStatusParser;

    CmdParserPtr updateTParser(new CmdParser);
    updateTParser->add<int64_t>("ID", 'I', "task ID", true);
    updateTParser->add<std::string>("name", 'n', "task name", false, "None");
    updateTParser->add<std::string>("dueDate", 'd', "task due date", false, "None");
    updateTParser->add<std::string>("description", 't', "task description", false, "None");
    updateTParser->add<std::string>("expectTime", 'e', "task expect time(ms/s/m/h)", false, "30m");
    updateTParser->add<std::string>("priority", 'p', "task priority(UD/TR/MIN/MAJ/CRI/BLK)", false, "UD");
    mParser["update"] = updateTParser;

    CmdParserPtr moveTParser(new CmdParser);
    moveTParser->add<int64_t>("srcID", 's', "source task ID", true);
    moveTParser->add<int64_t>("targetID", 't', "target task ID(new parent node)", true);
    mParser["move"] = moveTParser;

    CmdParserPtr graphTParser(new CmdParser);
    // graphTParser->add<int64_t>("ID", 'I', "task ID", true);
    graphTParser->add<int>("mode", 'm', "graph viewer mode", false, 0);
    graphTParser->add<int>("nums", 'n', "display nums(each mode has different meas)", false, 0);
    graphTParser->add<std::string>("date", 'd', "date to display", false, "");
    mParser["graph"] = graphTParser;
}

std::vector<ViewData> getInfoOfTimeRange(int64_t time_beg, int64_t time_end) {
    std::vector<ViewData> vInfo;
    if (time_beg < time_end) {
        pLogger->error("invalid time range", time_beg, time_end);
        return vInfo;
    }

    float length = static_cast<float>(time_beg - time_end);
    for (auto &pPieces : gvpPieces) {
        if (pPieces->endtime < time_beg || pPieces->begintime > time_end) {
            continue;
        }
        ViewData vd;
        vd.beg = static_cast<float>(pPieces->begintime - time_beg) / length;
        vd.end = static_cast<float>(pPieces->endtime - time_beg) / length;
        vd.val = pPieces->efficiency;
    }

    std::sort(vInfo.begin(), vInfo.end(), [](const ViewData &a, const ViewData &b) {
        return a.beg < b.beg;});

    return vInfo;
}

int graph_analize(const CmdParserPtr &pParser, const std::vector<std::string> &vArgs,
  std::unordered_map<int64_t, TNodePtr> &mNode) {
  if (!pParser->parse(vArgs)) {
    std::cout << "parse graph cmd arguments failed, " << pParser->usage()
              << std::endl;
    return -1;
  }

  std::time_t current_time = std::time(nullptr);
  std::tm *local_time = std::localtime(&current_time);
  std::tm rt = *local_time;
  // get begtime
  rt.tm_hour = 0;
  rt.tm_min = 0;
  rt.tm_sec = 0;
  std::time_t begTime  = std::mktime(&rt);

  std::vector<ViewData> vData;
  for (auto &pieces : gvpPieces) {
      ViewData vd;
      vd.beg = (float)(pieces->begintime / 1000 - begTime) / 86400; // sec per day
      vd.end = (float)(pieces->endtime / 1000 - begTime) / 86400;
      if (vd.end < 0.0 || vd.beg > 1.0) {
          continue;
      } else if (vd.beg < 0.0) {
        vd.beg = 0.0;
      } else if (vd.end > 1.0) {
        vd.end = 1.0;
      }
      vd.val = pieces->efficiency;
      vData.push_back(vd);
  }

  std::cout << renderDailyEffGraph(vData, 12, 60) << std::endl;

  // int g_mode = pParser->get<int>("mode");

  // switch(g_mode) {
  //     case 0:

  //     default:
  //       break;
  // }

  return 0;
}

int moveT(const CmdParserPtr &pParser, const std::vector<std::string> &vArgs,
  std::unordered_map<int64_t, TNodePtr> &mNode) {
  if (!pParser->parse(vArgs)) {
    std::cout << "parse move cmd arguments failed, " << pParser->usage()
              << std::endl;
    return -1;
  }
  int64_t sID = pParser->get<int64_t>("srcID");
  int64_t tID = pParser->get<int64_t>("targetID");

  if (mNode.find(sID) == mNode.end() || mNode.find(tID) == mNode.end()) {
    pLogger->error("invalid srcID:{} or targetID:{}", sID, tID);
    return -1;
  }

  if (SID::getParentID(sID) == tID) {
    pLogger->warn("this not is already at the target location");
    return 1;
  }

  TNodePtr sNode = mNode[sID];
  TNodePtr npNode = mNode[tID];

  std::stack<TNodePtr> srcStack;
  std::stack<TNodePtr> targetStack;
  srcStack.push(sNode);
  targetStack.push(npNode);

  while(!srcStack.empty()) {
    TNodePtr tempNode = srcStack.top();
    TNodePtr newPNode = targetStack.top();
    srcStack.pop();
    targetStack.pop();

    Item it = tempNode->getData();
    TNodePtr newNode = newPNode->createSubNode(it);
    tempNode->mStatus = -1;
    newNode->mqPieces = std::move(tempNode->mqPieces);
    mNode[newNode->getID()] = newNode;
    for (auto &pieces : newNode->mqPieces) {
      pieces->taskID = newNode->getID();
      pieces->status = 1;
    }

    for (auto it = tempNode->mqSubTNode.rbegin(); it != tempNode->mqSubTNode.rend(); it++) {
      srcStack.push(*it);
      targetStack.push(newNode);
    }
  }

  return 0;
}

int createT(const CmdParserPtr &pParser, const std::vector<std::string> &vArgs,
  std::unordered_map<int64_t, TNodePtr> &mNode) {
  if (!pParser->parse(vArgs)) {
    std::cout << "parse creat cmd arguments failed, " << pParser->usage()
              << std::endl;
    return -1;
  }

  // if (pParser->exist("help")) {
  //   std::cout << "parse creat cmd, " << pParser->usage()
  //             << std::endl;
  //   return -1;
  // }

  Item it;
  int64_t pID = pParser->get<int64_t>("parentID");
  SID s_pID(pID);
  if (!s_pID.isValid() || mNode.find(pID) == mNode.end()) {
    pLogger->warn("get invalid parent id:{}, create task failed", pID);
    return -1;
  }
  TNodePtr pNode = mNode[pID];
  it.parentTaskID = pID;
  it.name = pParser->get<std::string>("name");
  it.desc = pParser->get<std::string>("description");
  std::string sPrio = pParser->get<std::string>("priority");
  it.priority = parserPriority(sPrio);


  std::string eTimeStr = pParser->get<std::string>("expectTime");
  it.expectTime = timeParserValue(eTimeStr);

  if (pParser->exist("dueDate")) {
    std::string dueDateStr = pParser->get<std::string>("dueDate");
    it.dueTime = timeParserDate(dueDateStr);
  }

  TNodePtr subNode = pNode->createSubNode(it);
  if (subNode) {
    pLogger->trace("creat new task:{} ID:{} success", it.name, subNode->getID());
    auto it = subNode->getData();
    subNode->setParentNode(pNode);
    subNode->mStatus = 1;
    mNode[subNode->getID()] = subNode;
    pLogger->debug("id:{}, pid:{}, status:{}, prior:{}, effi:{}, creT:{}, updT:{}, dueT:{}, costT:{}, expT:{}, name:{}",
        it.taskID, it.parentTaskID, it.status, it.priority, it.efficiency, it.createTime, it.updateTime,
        it.dueTime, it.costTime, it.expectTime, it.name);
  } else {
    pLogger->warn("creat new task:{} failed, invalid nodeptr", it.name);
  }
  return 0;
}

bool insertOrUpdateTask(sqlite3* db, const std::shared_ptr<TNode> &node) {
    const Item &ti = node->getData();
    if (node->mStatus > 0) {
      if (gpCurrPieces && gpCurrPieces->taskID == node->getID()) {
        node->exe_halt(gpCurrPieces, "None", 3, true);
        gvpPieces.push_back(gpCurrPieces);
        gpCurrPieces = nullptr;
      }

      // node->exe_halt("None", 3, true);
      // 构造 SQL 语句
      std::string sql = "INSERT OR REPLACE INTO Tasks (TaskID, Name, ParentTaskID, Status, Priority, CreateTime, "
                        "UpdateTime, DueTime, CostTime, ExpectTime, Efficiency, TimePiecesTable, Description) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

      // 准备 SQL 语句
      sqlite3_stmt* stmt;
      if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
          pLogger->error("Failed to prepare statement:{}", sqlite3_errmsg(db));
          return false;
      }

      // 绑定参数
      sqlite3_bind_int64(stmt, 1, ti.taskID);
      sqlite3_bind_text(stmt, 2, ti.name.c_str(), -1, SQLITE_STATIC);
      sqlite3_bind_int64(stmt, 3, ti.parentTaskID);
      sqlite3_bind_int(stmt, 4, ti.status);
      sqlite3_bind_int(stmt, 5, ti.priority);
      sqlite3_bind_int64(stmt, 6, ti.createTime);
      sqlite3_bind_int64(stmt, 7, ti.updateTime);
      sqlite3_bind_int64(stmt, 8, ti.dueTime);
      sqlite3_bind_int64(stmt, 9, ti.costTime);
      sqlite3_bind_int64(stmt, 10, ti.expectTime);
      sqlite3_bind_int(stmt, 11, ti.efficiency);
      sqlite3_bind_text(stmt, 12, ti.timePiecesTable.c_str(), -1, SQLITE_STATIC);
      sqlite3_bind_text(stmt, 13, ti.desc.c_str(), -1, SQLITE_STATIC);

      // 执行 SQL 语句
      if (sqlite3_step(stmt) != SQLITE_DONE) {
          pLogger->error("Failed to prepare statement:{}", sqlite3_errmsg(db));
          sqlite3_finalize(stmt);
          return false;
      }

      pLogger->debug("storetoDb, id:{}, pid:{}, status:{}, prior:{}, effi:{}, creT:{}, updT:{}, dueT:{}, costT:{}, expT:{}, name:{}",
          ti.taskID, ti.parentTaskID, ti.status, ti.priority, ti.efficiency, ti.createTime, ti.updateTime,
          ti.dueTime, ti.costTime, ti.expectTime, ti.name);

      // 释放资源
      sqlite3_finalize(stmt);

      std::string pieces_sql = "INSERT OR REPLACE INTO TimePieces (PiecesID, TaskID, SerialNumber, Efficiency, BeginTime, EndTime, "
                        "Description) VALUES (?, ?, ?, ?, ?, ?, ?);";
      if (sqlite3_prepare_v2(db, pieces_sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
          pLogger->error("Failed to prepare statement:{}", sqlite3_errmsg(db));
          return false;
      }

      for (auto &pPieces : node->mqPieces) {
        if (pPieces && pPieces->status > 0) {
          sqlite3_bind_int64(stmt, 1, pPieces->piecesID);
          sqlite3_bind_int64(stmt, 2, pPieces->taskID);
          sqlite3_bind_int(stmt, 3, pPieces->serialNumber);
          sqlite3_bind_int(stmt, 4, pPieces->efficiency);
          sqlite3_bind_int64(stmt, 5, pPieces->begintime);
          sqlite3_bind_int64(stmt, 6, pPieces->endtime);
          sqlite3_bind_text(stmt, 7, pPieces->desc.c_str(), -1, SQLITE_STATIC);
          if (sqlite3_step(stmt) != SQLITE_DONE) {
            char *errMsg = 0;
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, &errMsg);
            pLogger->error("Failed to save pieces prepare statement:{}", sqlite3_errmsg(db));
          } else {
            pLogger->trace("storage task pieces:{} to db success", pPieces->piecesID);
          }
          sqlite3_reset(stmt);
        }
      }
      sqlite3_finalize(stmt);
      return true;

    } else if (node->mStatus < 0) { // delete node
      char *errMsg = 0;
      std::string sql = "DELETE FROM Tasks WHERE TaskID = " + std::to_string(ti.taskID) + ";";
      int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
      if (rc != SQLITE_OK) {
          pLogger->error("delete task:{} from db failed, err:{}", ti.taskID, sqlite3_errmsg(db));
          sqlite3_free(errMsg);
          return false;
      } else {
          pLogger->trace("delete task:{} from db success", ti.taskID);
      }

      std::string sql_pieces = "DELETE FROM TimePieces WHERE TaskID = " + std::to_string(ti.taskID) + ";";
      rc = sqlite3_exec(db, sql_pieces.c_str(), 0, 0, &errMsg);
      if (rc != SQLITE_OK) {
          pLogger->error("delete task pieces:{} from TimePieces failed, err:{}", ti.taskID, sqlite3_errmsg(db));
          sqlite3_free(errMsg);
          return false;
      } else {
          pLogger->trace("delete task pieces:{} from TimePieces success", ti.taskID);
      }
    }
    return true;
}

int storeToDb(const std::string &dbPath, const TNodePtr &pNode) {
  sqlite3* db;

  // 打开数据库
  if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
      std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
      return false;
  }

  std::stack<std::shared_ptr<TNode>> spNode;
  spNode.push(pNode);
  // int pNodeLevel = pNode->getLevel();
  while(!spNode.empty()) {
    std::shared_ptr<TNode> pTempNode = spNode.top();
    spNode.pop();
    if (!pTempNode) {
      continue;
    }

    if (insertOrUpdateTask(db, pTempNode)) {
      pLogger->trace("Success to insert or update task:{}", pTempNode->getID());
      for (auto it = pTempNode->mqSubTNode.rbegin(); it != pTempNode->mqSubTNode.rend(); it++) {
        spNode.push(*it);
      }
    } else {
      pLogger->error("Failed to insert or update task:{}", pTempNode->getID());
      return -1;
    }

    // }
  }
  sqlite3_close(db);
  return 0;
}

int deleteT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args,
    std::unordered_map<int64_t, TNodePtr> &mNode) {
  pParser->parse(v_args);
  if (!pParser->parse(v_args)) {
    std::cout << "parse listT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }
  int64_t id = pParser->get<int64_t>("ID");
  pLogger->debug("start to remove task:{}", id);

  if (mNode.find(id) == mNode.end()) {
    pLogger->trace("cannot to find task:{}", id);
    return 0;
  }

  TNodePtr d_node = mNode[id];

  int status(-1);
  if (d_node->mStatus >= 0) {
    for (auto it : d_node->mqSubTNode) {
      if (it->mStatus >= 0) {
        pLogger->warn("cannot delete currentnode: {}, this is not leaf node, subNode:{}",
          d_node->getID(), it->getID());
        status = d_node->mStatus;
      }
    }
    d_node->mStatus = status;
  }
  // d_node->deleteSelfFromTree();
  // mNode.erase(id);
  // pLogger->debug("erase node:{} from map success", id);

  return 0;
}

int updateT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args,
    std::unordered_map<int64_t, TNodePtr> &mNode) {
  pParser->parse(v_args);
  if (!pParser->parse(v_args)) {
    std::cout << "parse updateT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }
  int64_t id = pParser->get<int64_t>("ID");
  pLogger->debug("start to update task:{}", id);

  if (mNode.find(id) == mNode.end()) {
    pLogger->trace("cannot to find task:{}", id);
    return -2;
  }
  TNodePtr node = mNode[id];
  Item it = node->getData();

  bool update(false);
  if (pParser->exist("name")) {
    it.name = pParser->get<std::string>("name");
    update = true;
  }
  if (pParser->exist("dueDate")) {
    std::string dueDateStr = pParser->get<std::string>("dueDate");
    it.dueTime = timeParserDate(dueDateStr);
    update = true;
  }

  if (pParser->exist("description")) {
    it.desc = pParser->get<std::string>("description");
    update = true;
  }

  if (pParser->exist("expectTime")) {
    std::string eTimeStr = pParser->get<std::string>("expectTime");
    it.expectTime = timeParserValue(eTimeStr);
    update = true;
  }

  if (pParser->exist("priority")) {
    it.priority = parserPriority(pParser->get<std::string>("priority"));
    if (it.priority == 0) {
      pLogger->warn("get undefined priority");
    }
    update = true;
  }

  if (update) {
    node->setData(it, true);
  }

  return 0;
}

void listAllT(std::shared_ptr<TNode> pNode, std::function<bool(std::shared_ptr<TNode>)> filter) {
  // for 
  // std::stringstream ss;
  std::stack<std::shared_ptr<TNode>> spNode;
  spNode.push(pNode);
  int pNodeLevel = pNode->getLevel();
  while(!spNode.empty()) {
    std::shared_ptr<TNode> pTempNode = spNode.top();
    spNode.pop();
    int currLevel = pTempNode->getLevel();
    for (int i = 0; i < currLevel - pNodeLevel; i++) {
        std::cout << " | ";
    }

    if (pTempNode->mStatus < 0) {
      continue;
    }

    auto it = pTempNode->getData();
    if (filter(pTempNode)) {
      std::cout << "+";
    } else {
      std::cout << "-";
      for (auto it = pTempNode->mqSubTNode.rbegin(); it != pTempNode->mqSubTNode.rend(); it++) {
        if ((*it) && (*it)->mStatus >= 0) {
          spNode.push(*it);
        }
      }
    }
    std::cout << it.taskID << " " << "[" << TStatusToStrS(it.status) << "] "
              << getColors(it.status) << it.name << " (" << pTempNode->mqPieces.size() << " pieces)"
              << RESET << "\n";
  }
}

int listT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args,
  std::unordered_map<int64_t, TNodePtr> &mNode) {
  pParser->parse(v_args);
  if (!pParser->parse(v_args)) {
    std::cout << "parse listT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }

  int level = pParser->get<int>("level");
  if (level < 0) {
    level = 9999;
  }
  int64_t id = pParser->get<int64_t>("ID");

  if (mNode.find(id) != mNode.end()) {
    int currLevel = mNode[id]->getLevel();
    auto filter = [&](std::shared_ptr<TNode> pNode) -> bool {
      if (pNode->getLevel() < currLevel + level) {
        return false;
      }
      return true;
    };

    listAllT(mNode[id], filter);
    pLogger->trace("list node:{}, currLevel:{}, level:{}", id, currLevel, level);
  } else {
    pLogger->warn("cannot find id:{} to list", id);
  }

  return 0;
}

int showT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args,
    std::unordered_map<int64_t, TNodePtr> &mNode) {
  pParser->parse(v_args);
  if (!pParser->parse(v_args)) {
    std::cout << "parse showT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }
  int64_t id = pParser->get<int64_t>("ID");
  if (mNode.find(id) == mNode.end()) {
    pLogger->warn("cannot find node:{}", id);
    return -1;
  }
  TNodePtr node = mNode[id];
  const Item &it = node->getData();

  std::cout << "- taskID:" << WHITE << it.taskID << RESET << "\n"
            << "  parentID:" << WHITE << it.parentTaskID << RESET << "\n"
            << "  name:" << GREEN << it.name << RESET << "\n"
            << "  description:" << BLUE << it.desc << RESET << "\n"
            << "  status:" << TStatusToStr(it.status) << "\n"
            << "  priority:" << TPrioToStr(it.priority, true) << "\n"
            << "  efficiency:" << TEffiToStr(it.efficiency, true) << "\n"
            << "  createTime:" << MAGENTA << getDateStr(it.createTime) << RESET << "\n"
            << "  updateTime:" << GREEN << getDateStr(it.updateTime) << RESET << "\n"
            << "  dueTime:" << RED << getDateStr(it.dueTime) << RESET << "\n"
            << "  costTime:" << BLUE << getTimeStr(it.costTime) << RESET << "\n"
            << "  expectTime:" << GREEN << getTimeStr(it.expectTime) << RESET << "\n"
            << "  timePiecesTable:" << it.timePiecesTable << '\n';
  if (node->mqPieces.size() > 0) {
    std::cout << WHITE << "   + PieceID, TaskID, SerialID, Effic, BegTime, EndTime, CostTime, Desc" << WHITE << '\n';
    int nums(0);
    for (auto &piece : node->mqPieces) {
      if (!piece) {
        pLogger->error("invalid time pieces:{} in task:{}", nums, it.taskID);
        continue;
      }

      std::cout << "   - " << WHITE << piece->piecesID << ", "
                << piece->taskID << ", "
                << piece->serialNumber << RESET << ", "
                << TEffiToStr(piece->efficiency, true) << ", "
                << CYAN << getDateStr(piece->begintime) << ", "
                << getDateStr(piece->endtime) << RESET << ", "
                << GREEN << getTimeStr(piece->endtime - piece->begintime) << RESET << ", "
                << WHITE << piece->desc << RESET << "\n";
      nums++;
    }
  }
  return 0;
}

int selectT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args,
    std::unordered_map<int64_t, TNodePtr> &mNode) {
  return 0;
}

int execT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args,
  std::unordered_map<int64_t, TNodePtr> &mNode) {
  pParser->parse(v_args);
  if (!pParser->parse(v_args)) {
    std::cout << "parse exec cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }
  int64_t id = pParser->get<int64_t>("ID");
  if (mNode.find(id) == mNode.end()) {
    pLogger->warn("cannot find node:{}", id);
    return -1;
  }

  if (gpCurrPieces) {
    pLogger->warn("the task:{} is progressing", gpCurrPieces->taskID);
    return -2;
  }
  TNodePtr node = mNode[id];
  node->exe_start(gpCurrPieces);

  return 0;
}

int haltT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args,
  std::unordered_map<int64_t, TNodePtr> &mNode) {

  pParser->parse(v_args);
  if (!pParser->parse(v_args)) {
    std::cout << "parse halt cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }

  if (!gpCurrPieces) {
    pLogger->warn("no task is processing");
  }

  int64_t id = gpCurrPieces->taskID;
  if (mNode.find(id) == mNode.end()) {
    pLogger->warn("cannot find task:{}", id);
    return -1;
  }
  TNodePtr node = mNode[id];

  std::string desc = pParser->get<std::string>("desc");
  uint8_t efficiency = parserEfficiency(pParser->get<std::string>("efficiency"));
  node->exe_halt(gpCurrPieces, desc, efficiency);
  gvpPieces.push_back(gpCurrPieces);
  gpCurrPieces = nullptr;
  return 0;
}

int setStatusOfT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args,
  std::unordered_map<int64_t, TNodePtr> &mNode) {

  pParser->parse(v_args);
  if (!pParser->parse(v_args)) {
    std::cout << "parse doneT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }
  int64_t id = pParser->get<int64_t>("ID");
  std::string status = pParser->get<std::string>("status");
  if (mNode.find(id) == mNode.end()) {
    pLogger->warn("cannot find node:{}", id);
    return -1;
  }
  TNodePtr node = mNode[id];
  int istatus(-1);
  if (status == "start") {
    istatus = 1; //inprogress
  } else if (status == "stop") {
    istatus = 2; // pause
  } else if (status == "done") {
    istatus = 3;
  }
  return node->setTaskStatus(istatus);
}

int main(int argc, char **argv) {

    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("./tfLog.txt", 1024 * 1024 * 10, 100, false);
    fileSink->set_level(spdlog::level::trace);

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::warn);

    pLogger = std::make_shared<spdlog::logger>("Log", spdlog::sinks_init_list{fileSink, consoleSink});
    // pLogger->set_pattern("[%Y%m%d %H:%M:%S.%e] [%l] [%@] [%!] %v");
    pLogger->set_level(spdlog::level::trace);

    // pLogger->set_level(spdlog::level::trace);
    Item rIt;
    rIt.name = "root";
    std::shared_ptr<TNode> pRootNode(new TNode(rIt, SID(0)));
    std::shared_ptr<TNode> pSelectedNode(pRootNode);
    std::unordered_map<int64_t, TNodePtr> mNode;
    mNode[pRootNode->getID()] = pRootNode;
    pLogger->debug("create root node success, id:{}", pRootNode->getID());

    std::string dbPath = ".tf.db"; // 数据库路径
    if (loadOrCreateDatabase(dbPath, mNode)) {
        pLogger->info("Database :{} loaded or created successfully", dbPath);
    } else {
        pLogger->critical("Failed to load or create database:{}", dbPath);
    }
    initArgParser(gmCmdParser);

    while(true) {
        std::string input;
        std::cout << ">>> ";
        std::getline(std::cin, input);
        pLogger->info("get inputs:{}", input);
        size_t validIndex(0);
        while(validIndex < input.length()) {
            if (input[validIndex] != ' ') {
                break;
            }
        }

        std::stack<char> cStack;
        std::string buff;
        std::vector<std::string> vBuff;
        for (size_t i = 0; i < input.length(); i++) {
          char curr = input[i];
          if (curr == '\'' || curr == '\"') {
            if (!cStack.empty() && cStack.top() == curr) {
              vBuff.push_back(buff);
              buff.clear();
              cStack.pop();
            } else if (cStack.empty()) {
              cStack.push(curr);
            } else {
              buff.push_back(curr);
            }
          } else if (curr == ' ') {
            if (!cStack.empty()) {
              buff.push_back(curr);
            } else {
              if (!buff.empty()) {
                vBuff.push_back(buff);
                buff.clear();
              }
            }
          } else {
            buff.push_back(curr);
          }
        }

        if (!buff.empty()) {
          vBuff.push_back(buff);
        } else if (vBuff.empty()) {
          continue;
        }

        std::stringstream ss;
        ss << "get buff:";
        for (auto &buff : vBuff) {
             ss << " \"" << buff << "\"  ";
        }
        pLogger->debug("{}", ss.str());

        if (vBuff[0] == "q") {
            pLogger->info("exit program!");
            storeToDb(dbPath, pRootNode);
            break;
        } else if (vBuff[0] == "create") {
          pLogger->info("get create cmd");
          createT(gmCmdParser["create"], vBuff, mNode);
          // gmCmdParser["create"]->reset();
        } else if (vBuff[0] == "delete") {
          pLogger->info("get delete cmd");
          deleteT(gmCmdParser["delete"], vBuff, mNode);
        } else if (vBuff[0] == "list") {
          pLogger->info("get list cmd");
          listT(gmCmdParser["list"], vBuff, mNode);
        } else if (vBuff[0] == "show") {
          pLogger->info("get show task cmd");
          showT(gmCmdParser["show"], vBuff, mNode);
        } else if (vBuff[0] == "set_status") {
          pLogger->info("get set task status cmd");
          setStatusOfT(gmCmdParser["set_status"], vBuff, mNode);
        } else if (vBuff[0] == "exec") {
          pLogger->info("get exec cmd");
          execT(gmCmdParser["exec"], vBuff, mNode);
        } else if (vBuff[0] == "halt") {
          pLogger->info("get halt cmd");
          haltT(gmCmdParser["halt"], vBuff, mNode);
        } else if (vBuff[0] == "update") {
          pLogger->info("get update cmd");
          updateT(gmCmdParser["update"], vBuff, mNode);
        } else if (vBuff[0] == "move") {
          pLogger->info("get move cmd");
          moveT(gmCmdParser["move"], vBuff, mNode);
        } else if (vBuff[0] == "graph") {
          pLogger->info("get graph cmd");
          graph_analize(gmCmdParser["graph"], vBuff, mNode);
        }
        else if (vBuff[0] == "help") {
          for (auto &pair : gmCmdParser) {
            std::cout << " - " << pair.first << '\n';
          }
          std::cout << " - q       quit cmds\n"
                    << " - helps   list all cmds\n";
        } else {
          pLogger->warn("get unknown cmd:{}", vBuff[0]);
        }
    }
    return 0;
}
