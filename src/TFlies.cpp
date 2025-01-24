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
        "piecesID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "taskID INTEGER, "
        "serialNumber INTEGER, "
        "efficiency INTEGER, "
        "begintime INTEGER, "
        "endtime INTEGER, "
        "desc TEXT"
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
              pParent->setSubNode(pTempNode);
              pTempNode = pParent;
            } else {
              mNode[temp_id.getID()]->setSubNode(pTempNode);
            }
          }
          pCurrentNode->backUpdate(ti);
        }
      }
      sqlite3_finalize(stmt);
    }

    std::cout << "Database loaded successfully." << std::endl;
    sqlite3_close(db);
    return true;
}

void initArgParser(std::unordered_map<std::string, CmdParserPtr> &mParser) {
    CmdParserPtr createParser(new CmdParser);
    createParser->add<int64_t>("parentID", 'P', "task parent ID", false, 0);
    createParser->add<std::string>("name", 'n', "task name", true);
    createParser->add<std::string>("dueDate", 'd', "task due date", false, "None");
    createParser->add<std::string>("description", 't', "task description", false, "None");
    createParser->add<std::string>("expectTime", 'e', "task expect time(ms/s/m/h)", false, "30m");
    createParser->add<int>("priority", 'p', "task priority", false, 1);
    mParser["create"] = createParser;

    CmdParserPtr deleteParser(new CmdParser);
    deleteParser->add<int64_t>("ID", 'I', "task ID", true);
    mParser["delete"] = deleteParser;

    CmdParserPtr listParser(new CmdParser);
    listParser->add<int64_t>("ID", 'I', "task ID", false, 0);
    listParser->add<int>("level", 'l', "task list level", false, -1);
    mParser["list"] = listParser;

    CmdParserPtr startParser(new CmdParser);
    startParser->add<int64_t>("ID", 'I', "task ID", true);
    mParser["start"] = startParser;

    CmdParserPtr stopParser(new CmdParser);
    stopParser->add<int64_t>("ID", 'I', "task ID", true);
    mParser["stop"] = stopParser;

    CmdParserPtr doneParser(new CmdParser);
    doneParser->add<int64_t>("ID", 'I', "task ID", true);
    mParser["done"] = doneParser;
}

// 检查月份是否有效
bool isValidMonth(int month) {
    return month >= 1 && month <= 12;
}

// 检查日期是否有效
bool isValidDay(int year, int month, int day) {
    if (day < 1 || day > 31) return false;

    // 检查月份对应的日期
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return day <= 30; // 4 月、6 月、9 月、11 月最多 30 天
    } else if (month == 2) {
        // 检查闰年
        bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return day <= (isLeapYear ? 29 : 28); // 2 月最多 28 或 29 天
    } else {
        return day <= 31; // 其他月份最多 31 天
    }
}

// 检查时间值是否有效
bool isValidTime(int hour, int minute, int second, int milliseconds) {
    return (hour >= 0 && hour <= 23) &&
           (minute >= 0 && minute <= 59) &&
           (second >= 0 && second <= 59) &&
           (milliseconds >= 0 && milliseconds <= 999);
}

bool isFutureTime(const std::tm& tm, int milliseconds) {
    auto now = std::chrono::system_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    std::time_t time = std::mktime(const_cast<std::tm*>(&tm));
    auto inputMs = time * 1000 + milliseconds;

    // 比较时间戳
    return inputMs > nowMs;
}

int64_t timeParserDate(const std::string &timeStr) {
    std::regex pattern(R"((\d{4})(\d{2})(\d{2})-(\d{2})(\d{2})(\d{2})\+(\d{3}))");
    std::smatch match;

    int64_t finalTimestamp(-1);
    do {
      if (std::regex_match(timeStr, match, pattern)) {
          int year = std::stoi(match[1]);
          int month = std::stoi(match[2]);
          int day = std::stoi(match[3]);
          int hour = std::stoi(match[4]);
          int minute = std::stoi(match[5]);
          int second = std::stoi(match[6]);
          int milliseconds = std::stoi(match[7]);

          // 检查时间值的有效性
          if (!isValidMonth(month)) {
              pLogger->warn("invalid month:{}", month);
              break;
          }
          if (!isValidDay(year, month, day)) {
              pLogger->warn("invalid day:{}", day);
              break;
          }
          if (!isValidTime(hour, minute, second, milliseconds)) {
              pLogger->warn("invalid time:{}h, {}m, {}s, {}ms",
              hour, minute, second, milliseconds);
              break;
          }

          // 构造 tm 结构体
          std::tm tm = {};
          tm.tm_year = year - 1900; // tm_year 是从 1900 开始的年份
          tm.tm_mon = month - 1;    // tm_mon 是从 0 开始的月份
          tm.tm_mday = day;
          tm.tm_hour = hour;
          tm.tm_min = minute;
          tm.tm_sec = second;

          // 检查时间是否大于当前时间
          if (!isFutureTime(tm, milliseconds)) {
              pLogger->warn("Time is not in the future");
              break;
          }

          // 将 tm 转换为 time_t（秒级时间戳）
          std::time_t time = std::mktime(&tm);

          // 转换为 chrono::system_clock::time_point
          auto timePoint = std::chrono::system_clock::from_time_t(time);

          // 加上毫秒部分
          timePoint += std::chrono::milliseconds(milliseconds);

          // 计算时间戳（从 1970-01-01 开始的毫秒数）
          finalTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
              timePoint.time_since_epoch()).count();
      } else {
        pLogger->warn("invalid time format:{}, should:yyyymmdd-hhmmss+xxx");
      }
    } while(0);
    return finalTimestamp;
}

