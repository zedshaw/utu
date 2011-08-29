#!/usr/bin/env ruby
require 'tk'
require 'utu/client'
require 'thread'
require 'pstore'
require 'fileutils'

HOME_PRIVATE_KEYFILE=".utu/private.key"
HOME_PUBLIC_KEYFILE=".utu/public.key"

class UtuTkClient

  def initialize(host, port, name, room, key_file)
    @host, @port, @name, @room = host, port, name, "room:#{room}"
    @key = load_key_file(key_file)

    @config = {
      :my => { 
        :name => @name, 
        :key => @key ? @key.to_blob : "".to_blob
      },
      :their => {
        :address => @host, 
        :port => @port,
        :name => "utu", 
        :key => "".to_blob,
      }
    }

    @peer = Utu::Peer.new @config[:my], @config[:their]
    start_ui
  end

  def load_key_file(key_file)
    if File.exist? key_file
      return open(key_file) {|f| f.read }
    else
      FileUtils.mkdir_p(File.dirname(key_file))
      return ""
    end
  end


  def start_ui
    @root = TkRoot.new { title "Utu #@host #@port AS #@name" }

    @text = TkText.new(@root) {
      height 30
      wrap "word"
      font "Arial 9"
    }
    @text.pack('fill' => 'both', 'expand' => 1)

    @entry = TkEntry.new(@root)
    @entry.pack('side' => 'bottom', 'fill' => 'x')
    @entry.bind("Return") { send_msg(@peer, @room, :say, @entry.value.strip); @entry.value = "" }
    @entry.focus

    @root.bind("Escape") { shutdown }

    start_read_thread

    Tk.mainloop
  end

  def start_read_thread
    connect_peer

    @ping_thread = Thread.new do
      loop do
        sleep 20
        @peer.push({:msg => [{:system => [{:ping => [Time.now.to_i]}]}]})
      end
    end

    @read_thread = Thread.new do 
      begin
        loop do
          body = @peer.pull
          break if !body

          if body[:rpy] or body[:err]
            # this is a service reply, so probably a ping for now
            STDERR.puts "Service reply: #{body.to_stackish}"
          else
            room = body[:msg][0][:chat][0]
            room_name = room.keys[0]
            room_data = room[room_name]
            text = room_data[0][:text][0]
            name = room_data[1][:from]
            type = room_data[2][:type]

            msg = "[#{room_name}] [#{name}] "

            if type.to_s != "say"
              msg << "(#{type}) "
            end

            append_text(msg + text)
          end
        end
      rescue IOError
        # ignore
      rescue
        puts "Failed parsing message: #{$!}"
        exit 1
      end
    end
  end

  def server_name
    @peer.config.their[:name]
  end

  def server_key_fp
    @peer.config.their[:key_fp]
  end

  def save_client_key
    if not File.exist? HOME_PRIVATE_KEYFILE
      append_text("Saved your private key to #{HOME_PRIVATE_KEYFILE}.")
      open(HOME_PRIVATE_KEYFILE, "w") {|f| f.write @peer.config.my[:private][:key] }

      append_text("Saved your public key to #{HOME_PUBLIC_KEYFILE}.")
      open(HOME_PUBLIC_KEYFILE, "w") {|f| f.write @peer.config.my[:public][:key] }
    end
  end

  def connect_peer
    append_text("Connecting to #{@config[:their][:host]}:#{@config[:their][:port]} as #{@config[:their][:name]}")

    @peer.connect
    save_client_key

    append_text("Connected, registering with server...")
    append_text("Server's name is #{server_name.inspect} public key fingerprint is #{server_key_fp}.")

    @peer.register("chat", [@room])

    send_msg(@peer, @room, :join, "Hello everyone.")
    # TODO: store keys for later comparison and show the fingerprint if failure
    append_text("Ready to chat.  Just say something. ESC quits.")
  end

  def shutdown
    send_msg(@peer, @room, :leave, "Good night!")
    @peer.unregister("chat", [@room])
    @peer.close 
    append_text("Goodbye!")
    sleep 1
    exit 0 
  end

  def send_msg(peer, room, type, *values)
    values[0].to_blob
       msg = {:chat => [
                {room.to_sym => [ 
                  {:text => values},
                  {:from => [@peer.config.my[:name]]},
                  {:type => [type.to_s]},
                ]}
            ]}
      @peer.push({:msg => [msg]})
  end

  def append_text(msg)
    ts = Time.now
    @text.insert('end', "#{ts.hour}:#{ts.min}  " + msg + "\n")
    @text.yview('end')
  end
end

if ARGV.length != 4
  puts "usage: ruby tk_client.rb host port name room"
  exit 1
end

host, port, name, room = ARGV.collect {|x| x.clone }

client = UtuTkClient.new(host, port, name, room, HOME_PRIVATE_KEYFILE)
