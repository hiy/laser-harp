class OSCClient
  def initialize
    # noop
  end

  def send(message)
    send_osc_message(message, message.size)
  end
end