# Copyright 2010-2012 RethinkDB, all rights reserved.
module RethinkDB
  # Data collectors are ways of aggregating data (for use with
  # Sequence#groupby).  You can define your own; all they are is a hash
  # containing a <b>+mapping+</b> to be applied to row, a
  # <b>+base+</b>/<b>+reduction+</b> to reduce over the mappings, and an
  # optional <b>+finalizer+</b> to call on the output of the reduction.  You can
  # expand the builtin data collectors below for examples.
  #
  # All of the builtin collectors can also be accessed using the <b>+r+</b>
  # shortcut (like <b>+r.sum+</b>).
  module Data_Collectors
    # Count the number of rows in each group.
    def self.count
      { :mapping => lambda{|row| 1},
        :base => 0,
        :reduction => lambda{|acc, val| acc + val} }
    end

    # Sum a particular attribute of the rows in each group.
    def self.sum(attr)
      { :mapping => lambda{|row| row[attr]},
        :base => 0,
        :reduction => lambda{|acc, val| acc + val} }
    end

    # Average a particular attribute of the rows in each group.
    def self.avg(attr)
      { :mapping => lambda{|row| [row[attr], 1]},
        :base => [0, 0],
        :reduction => lambda{|acc, val| [acc[0] + val[0], acc[1] + val[1]]},
        :finalizer => lambda{|res| res[0] / res[1]} }
    end
  end
end
