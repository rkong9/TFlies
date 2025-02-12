#include "viewer.hpp"
#include "utils.hpp"

uint8_t effColorMap[8][3] = {
    {128, 128, 128},    // 灰色
    {220, 20, 60},    // 深红色
    {255, 69, 0},     // 红色
    {255, 140, 0},    // 橙色
    {255, 255, 0},    // 黄色
    {173, 255, 47},   // 黄绿色
    {0, 255, 0},      // 绿色
    {0, 100, 0}       // 深绿色
};

uint8_t prioColorMap[6][3] = {
    {128, 128, 128},  // 灰色
    {220, 20, 60},    // 深红色
    {255, 140, 0},    // 橙色
    {255, 255, 0},    // 黄色
    {173, 255, 47},   // 黄绿色
    {0, 255, 0},      // 绿色
};

// 右半边通过反色实现
const std::vector<std::string> viewblocks = {
    "\u258F", // ▏ 1/8 宽
    "\u258E", // ▎ 2/8 宽
    "\u258D", // ▍ 3/8 宽
    "\u258C", // ▌ 4/8 宽
    "\u258B", // ▋ 5/8 宽
    "\u258A", // ▊ 6/8 宽
    "\u2589", // ▉ 7/8 宽
    "\u2588"  // █ 8/8 宽（全块)
};

std::string renderDailyEffGraph(const std::vector<ViewData> &vData, int chars) {
    std::string effGraph;

    // 6 rows, 5min / per grid
    std::vector<int> vColor;
    std::vector<float> vEffi;

    for (auto &vd : vData) {


    }

    return effGraph;
}
