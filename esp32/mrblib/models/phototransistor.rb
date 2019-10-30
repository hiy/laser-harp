class Phototransistor
  def initialize(gpio_pin, adc_pin)
    gpio_init_output(gpio_pin)
    gpio_set_level(gpio_pin, 1)
    init_adc(adc_pin)
    @adc_pin = adc_pin
  end

  def value
    read_adc(@adc_pin)
  end
end