int64_t timeParserValue(const std::string &str) {
  std::regex time_pattern(R"(^\d+(ms|s|min|h))");
  int64_t finalTime(-1);
  if (std::regex_match(str, time_pattern)) {
    // get digits
    size_t idx(0);
    while(idx < str.length()) {
      if (!std::isdigit(str[idx])) {
        break;
      }
      idx++;
    }
    finalTime = std::atol(str.substr(0, idx).c_str());
    std::string unit = str.substr(idx);
    if (unit == "s") { 
      finalTime *= 1000;
    } else if (unit == "min") {
      finalTime *= 1000 * 60;
    } else if (unit == "h") {
      finalTime *= 1000 * 60 * 60;
    }
  }
  return finalTime;
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
  it.priority = pParser->get<int>("priority");


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
    pLogger->debug("id:{}, pid:{}, status:{}, prior:{}, effi:{}, creT:{}, updT:{}, dueT:{}, costT:{}, expT:{}, name:{}",
        it.taskID, it.parentTaskID, it.status, it.priority, it.efficiency, it.createTime, it.updateTime,
        it.dueTime, it.costTime, it.expectTime, it.name);
  } else {
    pLogger->warn("creat new task:{} failed, invalid nodeptr", it.name);
  }
  return 0;
}

bool insertOrUpdateTask(sqlite3* db, const Item &ti) {

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
    // int currLevel = pTempNode->getLevel();
    // for (int i = 0; i < currLevel - pNodeLevel; i++) {
    //     std::cout << " | ";
    // }

    // if (filter(pNode)) {
    //     std::cout << "+" << pNode->getID() << '\n';

    // } else {
    //     std::cout << "-" << pNode->getID() << '\n';
    if (insertOrUpdateTask(db, pTempNode->getData())) {
      for (auto it = pTempNode->mqSubTNode.rbegin(); it != pTempNode->mqSubTNode.rend(); it++) {
        spNode.push(*it);
      }
      pLogger->trace("Success to insert or update task:{}", pTempNode->getID());
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
    std::cout << "parse deleteT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
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

    auto it = pTempNode->getData();
    if (filter(pTempNode)) {
        std::cout << "+" << it.taskID << " " << it.name << '\n';
    } else {
        std::cout << "-" << it.taskID << " " << it.name << '\n';
      for (auto it = pTempNode->mqSubTNode.rbegin(); it != pTempNode->mqSubTNode.rend(); it++) {
        spNode.push(*it);
      }
    }
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

int showT(const CmdParser &pParser, const std::vector<std::string> &v_args,
    std::unordered_map<int64_t, TNodePtr> &mNode) {

  return 0;
}

int startT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args) {
  pParser->parse(v_args);
  if (!pParser->parse(v_args) || pParser->exist("help")) {
    std::cout << "parse startT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }
  return 0;
}

int stopT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args) {
  pParser->parse(v_args);
  if (!pParser->parse(v_args) || pParser->exist("help")) {
    std::cout << "parse stopT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }
  return 0;
}

int doneT(const CmdParserPtr &pParser, const std::vector<std::string> &v_args) {
  pParser->parse(v_args);
  if (!pParser->parse(v_args) || pParser->exist("help")) {
    std::cout << "parse doneT cmd arguments failed, usage:" << pParser->usage()
              << std::endl;
    return -1;
  }
  return 0;
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
        for (auto &buff : vBuff) {
             ss << "get buff:\"" << buff << "\"  ";
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
        } else if (vBuff[0] == "list") {
          pLogger->info("get list cmd");
          listT(gmCmdParser["list"], vBuff, mNode);
          gmCmdParser["list"]->rest();
        }

    }
    return 0;
}
