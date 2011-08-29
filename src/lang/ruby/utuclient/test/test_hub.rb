require 'test/unit'
require 'utu/client'

include Utu

class UtuHubTest < Test::Unit::TestCase

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

  def teardown
    @peer.close
  end

  def test_route_register
    @peer.register("chat", ["room:help"])
  end

  def test_route_unregister
    test_route_register
    @peer.unregister("chat", ["room:help"])
  end

  def test_route_children
    test_route_register
    @peer.children("chat")
    msg = @peer.pull
    assert msg/:rpy
    assert_equal msg/:rpy/:children/:@path, ["[ chat \n"]
    assert_equal msg/:rpy/:children/1, "room:help"
    test_route_unregister
  end

  def test_route_members
    test_route_register
    @peer.members("chat", ["room:help"])
    msg = @peer.pull
    assert msg/:rpy
    assert_equal msg/:rpy/:members/:@path, ["[ [ room:help chat \n"]
    assert_equal msg/:rpy/:members/1, "zed"
    test_route_unregister
  end

  def test_member_mendicate
  end

  def test_member_invite
  end

  def test_member_register
  end

  def test_member_send
  end

  def test_system_ping
    @peer.push({:msg => [{:system => [{:ping => [Time.now.to_i]}]}]})
    msg = @peer.pull
    assert msg/:rpy/:pong/0 >=0
  end

  def test_info_tag
    @peer.push({:msg => [{:info => [{:tag => [{:name => ["Rakefile"]}]}]}]})
    msg = @peer.pull
    data = parse_info_msg(msg, "Rakefile", :tag)
  end

  def test_info_get
    @peer.push({:msg => [{:info => [{:get => [{:name => ["Rakefile"]}]}]}]})
    msg = @peer.pull
    data = parse_info_msg(msg, "Rakefile", :get)
    assert data.is_blob?
  end

  def test_info_list
    @peer.push({:msg => [{:info => [{:list => [{:name => ["lib"]}]}]}]})
    msg = @peer.pull
    dir = parse_info_msg(msg, "lib", :list)
    assert dir.is_blob?

    files = dir.split("\n")
    assert files.length > 0
    assert_equal files[0], "."
    assert_equal files[1], ".."
  end


  private

  def parse_info_msg(msg, named, type)
    assert msg/:rpy
    name, data = msg/:rpy/:info
    assert_equal name/:name/0, named
    data = data[type][0]
    assert data.length > 0
    data
  end
end
