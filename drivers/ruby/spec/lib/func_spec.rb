require 'spec_helper'

RSpec.describe RethinkDB::RQL do
  context '==' do
    it 'raises ArgumentError' do
      expect{ subject == 1 }.to raise_exception(ArgumentError)
    end
  end
end
