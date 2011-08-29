require 'utu/client'
require 'thread'

if ARGV.length != 4
  STDERR.puts "usage: ruby tap.rb host port name keyfile"
  exit 1
end

private_key = open(ARGV[3]) {|f| f.read }

config = {
  :my => { 
    :name => ARGV[2], 
    :key => private_key.to_blob,
  },
  :their => {
    :address => ARGV[0], 
    :port => ARGV[1],
    :name => "utu", 
    :key => "".to_blob
  }
}

peer = Utu::Peer.new config[:my], config[:their]
peer.connect

Thread.new {
  begin
    loop do
      body = peer.pull
      puts body.to_stackish
    end
  rescue Object
    STDERR.puts $!
    STDERR.puts $!.backtrace.join("\n")
    exit 1
  end
}

while stackish = STDIN.readline
  if not node = stackish.from_stackish
    STDERR.puts "Failed to parse stackish input."
    next
  end

  peer.push(node)
end

peer.close
