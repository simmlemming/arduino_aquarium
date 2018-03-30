#include "Arduino.h"
#include "RTClib.h"

class ObservableDateTime {
  private:
    DateTime _value;
    void (*_onMinuteChanged)(DateTime time);
    void (*_onSecondChanged)(DateTime time);
    bool _is_minute_changed(DateTime first, DateTime second);
    bool _is_second_changed(DateTime first, DateTime second);
    
  public:
    ObservableDateTime(DateTime value, void (*onMinuteChanged)(DateTime time), void (*onSecondChanged)(DateTime time));
    void setValue(DateTime value);
    DateTime getValue();
};

