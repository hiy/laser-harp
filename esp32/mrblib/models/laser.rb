class Laser
  def initialize(gpio_pin, adc_pin, name)
    @pt = Phototransistor.new(gpio_pin, adc_pin)
    @name = name
  end

  # レーザーを遮られた時にtrueを返す。
  def play?
    value = @pt.value
    puts "#{@name}: #{value}"
    value < 3000
  end
end