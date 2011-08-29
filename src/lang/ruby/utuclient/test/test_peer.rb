# Copyright (C) 2007 Zed A. Shaw

require 'test/unit'
require 'utu/client'
require 'pp'

include Utu

class UtuPeerTest < Test::Unit::TestCase

  def setup
   @config = Configuration.new({ 
        :name => "zed", 
        :key => "".to_blob,
      }, {
        :address => "localhost", 
        :port => "5000",
        :name => "utu", 
        :key => "".to_blob,
      })
    @path =  "../../../../src/utumendicant"
    @mendicant = Mendicant.new(@config, @path)

    @peer = Peer.new(@mendicant)
    @peer.connect(:fifo)
  end

  def connect(type)
    peer = Peer.new(@mendicant)
    peer.connect(type)
    return peer
  end

  def teardown
    @peer.close
  end

  def test_send_recv
    texts = ["test", "that", "howdy\0there", "a" * 136]
    @peer.register("chat.speak", [])

    10.times do 
      texts.each do |text|
        body = {"msg"=>[{"chat.speak"=>[ text.to_blob ]}]}

        @peer.push(body)
        body = @peer.pull

        assert_equal body/:msg/:"chat.speak"/0, text
      end
    end
  end

  def test_send_recv_fifo
    @peer.close
    @peer = connect(:fifo)

    test_send_recv
  end

  def test_sign_validate
    hdr, results = @peer.sign(["hi"])
    hdr2, validate = @peer.validate(results, "zed")

    assert_equal hdr/:sign/0, hdr2/:validate/0
    # hdr has the private, we MUST get a public back
    assert_not_equal hdr/:sign, hdr2/:validate
    # this key should match what's in the results from test_signature
    assert_equal results/:signed/:identity/0, hdr2/:validate/1
    assert_equal results/:signed/:identity/1, hdr2/:validate/0

    # make sure it also will bomb
    assert_raises Utu::ClientError do
      h, v = @peer.validate(results, "badname")
    end
  end
end
