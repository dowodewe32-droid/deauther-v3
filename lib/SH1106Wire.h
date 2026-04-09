#pragma once
#include <Wire.h>
class SH1106Wire {
public:
    SH1106Wire(uint8_t addr, int sda, int scl) {}
    void init() {}
    void setContrast(uint8_t c) {}
    void display() {}
    void setFont(const uint8_t* f) {}
    void setTextAlignment(int a) {}
    void drawString(int x, int y, const String& s) {}
    void drawString(int row, const String& s) {}
    void drawLine(int x1, int y1, int x2, int y2) {}
    void clear() {}
};
