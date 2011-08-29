
def extract_numbers(line)
  n = line.match(/:([0-9]?[0-9]?[0-9]?\.[0-9][0-9])% of ([0-9]+)$/)[1,2]

  return n[0].to_f, n[1].to_f
end

def run_gcov(dir)
  out = `gcov -b -a -c -p -u -o #{File.dirname(dir)}/CMakeFiles/utu.dir/#{File.basename(dir)} -n #{dir}/*.c 2> /dev/null`
  return out.split("\n")
end

def parse_gcov(stats, lines)
  file = nil

  while l = lines.shift
    case l
    when /^File/
      file = l.match(/'(.*)'$/)[1]
      stats[file] = {}
    when /^Lines/
      stats[file][:lines] = extract_numbers(l)
    when /^No branches/
      stats[file][:branches] = [0.0,0]
    when /^No calls/
      stats[file][:calls] = [0.0,0]
    when /^Branches/
      stats[file][:branches] = extract_numbers(l)
    when /^Taken/
      stats[file][:taken] = extract_numbers(l)
    when ""
      file = nil
    else
      puts "ERR: #{l}"
    end
  end
end

def print_report(stats)
  source = stats.keys.grep(/.(rl|c)$/)

  mean = {:lines => 0.0, :branches => 0.0, :taken => 0.0, :calls => 0.0}

  puts "#{' ' * 26}File\tLi\tBr\tTa\tCa"
  source.sort.each do |s|
    i = stats[s]
    i.each {|k,v| mean[k] += v[0] }

    print "%30s\t%0.2f\t%0.2f\t%0.2f\t%0.2f\n" % 
    [s, i[:lines][0], i[:branches][0], i[:taken] ? i[:taken][0] : 0.0, i[:calls][0]]
  end

  mean
end

def print_mean(mean, length)
  grand = 0.0
  mean.delete :calls

  print "\nmean "
  mean.each do |k,v| 
    print "#{k}=#{"%0.2f" % (v/length)} "
    grand += v/length
  end
  puts "\n#{grand/mean.length}"
end

if $0 == __FILE__
  stats = {}

  Dir["src/*"].each do |dir| 
    parse_gcov(stats, run_gcov(dir)) if File.directory?(dir)
  end

  mean = print_report(stats)
  print_mean(mean, stats.length)
end
