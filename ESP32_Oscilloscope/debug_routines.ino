#ifdef DEBUG_BUF
void debug_buffer() {
  ADC_Sampling();
  i2s_adc_disable(I2S_NUM_0);
  delay(1000);
  for (uint32_t i = 0; i < B_MULT * NUM_SAMPLES; i++) {
    for (int j = 0; j < 1; j++) {
      Serial.println(i2s_buff[i]);
    }
    delay(5);
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  i2s_adc_enable(I2S_NUM_0);
  while (1);
}
#endif
