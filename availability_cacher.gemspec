# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{availability_cacher}
  s.version = "1.0.0.20111006105036"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = [%q{Rob Scheepmaker}]
  s.date = %q{2011-10-06}
  s.description = %q{Allows quick generating of records in mongodb.}
  s.email = [%q{rob@ikdoe.nl}]
  s.executables = [%q{availability_cacher}]
  s.extensions = [%q{ext/availability_cacher/extconf.rb}]
  s.extra_rdoc_files = [%q{History.txt}, %q{Manifest.txt}, %q{README.txt}]
  s.files = [%q{History.txt}, %q{Manifest.txt}, %q{README.txt}, %q{Rakefile}, %q{bin/availability_cacher}, %q{lib/availability_cacher.rb}, %q{ext/availability_cacher/availability_cacher.c}, %q{ext/availability_cacher/extconf.rb}, %q{ext/availability_cacher/mongo-c-driver.tar.gz}, %q{test/test_availability_cacher.rb}, %q{.gemtest}]
  s.homepage = %q{http://www.ikdoe.nl}
  s.rdoc_options = [%q{--main}, %q{README.txt}]
  s.require_paths = [%q{lib}]
  s.rubyforge_project = %q{availability_cacher}
  s.rubygems_version = %q{1.8.8}
  s.summary = %q{Allows quick generating of records in mongodb.}
  s.test_files = [%q{test/test_availability_cacher.rb}]

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<rake-compiler>, [">= 0"])
      s.add_development_dependency(%q<hoe>, ["~> 2.12"])
    else
      s.add_dependency(%q<rake-compiler>, [">= 0"])
      s.add_dependency(%q<hoe>, ["~> 2.12"])
    end
  else
    s.add_dependency(%q<rake-compiler>, [">= 0"])
    s.add_dependency(%q<hoe>, ["~> 2.12"])
  end
end
