void setup_screen() {
  // Initialise the TFT registers
  tft.init();
  tft.setRotation(1);

  // Optionally set colour depth to 8 or 16 bits, default is 16 if not spedified
   spr.setColorDepth(8);

  // Create a sprite of defined size
  spr.createSprite(HEIGHT, WIDTH);
  // Clear the TFT screen to blue
  tft.fillScreen(TFT_BLACK);
}

int data[280] = {0};

float to_scale(float reading) {
  float temp = WIDTH -
               (
                 (
                   (
                     (float)((reading - 20480.0) / 4095.0)
                     + (offset / 3.3)
                   )
                   * 3300 /
                   (v_div * 6)
                 )
               )
               * (WIDTH - 1)
               - 1;
  return temp;
}

float to_voltage(float reading) {
  return  (reading - 20480.0) / 4095.0 * 3.3;
}

uint32_t from_voltage(float voltage) {
  return uint32_t(voltage / 3.3 * 4095 + 20480.0);
}

void update_screen(uint16_t *i2s_buff, float sample_rate) {

  float mean = 0;
  float max_v, min_v;

  peak_mean(i2s_buff, BUFF_SIZE, &max_v, &min_v, &mean);

  float freq = 0;
  float period = 0;
  uint32_t trigger0 = 0;
  uint32_t trigger1 = 0;

  //if analog mode OR auto mode and wave recognized as analog
  bool digital_data = false;
  if (digital_wave_option == 1) {
    trigger_freq_analog(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0, &trigger1);
  }
  else if (digital_wave_option == 0) {
    digital_data = digital_analog(i2s_buff, max_v, min_v);
    if (!digital_data) {
      trigger_freq_analog(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0, &trigger1);
    }
    else {
      trigger_freq_digital(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0);
    }
  }
  else {
    trigger_freq_digital(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0);
  }

  draw_sprite(freq, period, mean, max_v, min_v, trigger0, sample_rate, digital_data, true);
}

void draw_sprite(float freq,
                 float period,
                 float mean,
                 float max_v,
                 float min_v,
                 uint32_t trigger,
                 float sample_rate,
                 bool digital_data,
                 bool new_data
                ) {

  max_v = to_voltage(max_v);
  min_v = to_voltage(min_v);

  String frequency = "";
  if (freq < 1000)
    frequency = String(freq) + "hz";
  else if (freq < 100000)
    frequency = String(freq / 1000) + "khz";
  else
    frequency = "----";

  String s_mean = "";
  if (mean > 1.0)
    s_mean = "Avg: " + String(mean) + "V";
  else
    s_mean = "Avg: " + String(mean * 1000.0) + "mV";

  String str_filter = "";
  if (current_filter == 0)
    str_filter = "None";
  else if (current_filter == 1)
    str_filter = "Pixel";
  else if (current_filter == 2)
    str_filter = "Mean-5";
  else if (current_filter == 3)
    str_filter = "Lpass9";

  String str_stop = "";
  if (!stop)
    str_stop = "RUNNING";
  else
    str_stop = "STOPPED";

  String wave_option = "";
  if (digital_wave_option == 0)
    if (digital_data )
      wave_option = "AUTO:Dig./data";
    else
      wave_option = "AUTO:Analog";
  else if (digital_wave_option == 1)
    wave_option = "MODE:Analog";
  else
    wave_option = "MODE:Dig./data";


  if (new_data) {
    // Fill the whole sprite with black (Sprite is in memory so not visible yet)
    spr.fillSprite(TFT_BLACK);

    draw_grid();

    if (auto_scale) {
      auto_scale = false;
      v_div = 1000.0 * max_v / 6.0;
      s_div = period / 3.5;
      if (s_div > 7000 || s_div <= 0)
        s_div = 7000;
      if (v_div <= 0)
        v_div = 550;
    }

    //only draw digital data if a trigger was in the data
    if (!(digital_wave_option == 2 && trigger == 0))
      draw_channel1(trigger, 0, i2s_buff, sample_rate);
  }

  int shift = 150;
  if (menu) {
    spr.drawLine( 0, 120, 280, 120, TFT_WHITE); //center line
    spr.fillRect(shift, 0, 102, 135, TFT_BLACK);
    spr.drawRect(shift, 0, 102, 135, TFT_WHITE);
    spr.fillRect(shift + 1, 3 + 10 * (opt - 1), 100, 11, TFT_RED);

    spr.drawString("AUTOSCALE",  shift + 5, 5);
    spr.drawString(String(int(v_div)) + "mV/div",  shift + 5, 15);
    spr.drawString(String(int(s_div)) + "uS/div",  shift + 5, 25);
    spr.drawString("Offset: " + String(offset) + "V",  shift + 5, 35);
    spr.drawString("T-Off: " + String((uint32_t)toffset) + "uS",  shift + 5, 45);
    spr.drawString("Filter: " + str_filter, shift + 5, 55);
    spr.drawString(str_stop, shift + 5, 65);
    spr.drawString(wave_option, shift + 5, 75);
    spr.drawString("Single " + String(single_trigger ? "ON" : "OFF"), shift + 5, 85);

    spr.drawLine(shift, 103, shift + 100, 103, TFT_WHITE);

    spr.drawString("Vmax: " + String(max_v) + "V",  shift + 5, 105);
    spr.drawString("Vmin: " + String(min_v) + "V",  shift + 5, 115);
    spr.drawString(s_mean,  shift + 5, 125);

    shift -= 70;

    //spr.fillRect(shift, 0, 70, 30, TFT_BLACK);
    spr.drawRect(shift, 0, 70, 30, TFT_WHITE);
    spr.drawString("P-P: " + String(max_v - min_v) + "V",  shift + 5, 5);
    spr.drawString(frequency,  shift + 5, 15);
    String offset_line = String((2.0 * v_div) / 1000.0 - offset) + "V";
    spr.drawString(offset_line,  shift + 40, 59);

    if (set_value) {
      spr.fillRect(229, 0, 11, 11, TFT_BLUE);
      spr.drawRect(229, 0, 11, 11, TFT_WHITE);
      spr.drawLine(231, 5, 238 , 5, TFT_WHITE);
      spr.drawLine(234, 2, 234, 8, TFT_WHITE);


      spr.fillRect(229, 124, 11, 11, TFT_BLUE);
      spr.drawRect(229, 124, 11, 11, TFT_WHITE);
      spr.drawLine(231, 129, 238, 129, TFT_WHITE);
    }
  }
  else if (info) {
    spr.drawLine( 0, 120, 280, 120, TFT_WHITE); //center line
    //spr.drawRect(shift + 10, 0, 280 - shift - 20, 30, TFT_WHITE);
    spr.drawString("P-P: " + String(max_v - min_v) + "V",  shift + 15, 5);
    spr.drawString(frequency,  shift + 15, 15);
    spr.drawString(String(int(v_div)) + "mV/div",  shift - 100, 5);
    spr.drawString(String(int(s_div)) + "uS/div",  shift - 100, 15);
    String offset_line = String((2.0 * v_div) / 1000.0 - offset) + "V";
    spr.drawString(offset_line,  shift + 100, 112);
  }


  //push the drawed sprite to the screen
  spr.pushSprite(0, 0);

  yield(); // Stop watchdog reset
}

