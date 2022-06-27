class low_pass {
  public:

    low_pass(int factor) {
      _factor = factor;
    }

    float filter(float reading) {
      _value = _value * (_factor) + reading * (1.0 - _factor);
      return _value;
    }

    float _value = 0;
    float _factor = 0.99;

};


class mean_filter {
  public:

    mean_filter(int values) {
      _values = values;
    }

    void init(float value){
      for(int i=0;i<_values;i++){
        _data[i] = value;
      }
    }
    float filter(float reading) {
      float temp = 0;
      _data[_values - 1] = reading;
      for (int i = 0; i < _values - 1; i++) {
        temp += _data[i];
        _data[i] = _data[i + 1];
      }
      temp += reading;
      return temp / float(_values);
    }
    
    int _values = 5;
    float _data[100] = {0};
};
