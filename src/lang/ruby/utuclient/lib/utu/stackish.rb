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

require 'strscan'


# Augments String with to_blob and is_blob? for tagging
# strings as having BLOB type for stackish.
class String

  # Does nothing more than set @is_blob=true and
  # return the original string (so it's fast).  In
  # order for binary data or other formats to be 
  # encapsulated in a Stackish BLOB you need to do this.
  def to_blob
    self.instance_variable_set(:@is_blob, true)
    self
  end

  # Tells you if a string is a BLOB.
  def is_blob?
    self.instance_variable_get(:@is_blob)
  end

  # Simply runs the parser on this string to convert it into a stackish structure.
  def from_stackish
    Utu::Stackish::Parser.new.process(self)
  end
end

class Hash
  # Processes this hash as a stackish structure to produce a string
  # in stackish format.
  def to_stackish
    Utu::Stackish::Emitter.new.process([self])
  end

  # Makes it so you can use a s/:name to access the
  # contents of the stackish.  Used more with the Array./
  # notation.
  def /(member)
    self[member]
  end
end


class Array
  # Processes this array containing stackish structures into a series
  # of stackish structures as one string.
  def to_stackish
    Utu::Stackish::Emitter.new.process(self)
  end

  # Used to access the members of a stackish struct by the
  # names of the child nodes.  You do it with the divide
  # doing struct/:msg/:child and it can take an Array.
  # When given an array (or anything responding to :collect)
  # it will build the list of all children that match it.
  #
  # This means you can do:  test/:msg/[:chat,:from]/:thing and
  # get all of the :thing children in chat or from.
  def /(member)
     results = if member.respond_to? :collect
                 self.collect {|child|
                   x = member.collect {|sel| child/sel }.flatten.compact
                   x.empty? ? nil : x
                 }
               elsif member.is_a? Fixnum
                 self[member]
               else
                 results = self.collect {|list| 
                   list/member if (list.respond_to? :to_stackish)
                 }.flatten
               end

     results.compact! if results.respond_to? :compact
     results
  end
end