void draw_grid() {

  for (int i = 0; i < 28; i++) {
    spr.drawPixel(i * 10, 40, TFT_WHITE);
    spr.drawPixel(i * 10, 80, TFT_WHITE);
    spr.drawPixel(i * 10, 120, TFT_WHITE);
    spr.drawPixel(i * 10, 160, TFT_WHITE);
    spr.drawPixel(i * 10, 200, TFT_WHITE);
  }
  for (int i = 0; i < 240; i += 10) {
    for (int j = 0; j < 280; j += 40) {
      spr.drawPixel(j, i, TFT_WHITE);
    }
  }
}

void draw_channel1(uint32_t trigger0, uint32_t trigger1, uint16_t *i2s_buff, float sample_rate) {
  //screen wave drawing
  data[0] = to_scale(i2s_buff[trigger0]);
  low_pass filter(0.99);
  mean_filter mfilter(5);
  mfilter.init(i2s_buff[trigger0]);
  filter._value = i2s_buff[trigger0];
  float data_per_pixel = (s_div / 40.0) / (sample_rate / 1000);

  //  uint32_t cursor = (trigger1-trigger0)/data_per_pixel;
  //  spr.drawLine(cursor, 0, cursor, 135, TFT_RED);

  uint32_t index_offset = (uint32_t)(toffset / data_per_pixel);
  trigger0 += index_offset;  
  uint32_t old_index = trigger0;
  float n_data = 0, o_data = to_scale(i2s_buff[trigger0]);
  for (uint32_t i = 1; i < 280; i++) {
    uint32_t index = trigger0 + (uint32_t)((i + 1) * data_per_pixel);
    if (index < BUFF_SIZE) {
      if (full_pix && s_div > 40 && current_filter == 0) {
        uint32_t max_val = i2s_buff[old_index];
        uint32_t min_val = i2s_buff[old_index];
        for (int j = old_index; j < index; j++) {
          //draw lines for all this data points on pixel i
          if (i2s_buff[j] > max_val)
            max_val = i2s_buff[j];
          else if (i2s_buff[j] < min_val)
            min_val = i2s_buff[j];

        }
        spr.drawLine(i, to_scale(min_val), i, to_scale(max_val), TFT_BLUE);
      }
      else {
        if (current_filter == 2)
          n_data = to_scale(mfilter.filter((float)i2s_buff[index]));
        else if (current_filter == 3)
          n_data = to_scale(filter.filter((float)i2s_buff[index]));
        else
          n_data = to_scale(i2s_buff[index]);

        spr.drawLine(i - 1, o_data, i, n_data, TFT_BLUE);
        o_data = n_data;
      }

    }
    else {
      break;
    }
    old_index = index;
  }
}
