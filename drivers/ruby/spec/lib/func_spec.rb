require 'spec_helper'

RSpec.describe RethinkDB::RQL do
  context '#==' do
    it 'raises ArgumentError' do
      expect{ subject == 1 }.to raise_exception(ArgumentError)
    end
  end

  context '#[]' do
    context 'with range' do
      it 'should call slice with rigth bound closed' do
        expect(subject).to receive(:slice).with(1, 3, {right_bound: 'closed'})
        subject[1..3]
      end

      it 'should call slice with rigth bound open' do
        expect(subject).to receive(:slice).with(1, 3, {right_bound: 'open'})
        subject[1...3]
      end
    end

    context 'with no range' do
      it do
        expect(subject).to receive(:bracket).with(1)
        subject[1]
      end
    end
  end

  context '#connect' do
    subject { RethinkDB::RQL }

    context 'with block' do
      let(:opts) { {a: 1} }
      it 'raises exception with not-RQL' do
        expect{ subject.new(1).connect }.to raise_exception(NoMethodError)
      end

      it 'calls block' do
        block = -> { double(:block) }
        expect(block).to receive(:call).with(an_instance_of(RethinkDB::Connection)) { true }
        subject.new(RethinkDB::RQL).connect(opts, &block)
      end

      it 'closes connection on exception' do
        block = -> { double(:block) }
        allow(block).to receive(:call) { raise RuntimeError }
        expect_any_instance_of(RethinkDB::Connection).to receive(:close)
        begin subject.new(RethinkDB::RQL).connect(opts, &block); rescue; end
      end
    end
  end

  context '#do' do
    context 'with non-RQL body' do
      subject { RethinkDB::RQL }

      it 'does not raise exception with empty argunemt list' do
        expect{ subject do; end }.not_to raise_exception
      end
    end

    context 'with RQL body' do
      context 'and empty arguments' do
        it do
          expect{ subject.do }.to raise_exception(RethinkDB::ReqlDriverCompileError)
        end
      end
    end
  end

  context '#row' do

    context 'with non-RQL body' do
      subject { RethinkDB::RQL }
      it do
        expect{ subject.new(nil).row }.to raise_exception(NoMethodError, /undefined method `row'/)
      end
    end

    context 'with RQL body' do
      context 'without args' do
        it do
          expect{ subject.row }.to raise_exception(NoMethodError, /Sorry, r.row is not available/)
        end
      end
    end
  end
end
