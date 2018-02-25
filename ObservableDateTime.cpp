#include "Arduino.h"
#include "ObservableDateTime.h"


ObservableDateTime::ObservableDateTime(DateTime value, void (*onMinuteChanged)(DateTime time), void (*onSecondChanged)(DateTime time)) {
  _value = value;
  _onMinuteChanged = onMinuteChanged;
  _onSecondChanged = onSecondChanged;
}

void ObservableDateTime::setValue(DateTime value) {
  if (_is_minute_changed(_value, value)) {
    _onMinuteChanged(value);
  }

  if (_is_second_changed(_value, value)) {
    _onSecondChanged(value);
  }

  DateTime old_value = _value;
  _value = value;
}

DateTime ObservableDateTime::getValue() {
  return _value;
}

bool ObservableDateTime::_is_minute_changed(DateTime first, DateTime second) {
  return  first.minute() != second.minute();
}

bool ObservableDateTime::_is_second_changed(DateTime first, DateTime second) {
  return  first.second() != second.second();
}

