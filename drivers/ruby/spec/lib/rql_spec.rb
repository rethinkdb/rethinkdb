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

  context '#new_func' do
    subject { RethinkDB::RQL.new }

    it 'calls block' do
      b = Proc.new {}
      expect(b).to receive(:call)
      subject.new_func(&b)
    end

    it 'wraps argument in RQL' do
      b = Proc.new {|a|
        expect(a).to be_kind_of(RethinkDB::RQL)
      }
      subject.new_func(&b)
    end

    it 'calls #func' do
      b = Proc.new {|a| 3}
      expect_any_instance_of(RethinkDB::RQL).to receive(:func).with([2], 3)
      subject.new_func(&b)
    end
  end

  context 'rql methods' do
    subject { RethinkDB::RQL.new }

    it 'raises ReqlDriverCompileError on bitop operators' do
      r = subject
      expect { r.var(1) < r.var(true) | r.var(nil) }.to raise_error(RethinkDB::ReqlDriverCompileError)
    end
  end
end
