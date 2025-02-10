#ifndef _TF_DATA_TYPE_HPP_
#define _TF_DATA_TYPE_HPP_
#include <string>

enum class TaskStatus
{
  TODO = 0,
  INPROGRESS,
  PAUSE,
  DONE,
};

enum class Priority
{
  UNDEFINED,
  TRIVAL,
  MINOR,
  MAJOR,
  CRITICAL,
  BLOCK,
};

enum class Efficiency
{
    UNDEFINED,
    EXTREM_LOW,
    VERY_LOW,
    LOW,
    NORMAL,
    HIGH,
    VERY_HIGH,
    EXTREM_HIGH,
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
    taskID = -1;
    parentTaskID = -1;
    status = static_cast<uint8_t>(TaskStatus::TODO);
    priority = static_cast<uint8_t>(Priority::UNDEFINED);
    efficiency = static_cast<uint8_t>(Efficiency::UNDEFINED);
    createTime = 0;
    updateTime = 0;
    dueTime = -1;
    costTime = 0;
    expectTime = -1;
    name = "None";
    timePiecesTable = "None";
    desc = "None";
  }
};

struct TPieces
{
  int64_t piecesID;
  int64_t taskID;
  uint32_t serialNumber;
  uint8_t efficiency;
  int64_t begintime;
  int64_t endtime;
  std::string desc; // description
  int status; // ignore(0)/write(1)/delete(-1)..
  TPieces() {
    piecesID = -1;
    taskID = -1;
    serialNumber = 0;
    efficiency = 0;
    begintime = -1;
    endtime = -1;
    desc = "None";
    status = 0;
  }
};

#endif
