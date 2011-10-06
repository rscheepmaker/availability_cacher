require 'mkmf'

HERE              = File.expand_path(File.dirname(__FILE__))
BUNDLE_MONGO      = Dir.glob("#{HERE}/mongo-c-driver*.tar.gz").first
BUNDLE_MONGO_PATH = BUNDLE_MONGO.gsub(".tar.gz", "")
BUNDLE_SCONS      = Dir.glob("#{HERE}/scons*.tar.gz").first
BUNDLE_SCONS_PATH = Dir.glob("#{HERE}/scons-2.1.0").first

def run_command( command )
  result = system( command )
  raise "'#{command}' failed" unless result
end

Dir.chdir(HERE) do
  if File.exists?("lib")
    puts "mongo-c-driver already built. run 'rake clean' if you need to rebuild."
  else
    run_command "tar xzf #{BUNDLE_SCONS}"
    run_command "python scons-2.1.0/setup.py install --prefix install"
    run_command "tar xzf #{BUNDLE_MONGO}"
    run_command "cp SConstruct #{BUNDLE_MONGO_PATH}"

    Dir.chdir(BUNDLE_MONGO_PATH) do
      cmd = "scons"
      run_command "../scons-2.1.0/install/bin/scons"
    end
  end

  $CFLAGS   = " -DMONGO_HAVE_STDINT -fPIC"
  $LDFLAGS  = " #{HERE}/mongo-c-driver/libbson.a #{HERE}/mongo-c-driver/libmongoc.a -fPIC"
end

if find_header('bson.h', "#{HERE}/mongo-c-driver/src") and
   find_header('mongo.h', "#{HERE}/mongo-c-driver/src") and
   create_makefile('availability_cacher/availability_cacher')
else
   p "No bson and mongo headers found"
end
