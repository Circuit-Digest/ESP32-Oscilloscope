void peak_mean(uint16_t *i2s_buffer, uint32_t len, float * max_value, float * min_value, float *pt_mean) {
  max_value[0] = i2s_buffer[0];
  min_value[0] = i2s_buffer[0];
  mean_filter filter(5);
  filter.init(i2s_buffer[0]);

  float mean = 0;
  for (uint32_t i = 1; i < len; i++) {

    float value = filter.filter((float)i2s_buffer[i]);
    if (value > max_value[0])
      max_value[0] = value;
    if (value < min_value[0])
      min_value[0] = value;

    mean += i2s_buffer[i];
  }
  mean /= float(BUFF_SIZE);
  mean = to_voltage(mean);
  pt_mean[0] = mean;
}


//true if digital/ false if analog
bool digital_analog(uint16_t *i2s_buffer, uint32_t max_v, uint32_t min_v) {
  uint32_t upper_threshold = max_v - 0.05 * (max_v - min_v);
  uint32_t lower_threshold = min_v + 0.05 * (max_v - min_v);
  uint32_t digital_data = 0;
  uint32_t analog_data = 0;
  for (uint32_t i = 0; i < BUFF_SIZE; i++) {
    if (i2s_buffer[i] > lower_threshold) {
      if (i2s_buffer[i] > upper_threshold) {
        //HIGH DIGITAL
        digital_data++;
      }
      else {
        //ANALOG/TRANSITION
        analog_data++;
      }
    }
    else {
      //LOW DIGITAL
      digital_data++;
    }
  }

  //more than 50% of data is analog
  if (analog_data < digital_data)
    return true;

  return false;
}

void trigger_freq_analog(uint16_t *i2s_buffer,
                         float sample_rate,
                         float mean,
                         uint32_t max_v,
                         uint32_t min_v,
                         float *pt_freq,
                         float *pt_period,
                         uint32_t *pt_trigger0,
                         uint32_t *pt_trigger1) {
  float freq = 0;
  float period = 0;
  bool signal_side = false;
  uint32_t trigger_count = 0;
  uint32_t trigger_num = 10;
  uint32_t trigger_temp[trigger_num] = {0};
  uint32_t trigger_index = 0;

  //get initial signal relative to the mean
  if (to_voltage(i2s_buffer[0]) > mean) {
    signal_side = true;
  }


  //waveform repetitions calculation + get triggers time
  uint32_t wave_center = (max_v + min_v) / 2;
  for (uint32_t i = 1 ; i < BUFF_SIZE; i++) {
    if (signal_side && i2s_buffer[i] < wave_center - (wave_center - min_v) * 0.2) {
      signal_side = false;
    }
    else if (!signal_side && i2s_buffer[i] > wave_center + (max_v - wave_center) * 0.2) {
      freq++;
      if (trigger_count < trigger_num) {
        trigger_temp[trigger_count] = i;
        trigger_count++;
      }
      signal_side = true;
    }
  }

  //frequency calculation
  if (trigger_count < 2) {
    trigger_temp[0] = 0;
    trigger_index = 0;
    freq = 0;
    period = 0;
  }
  else {

    //simple frequency calculation fair enough for frequencies over 2khz (20hz resolution)
    freq = freq * 1000 / 50;
    period = (float)(sample_rate * 1000.0) / freq; //us

    //from 2000 to 80 hz -> uses mean of the periods for precision
    if (freq < 2000 && freq > 80) {
      period = 0;
      for (uint32_t i = 1; i < trigger_count; i++) {
        period += trigger_temp[i] - trigger_temp[i - 1];
      }
      period /= (trigger_count - 1);
      freq = sample_rate * 1000 / period;
    }

    //under 80hz, single period for frequency calculation
    else if (trigger_count > 1 && freq <= 80) {
      period = trigger_temp[1] - trigger_temp[0];
      freq = sample_rate * 1000 / period;
    }
  }

  //setting triggers offset and getting second trigger for debug cursor on drawn_channel1
  /*
     The trigger function uses a rise porcentage (5%) obove the mean, thus,
     the real waveform starting point is some datapoints back.
     The resulting trigger gets a negative offset of 5% of the calculated period
  */
  uint32_t trigger2 = 0;
  if (trigger_temp[0] - period * 0.05 > 0 && trigger_count > 1) {
    trigger_index = trigger_temp[0] - period * 0.05;
    trigger2 = trigger_temp[1] - period * 0.05;
  }
  else if (trigger_count > 2) {
    trigger_index = trigger_temp[1] - period * 0.05;
    if (trigger_count > 2)
      trigger2 = trigger_temp[2] - period * 0.05;
  }


  pt_trigger0[0] = trigger_index;
  pt_trigger1[0] = trigger2;
  pt_freq[0] = freq;
  pt_period[0] = period;

}


void trigger_freq_digital(uint16_t *i2s_buffer,
                          float sample_rate,
                          float mean,
                          uint32_t max_v,
                          uint32_t min_v,
                          float *pt_freq,
                          float *pt_period,
                          uint32_t *pt_trigger0) {

  float freq = 0;
  float period = 0;
  bool signal_side = false;
  uint32_t trigger_count = 0;
  uint32_t trigger_num = 10;
  uint32_t trigger_temp[trigger_num] = {0};
  uint32_t trigger_index = 0;

  //get initial signal relative to the mean
  if (to_voltage(i2s_buffer[0]) > mean) {
    signal_side = true;
  }

  //waveform repetitions calculation + get triggers time
  uint32_t wave_center = (max_v + min_v) / 2;
  bool normal_high = (mean > to_voltage(wave_center)) ? true : false;
  if (max_v - min_v > 4095 * (0.4 / 3.3)) {
    for (uint32_t i = 1 ; i < BUFF_SIZE; i++) {
      if (signal_side && i2s_buffer[i] < wave_center - (wave_center - min_v) * 0.2) {

        //signal was high, fell -> trigger if normal high
        if (trigger_count < trigger_num && normal_high) {
          trigger_temp[trigger_count] = i;
          trigger_count++;
        }

        signal_side = false;

      }
      else if (!signal_side && i2s_buffer[i] > wave_center + (max_v - wave_center) * 0.2) {
        freq++;

        //signal was low, rose -> trigger if normal low
        if (trigger_count < trigger_num && !normal_high) {
          trigger_temp[trigger_count] = i;
          trigger_count++;
        }

        signal_side = true;
      }
    }

    freq = freq * 1000 / 50;
    period = (float)(sample_rate * 1000.0) / freq; //us

    if (trigger_count > 1) {
      //from 2000 to 80 hz -> uses mean of the periods for precision
      if (freq < 2000 && freq > 80) {
        period = 0;
        for (uint32_t i = 1; i < trigger_count; i++) {
          period += trigger_temp[i] - trigger_temp[i - 1];
        }
        period /= (trigger_count - 1);
        freq = sample_rate * 1000 / period;
      }

      //under 80hz, single period for frequency calculation
      else if (trigger_count > 1 && freq <= 80) {
        period = trigger_temp[1] - trigger_temp[0];
        freq = sample_rate * 1000 / period;
      }
    }
    
    trigger_index = trigger_temp[0];

    if (trigger_index > 10)
      trigger_index -= 10;
    else
      trigger_index = 0;
  }




  pt_trigger0[0] = trigger_index;
  pt_freq[0] = freq;
  pt_period[0] = period;

}
