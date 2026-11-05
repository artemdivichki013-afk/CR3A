#pragma once
#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

namespace cr3a
{
enum class Look { Graphite = 0, Aero, XP, Glass, Win98, Terminal, Paper };

struct Theme
{
    const char* name;
    Look look;
    juce::uint32 bgTop, bgBottom;        // window background
    juce::uint32 panelTop, panelBottom;  // panel body
    juce::uint32 panelEdge;
    juce::uint32 text, textDim;
    juce::uint32 accent, accent2;
    juce::uint32 knobLight, knobDark;
    juce::uint32 track;
    juce::uint32 displayBg, displayText, displayGrid;
    juce::uint32 ledOn, ledOff;
    const char* fonts;                   // preferred font names, ';' separated
    float radius;
    bool dark;

    juce::Colour c (juce::uint32 v) const { return juce::Colour (v); }
};

inline const std::array<Theme, 7>& themes()
{
    static const std::array<Theme, 7> list = { {
        // 0  graphite (default): grey / black / white
        { "CR3A Graphite", Look::Graphite,
          0xff2d2f33, 0xff1d1e21,  0xff36383d, 0xff2b2d31,  0xff484b51,
          0xffecedef, 0xff90949b,  0xfff4f4f5, 0xffb7bbc1,
          0xff63666d, 0xff26272a,  0xff18191b,
          0xff111214, 0xffe9eaec, 0xff2b2d31,
          0xfff4f4f5, 0xff4a4c51,
          "Segoe UI;Inter;Helvetica Neue;Arial", 10.0f, true },

        // 1  frutiger aero
        { "FruityGear Aero", Look::Aero,
          0xff6cc3ee, 0xffcdeeff,  0xc8ffffff, 0x78ffffff,  0xddffffff,
          0xff0c3d5e, 0xff3f7fa0,  0xff1d9ee8, 0xff5fd23a,
          0xfff2fbff, 0xff6ab9e4,  0x3a0c3d5e,
          0xff0a3b57, 0xffc4f4ff, 0x3067d4ff,
          0xff62dd34, 0x550c3d5e,
          "Segoe UI;Frutiger;Trebuchet MS;Arial", 14.0f, false },

        // 2  windows xp (luna)
        { "Windows XP", Look::XP,
          0xff5a8dea, 0xff3a6fd0,  0xfff6f4e8, 0xffece9d8,  0xff0b3fc9,
          0xff1a1a1a, 0xff5e5c50,  0xff2159d8, 0xff3b9b2a,
          0xffffffff, 0xffd2cdb6,  0xffb3ae98,
          0xff0f2455, 0xffc3dcff, 0x302a58b0,
          0xff39b035, 0xff9d9a88,
          "Tahoma;Verdana;Segoe UI;Arial", 8.0f, false },

        // 3  macOS liquid glass
        { "macOS Liquid Glass", Look::Glass,
          0xffa8c4ff, 0xffe8d4ff,  0x55ffffff, 0x22ffffff,  0x99ffffff,
          0xff1b1c21, 0x99202228,  0xff0a84ff, 0xff30d158,
          0xffffffff, 0xffd6def2,  0x30000000,
          0xcc12131a, 0xfff2f4ff, 0x25ffffff,
          0xff30d158, 0x40000000,
          "SF Pro Display;Segoe UI Variable;Segoe UI;Helvetica Neue;Arial", 18.0f, false },

        // 4  windows 98
        { "Windows 98", Look::Win98,
          0xffc0c0c0, 0xffc0c0c0,  0xffc0c0c0, 0xffc0c0c0,  0xff808080,
          0xff000000, 0xff404040,  0xff000080, 0xff008080,
          0xffdfdfdf, 0xff808080,  0xff808080,
          0xff000000, 0xff2cff4a, 0xff0d3a14,
          0xff22ff44, 0xff5a5a5a,
          "MS Sans Serif;Tahoma;Microsoft Sans Serif;Arial", 0.0f, false },

        // 5  terminal green (the look of the original preset)
        { "Terminal Green", Look::Terminal,
          0xff08130c, 0xff040905,  0xff0c1b11, 0xff08130c,  0xff1f5d33,
          0xffb0f7bf, 0xff4fa165,  0xff3ae872, 0xffa3ff62,
          0xff4a5d4f, 0xff111a14,  0xff0a1a10,
          0xff07180d, 0xff7dff9d, 0xff12331c,
          0xff3ae872, 0xff17361f,
          "Consolas;Cascadia Mono;Courier New;Menlo", 3.0f, true },

        // 6  paper (light, minimal)
        { "Paper", Look::Paper,
          0xffedece8, 0xffe1e0db,  0xfff9f8f5, 0xfff0efeb,  0xffd2d0c9,
          0xff1b1b1b, 0xff7b7972,  0xff1b1b1b, 0xff77756f,
          0xffffffff, 0xffd8d6cf,  0xffd0cec7,
          0xff1d1d1d, 0xffeeeeee, 0xff353535,
          0xff1b1b1b, 0xffbdbbb4,
          "Segoe UI;Inter;Helvetica Neue;Arial", 7.0f, false },
    } };
    return list;
}
} // namespace cr3a
