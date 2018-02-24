#include "Arduino.h"
#include "ObservableDateTime.h"


ObservableDateTime::ObservableDateTime(DateTime value, void (*onChanged)(DateTime old_value, DateTime new_value)) {
  _value = value;
  _onChanged = onChanged;
}

void ObservableDateTime::setValue(DateTime value) {
  if (_eq(_value, value)) {
    return;
  }

  DateTime old_value = _value;
  _value = value;

  _onChanged(old_value, _value);
}

DateTime ObservableDateTime::getValue() {
  return _value;
}

bool ObservableDateTime::_eq(DateTime first, DateTime second) {
  if (first.hour() != second.hour()) {
    return false;
  }

  if (first.minute() != second.minute()) {
    return false;
  }

  return true;
}

