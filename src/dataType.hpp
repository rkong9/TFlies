#ifndef _TF_DATA_TYPE_HPP_
#define _TF_DATA_TYPE_HPP_
#include <string>

enum class TaskStatus
{
  TODO = 0,
  INPROGRESS,
  PENDING,
  DONE,
};

enum class Priority
{
  TRIVAL,
  MINOR,
  MAJOR,
  CRITICAL,
  BLOCK,
};

struct Item
{
  int64_t taskID;
  int64_t parentTaskID;
  uint8_t status;
  uint8_t priority;
  uint8_t efficiency;
  int64_t createTime;
  int64_t updateTime;
  int64_t dueTime;
  int64_t costTime;
  int64_t expectTime;
  std::string name;
  std::string timePiecesTable;
  std::string desc; // description
  Item() {
    taskID = 1010;
    parentTaskID = 10;
    status = static_cast<int>(TaskStatus::TODO);
    priority = static_cast<int>(Priority::MINOR);
    efficiency = 0;
    createTime = 0;
    updateTime = 0;
    dueTime = -1;
    costTime = -1;
    expectTime = -1;
    name = "None";
    timePiecesTable = "None";
    desc = "None";
  }
};

struct TPieces
{
  uint64_t piecesID;
  uint64_t taskID;
  uint32_t serialNumber;
  uint8_t efficiency;
  int64_t begintime;
  int64_t endtime;
  std::string desc; // description
};

#endif
