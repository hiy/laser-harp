Wifi.setup
laser_1 = Laser.new(32, 34, 'laser_1')
laser_2 = Laser.new(33, 35, 'laser_2')
osc_client = OSCClient.new

loop do
  if laser_1.play?
    osc_client.send("/trigger/laser_1\x00\x00\x00\x00,\x00\x00\x00")
  end

  if laser_2.play?
    osc_client.send("/trigger/laser_2\x00\x00\x00\x00,\x00\x00\x00")
  end

  sleep 0.001
end
