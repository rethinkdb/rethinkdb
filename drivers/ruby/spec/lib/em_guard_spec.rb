require 'spec_helper'
require 'eventmachine'

RSpec.describe RethinkDB::EM_Guard do
  let(:conn) { double('conn').as_null_object }
  let(:mutex) { spy('mutex') }
  let(:conns) { subject.class_variable_get(:@@conns) }

  subject { RethinkDB::EM_Guard }

  before do
    @original_mutex = subject.class_variable_get(:@@mutex)
    @original_conns = subject.class_variable_get(:@@conns)
    subject.class_variable_set(:@@mutex, mutex)
    subject.class_variable_set(:@@conns, Set.new)
    subject.class_variable_set(:@@registered, false)

    allow(mutex).to receive(:synchronize).and_yield

  end

  after do
    subject.class_variable_set(:@@mutex, @original_mutex)
    subject.class_variable_set(:@@conns, @original_conns)
  end

  context '.register' do
    before do
      allow(EM).to receive(:add_shutdown_hook)
      subject.register(conn)
    end

    it 'calls mutex.synchronize' do
      expect(mutex).to have_received(:synchronize)
    end

    it 'changes @@registered to true' do
      expect(subject.class_variable_get(:@@registered)).to eq(true)
    end

    it 'adds conn to conns' do
      expect(conns).to include(conn)
    end
  end

  context '.unregister' do
    let(:conn2) { double('conn2').as_null_object }
    before do
      subject.class_variable_set(:@@conns, Set.new([conn, conn2]))
      subject.unregister(conn)
    end

    it 'calls mutex.synchronize' do
      expect(mutex).to have_received(:synchronize)
    end

    it 'adds conn to conns' do
      expect(conns).not_to include(conn)
    end

    it 'keeps conn2 in conns' do
      expect(conns).to include(conn2)
    end
  end

  context '.remove_em_waiters' do
    let(:conn2) { double('conn2').as_null_object }

    before do
      subject.class_variable_set(:@@conns, Set.new([conn, conn2]))
    end

    it 'calls remove_em_waiters on each conn' do
      expect(conn).to receive(:remove_em_waiters)
      expect(conn2).to receive(:remove_em_waiters)
      subject.remove_em_waiters
    end

    it 'resets conns to empty set' do
      subject.remove_em_waiters
      expect(conns).to be_empty
    end
  end
end
