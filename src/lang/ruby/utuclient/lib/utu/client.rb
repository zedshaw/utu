require 'socket'
require 'utu/stackish'
require 'tempfile'


module Utu

  # Standard Exception thrown by the client library.
  class ClientError < StandardError; end

  # Configuration for client connections.  You don't necessarily make these,
  # they are creatd by Utu::Peer and Utu::Mendicant so that it can keep the
  # configuration straight.
  #
  # Takes a configuration Hash of the format for *your* configuration:
  #
  #   {:name => "zed", :key => "".to_blob }
  #
  # And then this format for the peer configuration:
  #
  #   { 
  #     :address => "localhost", :port => "5000",
  #     :name => "utu", :key => "".to_blob 
  #   }
  #
  # This is then molded into the configuration required by the Peer and Mendicant.
  #
  # After it's configured then you can get various attributes, but if an attribute
  # is nil then it hasn't been set yet so you are probably not using it right.  For
  # example, you won't get your key if you gave a blank one until after you Peer.connect.
  #
  class Configuration
    attr_reader :my
    attr_reader :their
    attr_reader :original_config

    def initialize(my, their)
      @my = {:public => {} }
      @my[:private] = my
      @their = their
    end

    # Takes the current configuration, and spits out the structures
    # needed by the mendicant to make a connection.
    def to_mendicant
      [
        {:client=>[my[:private][:name], my[:private][:key]]}, 
        {:host=>[ @their[:address], @their[:port], @their[:name], @their[:key]]}
      ]
    end

    # Takes the config from the mendicant, and then breaks it up to be
    # easier to work with.  The original returned config is kept in the
    # original_config attribute.
    def from_mendicant(host_info, client_info)
      @original_config = {:their => host_info[:their], :my => client_info[:my]}

      host = host_info[:their]
      @their[:name] = host[Peer::NAME]
      @their[:key] = host[Peer::KEY][:public][0]
      @their[:key_fp] = host[Peer::KEY][:public][1]

      name, prv, pub = client_info[:my]

      # MY FUCKING GOD THIS HAS TO BE BETTER 
      @my[:name] = name
      @my[:public][:key] = pub[:public][0]
      @my[:public][:key_fp] = pub[:public][1]
      @my[:private][:key] = prv[:private][0]
      @my[:private][:key_fp] = prv[:private][1]
    end
  end

  # The best way to think of an Utu peer is as two block queues you push
  # messages on and pull messages off.  Each message then has a routing
  # setup that is currently outside the range of these docs.
  class Peer

    KEY=1
    NAME=0
    FINGER_PRINT=2

    # Messages are confirmed by the remote host via message counts.
    attr_reader :send_count
    # As with send_count, this determines proper sequencing.
    attr_reader :recv_count
    # The configuration used by this Peer (READ ONLY)
    attr_reader :config

    # This takes a configured but NOT connected Mendicant and uses it
    # to to it's thing.
    #
    # You should *never* send the private key, but store it for later when you
    # connect again.  For example, you might do this:
    #
    #   open("keyfile.prv") {|f| f.write(peer.config.my[:private][:key]) }
    #
    # But, remember this isn't encrypted so make sure the permissions are 
    # right or find a way to encrypt it.
    def initialize(mendicant)
      @mendi = mendicant
      @config = @mendi.config
    end

    def connect(type=:domain)
      @send_count = 0
      @recv_count = 0
      @mendi.connect(type)
    end


    def close
      @mendi.close
    end

    # This is the main method you'll use for sending Utu Messages to
    # a Hub or remote peer.  Utu Messages are built using stackish, 
    # but for the most part they have the same properties as s-expressions
    # and also XML.
    #
    # Here's a sample message:
    #
    #  body = {:msg=>[{"chat.speak"=>[ Utu::Blob("testing") ]}]}
    #
    def push(body)
      header = {:"header" => [ @send_count+=1 ]}
      @mendi.push_nodes header, body
    end

    # This is the inverse of Peer::push.  You simply call pull and when a
    # message is ready it is pulled off the wire, translated from a Utu::Node
    # to a Hash and given to you for processing.
    def pull
      header, body = @mendi.pull_nodes
      @recv_count += 1
      count = header[:header][0]

      raise ClientError.new("Remote Hub is out of sync (expected #{@recv_count} but got #{count} msg id.") if count != @recv_count

      return body.to_hash 
    end

    def sign(data, config=@mendi.config)
      name = config.my[:name]

      key=config.my[:private][:key]
      mendi = Mendicant.new(config)

      res = mendi.sign(name, key.to_blob, data)
      mendi.close
      return res
    end

    def validate(signed, name, config=@mendi.config)
      mendi = Mendicant.new(config)
      res = mendi.validate(name, signed)
      mendi.close
      return res
    end

    # Simplifies registering for messages by letting you just specify the 
    # root node of messages you want, and then the names of the child nodes (if any).
    # It will then register with the Hub your interest.  Keep in mind you don't have
    # to register to send, just if you want to receive them too.
    def register(root, children)
      push(make_route_msg(:register, root, children))
    end

    # Inverse of register.
    def unregister(root, children)
      push(make_route_msg(:unregister, root, children))
    end

    def members(root, children)
      push(make_route_msg(:members, root, children))
    end

    def children(root, children=[])
      push(make_route_msg(:children, root, children))
    end

    protected

    # Form should be either :register, :unregister, or :who
    def make_route_msg(form, root, children)
      msg = [{:msg => [
        {:route => [
          {form => [
            {root.to_sym => children.collect {|x| {x.to_sym => []} }
      }] }] }] }]
    end

  end

  class Mendicant
    # The domain socket file name being used.
    attr_reader :sock_name

    # The config it was given (don't touch!)
    attr_reader :config

    def initialize(config, mendicant_path="utumendicant")
      # TODO: Is this a race condition?
      temp = Tempfile.new("utumendicant")
      @sock_name = temp.path; temp.unlink
      @config = config
      @mendicant_path = mendicant_path
      @mendicant_opts = ""
      @err_log = "/tmp/utumendicanterr-#{Process.pid}.log"
    end

    def run_mendicant(type)
      system "#@mendicant_path #@mendicant_opts #{type} #@sock_name -e #@err_log &"
    end

    def connect_domain
      @serv = UNIXServer.new(@sock_name)
      run_mendicant("-d")

      @read_sock = @serv.accept
      @write_sock = @read_sock.dup

      # don't need the domain socket anymore
      @serv.close
      File.unlink(@sock_name)
    end

    def configure_mendicant
      write_frame(@config.to_mendicant.to_stackish)
      host_info, client_info = pull_nodes
      @config.from_mendicant(host_info, client_info)
    end

    def connect_fifo
      write_side = "#{@sock_name}.in"
      read_side = "#{@sock_name}.out"
      run_mendicant("-f")

      10.times do
        if File.exist? write_side
          @write_sock = open(write_side, "w")
          break
        end

        sleep 0.3
      end

      raise "Mendicant failed to create #{write_side} FIFO file." if !@write_sock
      @read_sock = open(read_side, "r")
     
      # not needed anymore now that we're all connected
      File.unlink write_side
      File.unlink read_side
    end

    def connect(type=:domain)
      self.send("connect_#{type}")
      configure_mendicant
    end

    def sign(name, key, data, type=:domain)
      @mendicant_opts = "-s"
      self.send("connect_#{type}")
      write_frame([{:sign => [name, key.to_blob]}, data].to_stackish)
      hdr, results = pull_nodes
    end

    def validate(name, data, type=:domain)
      @mendicant_opts = "-s"
      self.send("connect_#{type}")
      write_frame([{:validate => [name]}, data].to_stackish)
      hdr, results = pull_nodes
    end

    # Stops the connection to the mendicant and removes the domain socket being
    # used for communication.  The mendicant should then go away on its own
    # after that.
    def close
      [@read_sock, @write_sock].each do |sock|
        if sock and !sock.closed?
          sock.close
        else
          raise ClientError.new("Mendicant not open.")
        end
      end
    end

    # Reads a Utu frame from the peer.  An Utu Frame is simply a network
    # byte ordered unsigned short to indicate the size of the following
    # frame and then the data of that lenght.
    def read_frame
      spec = @read_sock.read(2)
      if spec
        len = spec.unpack('n')[0]
        return @read_sock.read(len)
      else
        close
        raise Utu::ClientError.new("Read failure from read_socket.");
      end
    end

    # Writes an Utu Frame to the socket and thus the remote peer.
    # As mentioned in read_frame a Frame is simply a network byte ordered
    # unsigned short for the size, and then that much data.
    def write_frame(payload)
      len = [payload.length].pack('n')
      @write_sock.write(len + payload)
      @write_sock.flush
    end

    # Raw interface for pushing two Utu::Node structures raw 
    # onto the wire as a frame as a header and then body.
    # All Utu Messages are composed of a Stackish Utu::Node for the
    # header and then another one for the body both in canonical form.
    #
    # The header is not encrypted but it is authenticated against tampering
    # as AAD data.  This means there shouldn't be anything in the header
    # except for sequencing and sizing data (which is needed to
    # route messages without decrypting them).
    #
    # You probably want to use Peer::push and Peer::pull instead.
    def push_nodes(*nodes)
      write_frame(nodes.collect {|x| x.to_stackish }.join(""))
    end

    # The inverse of Mendicant::push_nodes, this pulls two raw Utu::Node
    # structures from the remote peer.  This function will pull any
    # err messages it receives from the mendicant and put them into a
    # ClientError exception.  The caller should deal with the message and
    # determine whether to continue or not.
    def pull_nodes
      nodes = read_frame.from_stackish

      if err = nodes.first[:err]
        raise ClientError.new(err.join(": "))
      end

      return nodes
    end
  end
end
