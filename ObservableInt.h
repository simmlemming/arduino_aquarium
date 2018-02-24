#include "Arduino.h"

class ObservableInt {
  private:
    int _value;
    void (*_onChanged)(int old_value, int new_value);
    
  public:
    ObservableInt::ObservableInt(int value, void (*onChanged)(int old_value, int new_value));
    void setValue(int value);
    int getValue();
};