module Utu
  module Stackish
    # Exception thrown by the Parser when it has a problem.
    class ParsingError < StandardError; end

    # Exception thrown by the Emitter when it has a problem.
    class EmittingError < StandardError; end

    # The Parser takes a Stackish string and parses it into a 
    # series of Hashes, Arrays, Strings, Integers, and Floats
    # for you to handle in Ruby.
    class Parser

      # The current partially parsed structure, don't mess with it.
      attr_reader :parse_stack

      # The current marked parts of the stackish structure.
      attr_reader :mark_stack

      # The StringScanner tokens used during parsing.
      TOKENS = [
        [/\s+/ , :space],
        [/\[/ , :mark],
        [/\]/ , :group],
        [/:/, :colon],
        [/'/, :squote],
        [/@/  , :attrib],
        [/(\+|\-)?\d+\.\d+/ , :float],
        [/\d+/ , :number],
        [/\w+(\w|\-|_|\.|:)*/ , :word],
        [/".*?"/  , :string],
      ]

      # Inverse mapping of :token => regex for matching a specific
      # token in the stream.
      TOKENS_INVERTED = TOKENS.inject({}) {|i,v| i[v.last] = v.first; i }  

      # Given a string in Stackish format the process method will
      # parse it and convert it into a mixture of Arrays, Hashes,
      # Strings, Integers, and Floats.  Hashes are used as individual
      # named elements with the key being a Symbol for the word.  Each
      # Hash has an Array of children (which can be empty).  Each
      # child can then be an Array (for unnamed groups), Integer, Float,
      # String, or more Hash.
      #
      # For example, if you give this:
      #
      #   [ test 
      #
      # Then you'll get:
      #
      #   [{:test=>[]}] 
      #
      # Then if you add elements to the test node like so:
      #
      #   [ 1 2 3 4 test
      #
      # Then you'll get:
      #
      #   [{:test=>[1, 2, 3, 4]}]
      #
      # (Notice the numbers are "backwards").  If you add a group into that:
      #
      #   [ [ "hi" ] 1 2 3 4 test
      #
      # Then you'll get:
      #
      #   [{:test=>[["hi"], 1, 2, 3, 4]}]
      #
      # And if you decide to name this "hi" group:
      #
      #   [ [ "hi" howdy 1 2 3 4 test
      #
      # You'll then get:
      #
      #   [{:test=>[{:howdy=>["hi"]}, 1, 2, 3, 4]}]
      # 
      # And finally, you use the String#to_blob and String#is_blob? functions
      # to create blobs.
      #
      # The most convenient way to use Stackish is to simply work with String#from_stackish
      # and Array#to_stackish (or Hash#to_stackish).
      #
      # NOTE:  Currently process will throw in exception if the string doesn't 
      # represent a complete stackish structure. 
      def process(string)
        @parse_stack = []
        @mark_stack = []
        @scanner = StringScanner.new(string)

        tok = ""
        while !@scanner.eos?
          TOKENS.each do |p,r|
            tok = @scanner.check(p)
            if tok and tok.length > 0
              skip(tok.length)
              # skip spaces but process everything else
              self.send(r, tok) unless r == :space
              break
            end
          end

          if !tok or tok.length == 0
            raise ParsingError.new("Invalid stackish string at #{@scanner.pos} character, failed to parse.")
          end
        end

        raise ParsingError.new("Incomplete Stackish structure in string.  Expecting at least #{@mark_stack.length} words or groups.") if !finished?

        return @parse_stack
      end

      # Since Stackish is stack ordered, you can tell when you've processed
      # all of the structure very easily.
      def finished?
        @mark_stack.empty?
      end

      protected

      # Takes the current @parse_stack stack top position and pushes it onto
      # the @mark_stack stack for later popping off to produce groups.
      def mark(token)
        @mark_stack.push(@parse_stack.length)
      end

      # Creates an unnamed group (doesn't use token)
      def group(token)
        @parse_stack << collapse
      end

      # Produces a string (NOT blob) by stripping off the \" chars around
      # what scanner produces.
      def string(token)
        # TODO: gotta be a way to make strscan do this from the regex
        @parse_stack << token[1 ... token.length-1]
      end

      # Process a BLOB specification of the format 'LENGTH:DATA'.
      def squote(token)
        size = match(:number).to_i
        match(:colon)
        blob = @scanner.rest[0 ... size].to_blob
        skip(size)
        match(:squote)
        @parse_stack << blob
      end

      # Names whatever is on the top with the given attribute word
      def attrib(token)
        @parse_stack << name((token + match(:word)), @parse_stack.pop)
      end

      def float(token)
        @parse_stack << token.to_f
      end

      def number(token)
        @parse_stack << token.to_i
      end

      # Takes all the elements from the current top to the most recently
      # pushed mark, collapses them into a group, then names that group.
      def word(token)
        @parse_stack << name(token, collapse)
      end

      # Matches the given token and returns it.  Raises a ParsingError if the
      # match doesn't happen.
      def match(name)
        m = @scanner.scan(TOKENS_INVERTED[name])
        if !m or m.length == 0
          raise ParsingError.new("Expected #{name} token at #{@scanner.pos} character of input string.")
        end
        return m
      end

      # Skips over this much of the input text.
      def skip(length)
        begin
          @scanner.pos += length
        rescue RangeError
          raise ParsingError.new("Not enough input in string at #{@scanner.pos}, expecting #{@scanner.pos + length - @scanner.string.length} more.")
        end
      end

      # Names a group with a Symbol by wrapping it with a single
      # name=>children mapping Hash.
      def name(name, element)
        { name.to_sym => element}
      end

      # Takes the most recent mark off the @mark_stack stack, slices from that point
      # on into a separate array, then returns it so this new group can be
      # processed.
      def collapse
        top = @mark_stack.pop
        raise ParsingError.new("Unbalanced stackish structure at #{@scanner.pos}.") unless top
        @parse_stack.slice!(top, @parse_stack.length)
      end
    end


    class Emitter
      def process(stackish)
        stackish.inject([]) {|a,el| a << convert(el); a }.join(" ") + " \n"
      end

      def convert(element, named=false)
        case element
        when Hash
          name = element.keys[0].to_s
          [convert(element.values[0], name[0] != "@"[0])] + [name]
        when Array
          ["["] + element.inject([]) {|i,x| i << convert(x); i } + (named ? [] : [']'])
        when String
          if element.is_blob?
            "'#{element.length}:#{element.to_s}'"
          else
            "\"#{element.to_s}\""
          end
        when Integer
          element.to_s
        when Float
          element.to_s
        else
          raise EmittingError.new("Unknown Stackish type #{element.class}: #{element.inspect}")
        end
      end
    end
  end
end
