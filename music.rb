# レーザー1が遮られたら音を鳴らす
live_loop :laser_1 do
  use_real_time
  sync "/osc/trigger/laser_1"
  play 60 # C
end

# レーザー2が遮られたら音を鳴らす
live_loop :laser_2 do
  use_real_time
  sync "/osc/trigger/laser_2"
  play 64 # E
end