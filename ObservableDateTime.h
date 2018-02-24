#include "Arduino.h"
#include "RTClib.h"

class ObservableDateTime {
  private:
    DateTime _value;
    void (*_onChanged)(DateTime old_value, DateTime new_value);
    bool _eq(DateTime first, DateTime second);
    
  public:
    ObservableDateTime::ObservableDateTime(DateTime value, void (*onChanged)(DateTime old_value, DateTime new_value));
    void setValue(DateTime value);
    DateTime getValue();
};

