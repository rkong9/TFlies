#include "viewer.hpp"
#include "utils.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

uint8_t effColorMap[8][3] = {
    {128, 128, 128},    // 灰色
    {0xeb, 0x4a, 0x2d},    // 深红色
    {0xe5, 0xa5, 0x33},     // 红色
    {0xdf, 0xa1, 0x39},    // 橙色
    {0xd9, 0xcc, 0x3f},    // 黄色
    {0xaf, 0xc8, 0x3a},   // 黄绿色
    {0x85, 0xc5, 0x34},      // 绿色
    {0x5b, 0xc1, 0x2f}       // 深绿色
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
    "\u0020", // 空字符
    "\u258F", // ▏ 1/8 宽
    "\u258E", // ▎ 2/8 宽
    "\u258D", // ▍ 3/8 宽
    "\u258C", // ▌ 4/8 宽
    "\u258B", // ▋ 5/8 宽
    "\u258A", // ▊ 6/8 宽
    "\u2589", // ▉ 7/8 宽
    "\u2588"  // █ 8/8 宽（全块)
};

std::string renderDailyEffGraph(const std::vector<ViewData> &vData, int rows, int cols) {
    std::string effGraph;
    if (rows <= 0 || cols <= 0) {
        return effGraph;
    }

    // 6 rows, 5min / per grid
    int grid_size(rows * cols);
    std::vector<ViewData> vColor(grid_size, ViewData(0.0, 1.0, -1.0));
    const int MAX_COLOR(8);
    const int MAX_BLOCK(8);

    std::stringstream debug;
    for (auto &vd : vData) {
        float fbeg = vd.beg * grid_size;
        float fend = vd.end * grid_size;

        int i_beg = static_cast<int>(std::floor(fbeg));
        int i_end = static_cast<int>(std::floor(fend));

        if (i_beg < 0 || i_beg >= grid_size || i_end < 0 || i_end >= grid_size) {
            continue;
        }

        // mixture two edge
        float f_beg_res = fbeg - i_beg;
        if (vColor[i_beg].val < 0) { // empty num
            vColor[i_beg].beg = f_beg_res;
            vColor[i_beg].val = vd.val;
        } else {
            vColor[i_beg].val = (vColor[i_beg].end - vColor[i_beg].beg) * vColor[i_beg].val +
                (1.0 - f_beg_res) * vd.val;

            vColor[i_beg].beg = 0.0;
            vColor[i_beg].end = 1.0;
        }


        float f_end_res = fend - i_end;
        if (vColor[i_end].val < 0) {
            vColor[i_end].end = f_end_res;
            vColor[i_end].val = vd.val;
        } else {
            vColor[i_end].val = (vColor[i_end].end - vColor[i_end].beg) * vColor[i_end].val +
                f_end_res * vd.val;

            vColor[i_end].beg = 0.0;
            vColor[i_end].end = 1.0;
        }

        for (int i = i_beg + 1; i < i_end; i++) {
            vColor[i].val = vd.val;
        }
    }

    float last_color_idx(-1);
    uint8_t bg[3] = {23, 44, 60};
    std::string scolor = getTrueColors(bg[0], bg[1], bg[2], M_BACKGROUND);

    for (size_t i = 0; i < vColor.size(); i++) {
        bool reverse(false);
        if (i % cols == 0) {
            int begH = (i * 24 / vColor.size());
            int endH = ((i + cols) * 24 / vColor.size());
            std::stringstream ss;
            if (i == 0) {
                ss << "           " << scolor;
                for (int i = 0; i < cols; i++) {
                    if ((i + 1) % 10 == 0) {
                        ss << viewblocks[MAX_BLOCK];
                    } else {
                        ss << std::to_string((i + 1) % 10);
                    }
                }
                ss << RESET << "\n";
            }
            ss << RESET << std::string(WHITE) << std::setfill('0') << std::setw(2)
               << std::to_string(begH) << " --- " << std::setfill('0') << std::setw(2)
               << std::to_string(endH) << ": " << RESET << scolor;

            effGraph += ss.str();
            last_color_idx = -1;
        }

        auto &vd = vColor[i];
        std::string values(viewblocks[0]);

        do {
            if (vd.val < 0 || vd.val >= MAX_COLOR) { // color is invalid
                vd.val = -2;
                break;
            }

            if ((vd.end - vd.beg) > 0.95) { // full grid
                values = viewblocks[MAX_BLOCK];
            } else {
                if (vd.end == 1.0) { //取反色
                    int i_blocks = static_cast<int>(std::round(vd.beg / 0.125));
                    if (i_blocks >= 0 && i_blocks <= MAX_BLOCK) {
                        values = REVERSE + viewblocks[i_blocks] + RESET;
                        reverse = true;
                    } else {
                        values = viewblocks[0];
                    }
                } else { // 不取反色
                    int i_blocks = static_cast<int>(std::round(vd.end / 0.125));
                    if (i_blocks >= 0 && i_blocks <= MAX_BLOCK) {
                        values = viewblocks[i_blocks];
                    } else {
                        values = viewblocks[0];
                    }
                }
            }
        } while(0);

        int c_idx = static_cast<int>(std::floor(vd.val));
        if (vd.val >= 0 && vd.val < 8) {
            float ratio = vd.val - c_idx;
            if (ratio > 0.1 && c_idx < 8) { // mixture color
                float r = static_cast<float>(effColorMap[c_idx + 1][0]) - static_cast<float>(effColorMap[c_idx][0]);
                float g = static_cast<float>(effColorMap[c_idx + 1][1]) - static_cast<float>(effColorMap[c_idx][1]);
                float b = static_cast<float>(effColorMap[c_idx + 1][2]) - static_cast<float>(effColorMap[c_idx][2]);
                r = r * ratio + effColorMap[c_idx][0];
                g = g * ratio + effColorMap[c_idx][1];
                b = b * ratio + effColorMap[c_idx][2];
                scolor = getTrueColorsWithBg(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b),
                    bg[0], bg[1], bg[2]);
            } else {
                auto &fg = effColorMap[c_idx];
                scolor = getTrueColorsWithBg(fg[0], fg[1], fg[2], bg[0], bg[1], bg[2]);
                    // getTrueColors(effColorMap[c_idx][0], effColorMap[c_idx][1], effColorMap[c_idx][2]);
            }
        }

        if ((float)c_idx == last_color_idx) {
            effGraph += values;
        } else {
            effGraph += RESET + scolor + values;
            last_color_idx = vd.val;
        }

        if (reverse) {
            effGraph += scolor;
        }

        if ((i + 1) % cols == 0 && (i + 1) != vColor.size()) {
            effGraph += "\033[0m\n";
        }
    }
    effGraph += RESET + debug.str();

    return effGraph;
}
