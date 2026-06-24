#pragma once
enum YCC { YUV444, YUV422, YUV411, YUV440, YUV420, YUV410, GRAY };

constexpr uint8_t YCC_HV[7][2] = {
    // {Luma HV},  {Chroma HV}
    {0x11, 0x11},  // 444
    {0x21, 0x11},  // 422 実行できる
    {0x41, 0x11},  // 411 実行できる
    {0x12, 0x11},  // 440 実行できる
    {0x22, 0x11},  // 420 うまくいかない<-縦を間引いているため(?)
    {0x42, 0x11},  // 410
    {0x11, 0x00},  // GRAY
};