require 'osc-ruby'

message = OSC::Message.new('/trigger/laser_1')
p message.encode
# エンコードされたメッセージ
# => "/trigger/laser_1\x00\x00\x00\x00,\x00\x00\x00"

message = OSC::Message.new('/trigger/laser_2')
p message.encode