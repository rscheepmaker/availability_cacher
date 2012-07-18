require 'test/unit'
require 'availability_cacher'
require 'date'

class TestAvailabilityCacher < Test::Unit::TestCase
  def test_create_cache
    cacher = AvailabilityCacher.new
    assert cacher.create_cache( Date.civil(2012, 7, 1), Date.civil(2012, 8, 20), 
					    no_stay: [ Date.civil(2012, 7, 3), Date.civil(2012,7,5) ], 
					    no_arrive: [ Date.civil( 2012, 7, 8), Date.civil(2012, 7, 9) ], 
					    no_checkout: [ Date.civil(2012, 7, 7), Date.civil(2012,7,13) ], 
					    rentable_id: 1,
					    category_id: 1,
					    tags: %w( dogs cats dinner ),
					    arrival_checkout_hash:
			{ Date.civil(2012,7,1) => [ [Date.civil(2012,7,5), ''], [Date.civil(2012,7,6), ''] ], 
			  Date.civil(2012,7,4) => [ [Date.civil(2012,7,5), ''], [Date.civil(2012,7,7), ''], [Date.civil(2012,7,8), ''], [Date.civil(2012,7,9) ] ], 
			  Date.civil(2012,7,6) => [ [Date.civil(2012,7,9), 'weekend'], [Date.civil(2012,7,13), 'week'], [Date.civil(2012,7,7), ''] ], 
			  Date.civil(2012,7,7) => [ [Date.civil(2012,7,9), ''], [Date.civil(2012,7,10), ''] ],
			  Date.civil(2012,7,8) => [ [Date.civil(2012,7,10), ''] ],
			  Date.civil(2012,7,9) => [ [Date.civil(2012,7,11), ''] ],
   			  Date.civil(2012,7,13) => [ [Date.civil(2012,7,14), ''], [Date.civil(2012,7,15), 'awesome' ], [Date.civil(2012,8,15), ''] ] } )
  end
end
