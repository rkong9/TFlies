#ifndef _VIEWER_HPP_
#define _VIEWER_HPP_
#include <vector>
#include <string>

std::string renderDailyEffGraph(const std::vector<float> &vEff);
extern uint8_t effColorMap[8][3];

#endif
