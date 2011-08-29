require 'rexml/document'
include REXML

err = open("build_logs/err.log").readlines
out = open("build_logs/out.log").readlines
valg = open("build_logs/valgrind.log").readlines

efw = err.grep /(ERROR|FAIL|WARN)/
unit = out.grep /^tests\/.*.c\([0-9]*\):/
verr = valg.grep /ERROR SUMMARY/
vmem = valg.grep /malloc\/free/

# bust out the valgrind stats
fields = verr[0].split
verrors = fields[3].delete(",").to_i
vctxs = fields[6].delete(",").to_i

fields = vmem[0].split
vmemuse = fields[6].delete(",").to_i
vmemblks = fields[9].delete(",").to_i

cccc = Document.new(open(".cccc/cccc.xml").read)
sloc = cccc.elements["/CCCC_Project//lines_of_code/@value"].value.to_i
diff = `darcs whatsnew | wc -l`.to_i
coverage = `ruby build_logs/gcov_report.rb | tail -1`.to_i

log = "#{sloc} #{diff} #{efw.length} #{unit.length} #{verrors} #{vctxs} #{vmemuse} #{vmemblks} #{coverage}"

open("build_logs/qualstats.log", 'a+') {|out| out.puts log }

if efw.length > 0
  puts efw.join
end

# display my status
`fish -c "dm DONE: #{log}"`

puts unit.join if unit.length > 0
print "#" * efw.length
puts "*" * unit.length
puts log

