#include "Arduino.h"
#include "Button.h"

Button::Button(int pin) {
  _pin = pin;
  pinMode(_pin, INPUT_PULLUP);
}

void Button::loop() {
  int state = digitalRead(_pin);
  if (state != _prev_state) {
    _state_change_millis = millis();
  }

  if (millis() - _state_change_millis > 20) {
    _onDebouncedStateChange(state);
  }

  _prev_state = state;
}

void Button::_onDebouncedStateChange(int state) {
  if (state == LOW) {
    if (!_onClickCalled) {
      _onClick();
      _onClickCalled = true;
    }
  } else {
    _onClickCalled = false;
  }
}

void Button::onClick(void (*onClick)()) {
  _onClick = onClick;
}

