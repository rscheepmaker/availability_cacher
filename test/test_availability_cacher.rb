require 'test/unit'
require 'availability_cacher/availability_cacher'

class TestAvailabilityCacher < Test::Unit::TestCase
  def test_create_cache
    assert AvailabilityCacher.create_cache( 1, [Date.civil(2011, 9, 2)], [Date.civil(2011, 9, 4)], [Date.civil(2011, 9, 3)], [ Date.civil(2011, 9, 1), Date.civil(2011, 9, 2), Date.civil(2011, 9, 3), Date.civil(2011, 9, 4), Date.civil(2011, 9, 5) ] )
  end
end
