# 
# Utu -- Saving The Internet With Hate
# 
# Copyright (c) Zed A. Shaw 2005 (zedshaw@zedshaw.com)
# 
# This file is modifiable/redistributable under the terms of the GNU
# General Public License.
# 
# You should have recieved a copy of the GNU General Public License along
# with this program; see the file COPYING. If not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 0211-1307, USA.
# 

require 'test/unit'
require 'utu/stackish'

include Utu

class UtuNodeTest < Test::Unit::TestCase

  def setup
   @cases = {
       "[ [ '9:test this' good [ 1234 @an:integer 345.78 @a-float \"like yeah\" @a_string test [ [ 1 2 3 4 5 ] [ 1.1 2.2 3.3 4.4 5.5 ] [ \"test\" strings nesting doc \n"=>
      [{:doc=>
         [{:good=>["test this".to_blob]},
          {:test=>
            [{:"@an:integer"=>1234},
             {:"@a-float"=>345.78},
             {:@a_string=>"like yeah"}]},
          {:nesting=>
            [[1, 2, 3, 4, 5],
             [1.1, 2.2, 3.3, 4.4, 5.5],
             {:strings=>["test"]}]}]}],

     "[ [ [ [ [ [ [ [ one two three four five six seven eight \n"=>
      [{:eight=>
         [{:seven=>
            [{:six=>[{:five=>[{:four=>[{:three=>[{:two=>[{:one=>[]}]}]}]}]}]}]}]}],

     "[ 1 [ 2 [ 3 [ 4 [ 5 [ 6 six five four three two one \n"=>
      [{:one=>
         [1,
          {:two=>[2, {:three=>[3, {:four=>[4, {:five=>[5, {:six=>[6]}]}]}]}]}]}],

     "[ [ 5 4 3 2 1 ] [ 1 2 3 4 5 ] ] \n"=>[[[5, 4, 3, 2, 1], [1, 2, 3, 4, 5]]]
   }

    @parser = Stackish::Parser.new
    @emitter = Stackish::Emitter.new
  end

  def test_roundtrip
    @cases.each do |test,expect|
      res = @parser.process(test)
      assert_not_nil res
      assert_equal 1,res.length

      exp_stack = @emitter.process(expect)
      stackish = @emitter.process(res)

      assert_not_nil exp_stack
      assert_not_nil stackish

      assert_equal test, stackish
      assert_equal stackish, exp_stack
    end
  end


  def test_blob
    blobs = "[ "
    @cases.keys.each do |str|
      blobs << "'#{str.length}:#{str}' "
    end
    blobs << "blobs"

    res = @parser.process(blobs)
    assert_not_nil res
    assert_equal 1,res.length

    list = res[0][:blobs]
    assert_equal 4,list.length

    @cases.keys.each do |str|
      test = list.shift
      assert test.is_blob?
      assert_equal str,test
    end
  end

  def test_fuzz
    ["'300:too short'", "'3:too damn long'", "[ 1 2 3",
      "[ 1 2 3 @", "\"missing dquote"].each do |test|
      assert_raises Stackish::ParsingError do
        @parser.process(test)
      end
    end

    assert_raises Stackish::EmittingError do
      [Object.new].to_stackish
    end
  end

  def test_to_stackish
    @cases.keys.each do |test|
      res = test.from_stackish
      assert_not_nil res
      assert_equal 1,res.length

      array = res.to_stackish
      assert_not_nil array

      hash = res.shift.to_stackish
      if Hash === hash
        assert_not_nil hash
        assert_equal array, hash
      end
    end
  end

  def test_fancy_slash_accessor
    test = {:msg=>[
      {:chat=>[{:test=>[1, 2, 3]}, {:that=>["hi"]}, {:pass => ["no way"]}]},
      {:from => ["zed"]}
    ]}

    assert (test/:msg)
    assert (test/:msg/:chat)
    assert (test/:msg/:chat/:test)
    assert (test/:msg/:chat/[:test,:that])
    assert (test/:msg/[:chat,:from]/:pass)
    assert (test/:msg/:chat/0)
  end

end

