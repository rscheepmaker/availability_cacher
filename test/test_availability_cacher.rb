require 'test/unit'
require 'availability_cacher'
require 'date'

class TestAvailabilityCacher < Test::Unit::TestCase
  def test_create_cache
    AvailabilityCacher.connect
    assert AvailabilityCacher.create_cache( 1, [Date.civil(2011, 9, 2)], [Date.civil(2011, 9, 4)], [Date.civil(2011, 9, 3)], [ Date.civil(2011, 9, 1), Date.civil(2011, 9, 2), Date.civil(2011, 9, 3), Date.civil(2011, 9, 4), Date.civil(2011, 9, 5) ] )
  end
end
