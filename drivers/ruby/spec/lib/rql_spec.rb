require 'spec_helper'

RSpec.describe RethinkDB::RQL do
  subject { RethinkDB::RQL }
  context '#pp' do
    context 'without RQL body' do
      it do
        expect(subject.new(nil).pp).to eq('r(nil)')
      end
    end

    context 'with RQL body' do
      it do
        expect{ subject.new.pp }.to raise_exception(NoMethodError)
      end
    end
  end

  context '#inspect' do
    context 'with RQL body' do
      it 'calls super' do
        expect_any_instance_of(subject.ancestors.first).to receive(:inspect)
        subject.new.inspect
      end
    end

    context 'without RQL body' do
      it 'calls pp' do
        object = subject.new(nil)
        expect(object).to receive(:pp)
        object.inspect
      end
    end
  end
end
