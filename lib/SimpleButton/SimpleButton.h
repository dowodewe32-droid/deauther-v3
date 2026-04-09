#pragma once

#include <functional>

namespace simplebutton {
    typedef std::function<void()> Callback;

    class Button {
    public:
        Button(int pin) {}
        virtual ~Button() {}
        virtual void update() {}
        virtual bool isPressed() { return false; }
        virtual bool wasPressed() { return false; }
        virtual bool read() { return false; }
        virtual void setOnClicked(Callback) {}
        virtual void setOnHolding(Callback, unsigned long = 0) {}
        virtual void setOnReleased(Callback) {}
    };

    class ButtonPullup : public Button {
    public:
        ButtonPullup(int pin) : Button(pin) {}
        void update() override {}
        bool isPressed() override { return false; }
        bool wasPressed() override { return false; }
        bool read() override { return false; }
        void setOnClicked(Callback c) override {}
        void setOnHolding(Callback c, unsigned long t=0) override {}
        void setOnReleased(Callback c) override {}
    };
}
