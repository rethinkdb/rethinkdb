require 'spec_helper'

RSpec.describe RethinkDB::Utils do
  subject do
    class TestSubject
      include RethinkDB::Utils
    end.new
  end

  context '#get_mname' do
    it do
      expect(subject.get_mname(2)).to eq('instance_exec')
    end

    it do
      expect(subject.get_mname(1)).to eq('block (3 levels) in <top (required)>')
    end

    it do
      expect(subject.get_mname(0)).to eq('get_mname')
    end
  end

  context '#unbound_if' do
    context 'with false' do
      it 'does not raise' do
        expect{ subject.unbound_if(false) }.not_to raise_exception
      end
    end

    context 'with true' do
      let(:name) { 'xxx' }

      it 'raises error with name' do
        expect{ subject.unbound_if(true, name) }.to raise_exception(NoMethodError, "undefined method `#{name}'")
      end

      it 'raises error without name' do
        expect{ subject.unbound_if(true) }.to raise_exception(NoMethodError, "undefined method `block (5 levels) in <top (required)>'")
      end
    end
  end
end
