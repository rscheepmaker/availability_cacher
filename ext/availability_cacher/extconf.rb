require 'mkmf'

HERE        = File.expand_path(File.dirname(__FILE__))
BUNDLE      = Dir.glob("#{HERE}/mongo-c-driver*.tar.gz").first
BUNDLE_PATH = BUNDLE.gsub(".tar.gz", "")

Dir.chdir(HERE) do
  if File.exists?("lib")
    puts "mongo-c-driver already built. run 'rake clean' if you need to rebuild."
  else
    cmd = "tar xzf #{BUNDLE} 2>&1"
    result = %x(tar xzf #{BUNDLE})
    puts result
    raise "'#{cmd}' failed" unless result

    puts BUNDLE_PATH

    Dir.chdir(BUNDLE_PATH) do
      cmd = "scons"
      result = %x(scons)
      puts result
      raise "'#{cmd}' failed" unless result
    end
  end

  $CFLAGS   = " -DMONGO_HAVE_STDINT"
  $LDFLAGS  = " -static #{HERE}/mongo-c-driver/libbson.a #{HERE}/mongo-c-driver/libmongoc.a"
end

if find_header('bson.h', "#{HERE}/mongo-c-driver/src") and
   find_header('mongo.h', "#{HERE}/mongo-c-driver/src") and
   create_makefile('availability_cacher/availability_cacher')
else
   p "No bson and mongo headers found"
end
