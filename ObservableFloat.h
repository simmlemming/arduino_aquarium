#include "Arduino.h"

class ObservableFloat {
  private:
    float _value;
    void (*_onChanged)(float old_value, float new_value);
    
  public:
    ObservableFloat::ObservableFloat(float value, void (*onChanged)(float old_value, float new_value));
    void setValue(float value);
    float getValue();
};

