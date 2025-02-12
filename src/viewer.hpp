#ifndef _VIEWER_HPP_
#define _VIEWER_HPP_
#include <vector>
#include <string>

struct ViewData {
    float beg;
    float end;
    float val;
    ViewData() {
        beg = 0;
        end = 0;
        val = 0;
    }
};

std::string renderDailyEffGraph(const std::vector<ViewData> &vEff, int chars);
extern uint8_t effColorMap[8][3];
extern uint8_t prioColorMap[6][3];

#endif
