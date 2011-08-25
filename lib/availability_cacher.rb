require 'availability_cacher/availability_cacher'

class AvailabilityCacher
  VERSION = '1.0.0'

  def self.connect( options = {} )
    defaults = {
      host: "127.0.0.1",
      port: 27017,
      username: "",
      password: "",
      database: "test"
    }
    result = defaults.merge( options )

    @@host     = result[:host]
    @@port     = result[:port]
    @@username = result[:username]
    @@password = result[:password]
    @@database = result[:database]
  end
end
