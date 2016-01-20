require 'spec_helper'

RSpec.describe RethinkDB::Shortcuts do
  subject do
    class TestSubject
      include RethinkDB::Shortcuts
    end.new
  end

  context '#r' do
    it 'should be of RQL with empty array' do
      expect(subject.r([])).to be_kind_of(RethinkDB::RQL)
    end

    it 'should be of RQL with non-empty array' do
      expect(subject.r([1])).to be_kind_of(RethinkDB::RQL)
    end
  end
end
