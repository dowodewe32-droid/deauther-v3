#pragma once
namespace simplebutton {
    class Button {
    public:
        Button(int pin) {}
        void update() {}
        bool isPressed() { return false; }
        bool wasPressed() { return false; }
    };
}
