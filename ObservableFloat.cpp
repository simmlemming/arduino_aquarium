#include "Arduino.h"
#include "ObservableFloat.h"


ObservableFloat::ObservableFloat(float value, void (*onChanged)(float old_value, float new_value)) {
  _value = value;
  _onChanged = onChanged;
}

void ObservableFloat::setValue(float value) {
  if (_value == value) {
    return;
  }

  float old_value = _value;
  _value = value;

  _onChanged(old_value, _value);
}

float ObservableFloat::getValue() {
  return _value;
}

