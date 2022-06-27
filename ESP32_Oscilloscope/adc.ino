void characterize_adc() {
  esp_adc_cal_characterize(
    (adc_unit_t)ADC_UNIT_1,
    (adc_atten_t)ADC_CHANNEL,
    (adc_bits_width_t)ADC_WIDTH_BIT_12,
    1100,
    &adc_chars);
}
