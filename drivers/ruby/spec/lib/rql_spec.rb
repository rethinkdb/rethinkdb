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
    let(:r) { RethinkDB::RQL.new }

    it 'raises ReqlDriverCompileError on bitop operators' do
      expect { r.var(1) < r.var(true) | r.var(nil) }.to raise_error(RethinkDB::ReqlDriverCompileError)
    end

    context 'method with hash offset' do
      context 'body' do
        let(:body) { r.filter(a: 1).body }

        it 'has correct length' do
          expect(body.length).to eq(2)
        end

        it 'has correct first argument' do
          expect(body.first).to eq(39)
        end

        it 'has correct second argument' do
          hash = body.last.first
          expect(hash).to have_key("a")
        end
      end

      context 'and with optargs' do
        let(:body) { r.filter({a: 1}, {test: true}).body }

        it 'has correct length' do
          expect(body.length).to eq(3)
        end

        it 'has correct last argument' do
          expect(body.last).to be_kind_of(Hash)
          expect(body.last["test"]).to eq(true)
        end
      end
    end
  end
end
