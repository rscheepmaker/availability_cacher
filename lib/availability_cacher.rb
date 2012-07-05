require 'availability_cacher/availability_cacher'
require 'yaml'

class AvailabilityCacher
  VERSION = '1.1.0'

  def self.cacher
    @@cacher ||= AvailabilityCacher.new
  end

  def initialize( ) 
    options = YAML::load( File.open( File.join( Rails.root, 'config', 'mongoid_cacher.yml' ) ) )
    defaults = {
      'host'     => "127.0.0.1",
      'port'     => 27017,
      'username' => "",
      'password' => "",
      'database' => "test"
    }
    result = defaults.merge( options[Rails.env.to_s] )

    uri = ENV['MONGOLAB_URI']
    unless uri.blank?
      uri                = URI.parse(uri)
      result['host']     = IPSocket.getaddress(uri.host)
      result['port']     = uri.port
      result['username'] = uri.user
      result['password'] = uri.password
      result['database'] = uri.path.gsub('/', '')
    end

    mongo_connect( result['host'], result['port'], result['username'], result['password'], result['database'] );

    @database = result['database']
    @host     = result['host']
    @port     = result['port']
    @username = result['username']
    @password = result['password']
  end

  def create_cache( from, till, options = {} )
    return false if options[:rentable_id].nil? or options[:rentable_id] == 0
    return false if options[:no_stay].nil?
    return false if options[:no_arrive].nil?
    return false if options[:no_checkout].nil?
    options[:no_stay].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    options[:no_arrive].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    options[:no_checkout].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }

    arrival_checkout_hash = {}
    options[:arrival_checkout_hash].each_pair do |key, value|
	    arrival_checkout_hash[Time.utc(key.year, key.month, key.mday).localtime] = value.map{|v| [Time.utc(v.first.year, v.first.month, v.first.mday).localtime, v.last]}
    end

    dates = (from..(till + 9)).to_a
    dates.map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    create_cache_from_normalized_dates( options[:rentable_id], options[:category_id], options[:no_stay], options[:no_arrive], options[:no_checkout], dates, arrival_checkout_hash )
  end
end
