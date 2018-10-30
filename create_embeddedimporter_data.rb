
require("zlib")
require("base64")

def create_certifi_cacert_data
  cacert_data = File.read("Lib/certifi/cacert.pem")
  File.open("Lib/certifi/cacert.py", "w") do |file|
    file.puts '# This file was generated automatically.'
    file.write '_ca_cert_data = r"""'
    file.write cacert_data
    file.puts '"""'
  end
end

def create_werkzeug_shared_data
  File.open("Lib/werkzeug/debug/_shared.py", "w") do |file|
    file.puts '# This file was generated automatically.'
    file.puts 'shared_file = {'

    Dir.glob("Lib/werkzeug/debug/shared/*.*").to_a.sort.each do |filename|
      next if File.basename(filename) == "ubuntu.ttf"

      file.puts("    \"#{File.basename(filename)}\": " + '"""' + Base64.encode64(File.binread(filename)) + '""",')
    end
    file.puts "}"
  end
end

def get_file_data
  list = []

  Dir.glob("**/*.py").to_a.sort.each do |filename|
    next if filename.start_with?("test/") || filename.include?("/test/")
    next if filename.start_with?("__pycache__/") || filename.include?("/__pycache__/")

    list.push({
      filename: filename,
      data: File.open(filename, "r", &:read).b + "\n\0".b
    })
  end

  return list
end

def output_list(list)
  File.open("Modules/embeddedimport_data.c", "w") do |file|
    file.puts '#include <stddef.h>'

    file.puts

    file.puts "char embeddedimporter_filename[] = {"
    list.each do |item|
      file.puts "  // " + item[:filename]
      item[:filename].each_byte.each_slice(16) do |bytes|
        file.puts("  " + bytes.map{ |i| "0x" + i.to_s(16).rjust(2, '0') }.join(",") + ",")
      end
      file.puts("  0x00,")
    end
    file.puts("  0x00")
    file.puts("};")

    file.puts

    data = list.map{ |i| i[:data] }.reduce(:+)

    file.puts "const size_t embeddedimporter_raw_data_size = " + data.bytesize.to_s + ";"

    file.puts

    compressed = Zlib.deflate(data, Zlib::BEST_COMPRESSION)
    file.puts "const unsigned char embeddedimporter_raw_data_compressed[] = {"
    compressed.each_byte.each_slice(16) do |bytes|
      file.puts("  " + bytes.map{ |i| "0x" + i.to_s(16).rjust(2, '0') }.join(",") + ",")
    end
    file.puts "};"

    file.puts

    file.puts "const size_t embeddedimporter_raw_data_compressed_size = " + compressed.bytesize.to_s + ";"

    file.puts

    offset = 0
    file.puts "const size_t embeddedimporter_data_offset[] = {"
    list.each do |item|
      file.puts "  #{offset},  // #{item[:filename]}"
      offset += item[:data].bytesize
    end
    file.puts "};"
  end
end

Dir.chdir(__dir__) do
  create_certifi_cacert_data
  create_werkzeug_shared_data

  list = nil
  Dir.chdir("Lib") do
    list = get_file_data
  end

  output_list(list)
end

