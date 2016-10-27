require 'spec_helper'

RSpec.describe RethinkDB::Handler do
  subject { described_class.new }
  context 'stopping' do
    it 'is not stopped by default' do
      expect(subject.stopped?).to eq(false)
    end

    it 'is stoped after calling .stop' do
      subject.stop
      expect(subject.stopped?).to eq(true)
    end
  end

  context 'handle :on_error' do
    it 'raises error' do
      caller = Object.new
      expect { subject.handle(:on_error, StandardError, caller) }.to raise_error(StandardError)
    end
  end

  context 'CallbackHandler' do
    subject { RethinkDB::CallbackHandler }

    context '.initialize' do
      it 'raises error for arity bigger than 2' do
        callback = ->(a, b, c) {}
        expect { subject.new(callback) }.to raise_error(ArgumentError)
      end

      it 'does not raise error for arity 0' do
        callback = ->() {}
        expect { subject.new(callback) }.not_to raise_error
      end

      it 'does not raise error for arity 1' do
        callback = ->(a) {}
        expect { subject.new(callback) }.not_to raise_error
      end

      it 'does not raise error for arity 2' do
        callback = ->(a, b) {}
        expect { subject.new(callback) }.not_to raise_error
      end
    end

    context '.do_call' do
      subject { super().new(callback) }
      let(:val) { Object.new }

      context 'with callback of arity 0' do
        let(:callback) { ->() {} }

        it 'calls callback if no error given' do
          expect(callback).to receive(:call).with(no_args)
          subject.do_call(nil, val)
        end

        it 'raises error if err' do
          expect{ subject.do_call(StandardError, val) }.to raise_error(StandardError)
        end
      end

      context 'with callback of arity 1' do
        let(:callback) { ->(a) {} }

        it 'calls callback if no error given' do
          expect(callback).to receive(:call).with(val)
          subject.do_call(nil, val)
        end

        it 'raises error if err' do
          expect{ subject.do_call(StandardError, val) }.to raise_error(StandardError)
        end
      end

      context 'with callback of arity 2' do
        let(:callback) { ->(a, b) {} }

        it 'calls callback with error and value' do
          expect(callback).to receive(:call).with(nil, val)
          subject.do_call(nil, val)
        end

        it 'calls callback with error and value if error present' do
          expect(callback).to receive(:call).with(StandardError, val)
          subject.do_call(StandardError, val)
        end
      end
    end
  end
end
