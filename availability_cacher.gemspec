# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{availability_cacher}
  s.version = "1.1.18"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Rob Scheepmaker"]
  s.date = %q{2012-12-18}
  s.default_executable = %q{availability_cacher}
  s.description = %q{Allows quick generating of records in mongodb.}
  s.email = ["rob@ikdoe.nl"]
  s.executables = ["availability_cacher"]
  s.extensions = ["ext/availability_cacher/extconf.rb"]
  s.extra_rdoc_files = ["History.txt", "Manifest.txt", "README.txt"]
  s.files = ["History.txt", "Manifest.txt", "README.txt", "Rakefile", "bin/availability_cacher", "lib/availability_cacher.rb", "ext/availability_cacher/availability_cacher.c", "ext/availability_cacher/extconf.rb", "ext/availability_cacher/mongo-c-driver.tar.gz", "test/test_availability_cacher.rb"]
  s.homepage = %q{http://www.ikdoe.nl}
  s.rdoc_options = ["--main", "README.txt"]
  s.require_paths = ["lib"]
  s.rubyforge_project = %q{availability_cacher}
  s.rubygems_version = %q{1.3.7}
  s.summary = %q{Allows quick generating of records in mongodb.}
  s.test_files = ["test/test_availability_cacher.rb"]

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<rubyforge>, [">= 2.0.4"])
      s.add_development_dependency(%q<rake-compiler>, [">= 0"])
      s.add_development_dependency(%q<hoe>, [">= 2.6.1"])
    else
      s.add_dependency(%q<rubyforge>, [">= 2.0.4"])
      s.add_dependency(%q<rake-compiler>, [">= 0"])
      s.add_dependency(%q<hoe>, [">= 2.6.1"])
    end
  else
    s.add_dependency(%q<rubyforge>, [">= 2.0.4"])
    s.add_dependency(%q<rake-compiler>, [">= 0"])
    s.add_dependency(%q<hoe>, [">= 2.6.1"])
  end
end
