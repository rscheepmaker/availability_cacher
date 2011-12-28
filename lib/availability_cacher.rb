require 'availability_cacher/availability_cacher'

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
    return false if options[:rentable_id].blank?
    return false if options[:no_stay].nil?
    return false if options[:no_arrive].nil?
    return false if options[:no_checkout].nil?
    options[:no_stay].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    options[:no_arrive].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    options[:no_checkout].map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    dates = (from..till).to_a
    dates.map! { |d| Time.utc( d.year, d.month, d.mday ).localtime }
    create_cache_from_normalized_dates( options[:rentable_id], options[:category_id], options[:no_stay], options[:no_arrive], options[:no_checkout], dates )
  end
end
