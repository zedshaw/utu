require 'utu/client'
require 'thread'
require 'pp'

if ARGV.length != 3
  STDERR.puts "usage: ruby simple_chat.rb host port name"
  exit 1
end

config = {
    :my => { 
    :name => ARGV[2], 
    :key => "".to_blob,
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

def start_reader_thread(peer)
  return Thread.new do
    begin
      loop do
        # we don't care about circuit or more
        body = peer.pull
        break if !body
        if body[:rpy]
          # this was a reply to some request, ignore it
          STDERR.puts "reader thread intercepted: #{body.inspect}"
          next
        end

        room = body[:msg][0][:chat][0]
        room_name = room.keys[0]
        room_data = room[room_name]
        text = room_data[0][:text][0]
        name = room_data[1][:from]
        type = room_data[2][:type]

        print "[#{room_name}] [#{name}] "

        if type.to_s != "say"
          print "(#{type}) "
        end

        puts text
      end
    rescue IOError
      # ignore
    rescue
      STDERR.puts "Failed parsing message: #{$!}"
      STDERR.puts $!.backtrace.join("\n")
      exit 1
    end
  end
end

def send_msg(peer, room, type, *values)
     msg = {:chat => [
              {room.to_sym => [ 
                {:text => values},
                {:from => [peer.config.my[:name]]},
                {:type => [type.to_s]},
              ]}
          ]}
    peer.push({:msg => [msg]})
end

peer.children("chat")
rpy = peer.pull
rooms = rpy[:rpy][0][:children].collect {|r| 
  s = r.split(":")[1] rescue r
  s ? s : "ROGUE ROOM: #{r}"
}
STDERR.puts "Available rooms:\n#{rooms.join("\n")}"

STDERR.print "What room do you want? "
room = "room:#{STDIN.readline.strip}"

begin
  puts "Joining #{room}"
  peer.register("chat", [room])
  peer.members("chat", [room])
  users = peer.pull
  puts "Users in #{room}:\n#{users[:rpy][0][:members].join("\n")}"
  puts "Type something:"

  start_reader_thread(peer)
  send_msg(peer, room, :join, "Hello everyone.".to_blob)

  loop do
    line = STDIN.readline
    break if !line
    send_msg(peer, room, :say, line.strip.to_blob)
  end
rescue EOFError
  STDERR.puts "GOODBYE!"
  send_msg(peer, room, :leave, "Good night!".to_blob)
  peer.unregister("chat", [room])
  peer.close
end


