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

  def self.create_cache( from, till, options = {} )
    return false if options[:rentable_id].blank?
    return false if options[:no_stay].nil?
    return false if options[:no_arrive].nil?
    return false if options[:no_checkout].nil?
    options[:no_stay].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    options[:no_arrive].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    options[:no_checkout].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    dates = (from..till).to_a
    dates.map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    p "DATES:"
    p options[:no_stay]
    p options[:no_arrive]
    p options[:no_checkout]
    p options[:dates]
    create_cache_from_normalized_dates( options[:rentable_id], options[:no_stay], options[:no_arrive], options[:no_checkout], dates )
  end
end
