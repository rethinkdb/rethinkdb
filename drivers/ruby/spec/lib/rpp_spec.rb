require 'spec_helper'

RSpec.describe RethinkDB::RPP do
  context '.bt_consume' do
    it 'returns nil for falsey bt' do
      expect(subject.bt_consume(false, nil)).to be_nil
    end

    it 'returns nil when element is not first element of bt' do
      expect(subject.bt_consume([1], 2)).to be_nil
    end

    it 'returns proper slice of array' do
      expect(subject.bt_consume([1, 2, 3], 1)).to eq([2,3])
    end

    it 'returns proper slice for one element array' do
      expect(subject.bt_consume([1], 1)).to eq([])
    end
  end

  context '.pp_pseudotype' do
    it 'returns false for not pseudotype' do
      expect(subject.pp_pseudotype(nil, {}, nil)).to eq(false)
    end

    context 'TIME' do
      let(:pt_hash) do
        {'$reql_type$' => 'TIME'}
      end

      it 'returns false if no epoch_time' do
        expect(subject.pp_pseudotype(nil, pt_hash.merge('timezone' => 'CET'), nil)).to eq(false)
      end

      it 'returns false if no timezone' do
        expect(subject.pp_pseudotype(nil, pt_hash.merge('epoch_time' => 111), nil)).to eq(false)
      end

      context 'with correct params' do
        let(:time) { Time.now }
        let(:params) { pt_hash.merge('timezone' => '+01:00', 'epoch_time' => time.to_i)}
        let(:q) { double("q") }

        context 'with correct timezone' do
          it do
            expect(q).to receive(:text).with("r.expr(#{time.to_s})")
            subject.pp_pseudotype(q, params, nil)
          end
        end

        context 'with empty timezone' do
          it do
            expect(q).to receive(:text).with("r.expr(#{time.utc.to_s})")
            subject.pp_pseudotype(q, params.merge("timezone" => ''), nil)
          end
        end

        context 'with Z timezone' do
          it do
            expect(q).to receive(:text).with("r.expr(#{time.utc.to_s})")
            subject.pp_pseudotype(q, params.merge("timezone" => ''), nil)
          end
        end
      end
    end

    context 'BINARY' do
      let(:pt_hash) do
        {'$reql_type$' => 'BINARY'}
      end

      context 'without data' do
        it 'returns false' do
          expect(subject.pp_pseudotype(nil, pt_hash, nil)).to eq(false)
        end
      end

      context 'with data' do
        it do
          q = double('q')
          expect(q).to receive(:text).with('<data>')
          subject.pp_pseudotype(q, pt_hash.merge('data' => 123), nil)
        end
      end
    end

    context 'unsupported type' do
      it 'returns false' do
        hash = {'$reql_type$' => 'ASDF'}
        expect(subject.pp_pseudotype(nil, hash, nil)).to eq(false)
      end
    end
  end

  context '.pp_int_optargs' do
    let(:q) { double('q').as_null_object }

    context 'with pre_dot' do
      it 'receives opening clause' do
        expect(q).to receive(:text).with('r(')
        subject.pp_int_optargs(q, [], nil, true)
      end

      it 'receives closing clause' do
        expect(q).to receive(:text).with(')')
        subject.pp_int_optargs(q, [], nil, true)
      end
    end
  end
end
