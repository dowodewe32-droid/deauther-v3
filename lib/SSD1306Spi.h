#pragma once
#include <SPI.h>
class SSD1306Spi {
public:
    SSD1306Spi(int res, int dc, int cs) {}
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
