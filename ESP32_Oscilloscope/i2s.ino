void configure_i2s(int rate) {
  /*keep in mind:
     dma_buf_len * dma_buf_count * bits_per_sample/8 > 4096
  */
  i2s_config_t i2s_config =
  {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),  // I2S receive mode with ADC
    .sample_rate = rate,                                                          // sample rate
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                                 // 16 bit I2S
    .channel_format = I2S_CHANNEL_FMT_ALL_LEFT,                                   // only the left channel
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),   // I2S format
    .intr_alloc_flags = 1,                                                        // none
    .dma_buf_count = 2,                                                           // number of DMA buffers
    .dma_buf_len = NUM_SAMPLES,                                                   // number of samples
    .use_apll = 0,                                                                // no Audio PLL
  };
  adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_11db);
  adc1_config_width(ADC_WIDTH_12Bit);
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL);
  SET_PERI_REG_MASK(SYSCON_SARADC_CTRL2_REG, SYSCON_SARADC_SAR1_INV);
  i2s_adc_enable(I2S_NUM_0);
}

void ADC_Sampling(uint16_t *i2s_buff){
  for (int i = 0; i < B_MULT; i++) {
    //TODO i2s_read_bytes is deprecated, replace with new function
    i2s_read_bytes(I2S_NUM_0, (char*)&i2s_buff[i * NUM_SAMPLES],  NUM_SAMPLES * sizeof(uint16_t), portMAX_DELAY);    
  }
}

void set_sample_rate(uint32_t rate) {
  i2s_driver_uninstall(I2S_NUM_0);
  configure_i2s(rate);
}
