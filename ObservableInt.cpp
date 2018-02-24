#include "Arduino.h"
#include "ObservableInt.h"


ObservableInt::ObservableInt(int value, void (*onChanged)(int old_value, int new_value)) {
  _value = value;
  _onChanged = onChanged;
}

void ObservableInt::setValue(int value) {
  if (_value == value) {
    return;
  }

  int old_value = _value;
  _value = value;

  _onChanged(old_value, _value);
}

int ObservableInt::getValue() {
  return _value;
}

