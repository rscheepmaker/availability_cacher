# -*- ruby -*-

require 'rubygems'
require 'hoe'
require 'rake/extensiontask'

Hoe.plugin :gemspec

Hoe.spec 'availability_cacher' do
        developer 'Rob Scheepmaker', 'rob@ikdoe.nl'
        self.readme_file  = 'README.txt'
        self.history_file = 'HISTORY.txt'
        self.extra_dev_deps << ['rake-compiler', '>= 0']
        self.spec_extras = { :extensions => ["ext/availability_cacher/extconf.rb"] }
        Rake::ExtensionTask.new('availability_cacher', spec) do |ext|
          ext.lib_dir = File.join('lib', 'availability_cacher')
        end
end

Rake::Task[:test].prerequisites << :compile

# vim: syntax=Ruby
