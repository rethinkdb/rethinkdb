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
        let(:zone) { Time.now.isdst ? '+02:00' : '+01:00' }
        let(:params) { pt_hash.merge('timezone' => zone, 'epoch_time' => time.to_i)}
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

    context 'block to group' do
      let(:q) { spy("q") }

      before do
        allow(q).to receive(:group).and_yield
      end

      it 'should call q#text' do
        subject.pp_int_optargs(q, [1, 2, 3], nil, false)
        expect(q).to have_received(:text).exactly(5).times
      end

      it 'should call q#breakable' do
        subject.pp_int_optargs(q, [1, 2, 3], nil, false)
        expect(q).to have_received(:text).exactly(5).times
      end

      context 'with yielding #nest' do
        before do
          allow(q).to receive(:nest).and_yield
        end

        it 'should call q#text' do
          subject.pp_int_optargs(q, [1, 2, 3], nil, false)
          expect(q).to have_received(:text).exactly(8).times
        end

        it 'should call q#breakable' do
          subject.pp_int_optargs(q, [1, 2, 3], nil, false)
          expect(q).to have_received(:text).exactly(8).times
        end

        it 'should call #pp_int' do
          expect(subject).to receive(:pp_int).exactly(3).times
          subject.pp_int_optargs(q, [1, 2, 3], nil, false)
        end
      end
    end
  end

  context '.pp_int_args' do
    let(:q) { spy("q") }

    before do
      allow(q).to receive(:group).and_yield
    end

    context 'with pre_dot' do
      it 'receives opening clause' do
        expect(q).to receive(:text).with('r(')
        subject.pp_int_args(q, [], nil, true)
      end

      it 'receives closing clause' do
        expect(q).to receive(:text).with(')')
        subject.pp_int_args(q, [], nil, true)
      end
    end

    context 'with non-empty args' do
      context 'with one element' do
        let(:args) { [1] }

        it 'does not call q.text in block' do
          subject.pp_int_args(q, args, nil)
          expect(q).to have_received(:text).once # called explicitly before block
        end

        it 'does not call q.breakable' do
          subject.pp_int_args(q, args, nil)
          expect(q).not_to have_received(:breakable)
        end

        it 'does call pp_int' do
          expect(subject).to receive(:pp_int).with(q, 1, nil)
          subject.pp_int_args(q, args, nil)
        end
      end

      context 'with more args' do
        let(:args) { [1, 2, 3] }

        it 'calls q.breakable' do
          subject.pp_int_args(q, args, nil)
          expect(q).to have_received(:breakable).twice
        end

        it 'calls pp_int for each arg' do
          expect(subject).to receive(:pp_int).with(q, 1, nil)
          expect(subject).to receive(:pp_int).with(q, 2, nil)
          expect(subject).to receive(:pp_int).with(q, 3, nil)
          subject.pp_int_args(q, args, nil)
        end
      end
    end
  end

  context '.pp_int_datum' do
    let(:q)   { double('q') }
    let(:dat) { double('dat').as_null_object }

    it 'calls inspect and text' do
      expect(dat).to receive(:inspect)
      expect(q).to receive(:text)
      subject.pp_int_datum(q, dat, false)
    end
  end

  context '.pp_int_func' do
    let(:q) { spy('q') }
    let(:var) { RethinkDB::RQL.new.var(42) }
    let(:correct_args) { [32, [[42, [var]], 69]]}

    before do
      allow(q).to receive(:group).and_yield
      allow(q).to receive(:nest).with(2).and_yield
    end

    it 'should be unprintable function' do
      expect(q).to receive(:text).with(/unprintable function/)
      subject.pp_int_func(q, [], nil)
    end

    it 'should call text with space' do
      subject.pp_int_func(q, correct_args, nil)
      expect(q).to have_received(:text).with(' ')
    end

    it 'should call nest with 2 and yield' do
      subject.pp_int_func(q, correct_args, nil)
      expect(q).to have_received(:nest).with(2)
      expect(q).to have_received(:breakable).twice
    end

    it 'should call pp_int' do
      expect(subject).to receive(:pp_int).with(q, 69, nil)
      subject.pp_int_func(q, correct_args, nil)
    end
  end

  context '.can_prefix' do
    context 'table' do
      it do
        expect(subject.can_prefix('table', nil)).to eq(false)
      end

      context 'with array args' do
        it 'false with empty array' do
          expect(subject.can_prefix('table', [])).to eq(false)
        end

        it 'false with array with not first array' do
          expect(subject.can_prefix('table', [nil])).to eq(false)
        end

        it 'false wiith nested array and not TermType::DB' do
          expect(subject.can_prefix('table', [[nil]])).to eq(false)
        end

        it 'true with proper args' do
          expect(subject.can_prefix('table', [[RethinkDB::Term::TermType::DB]])).to eq(true)
        end
      end
    end

    context 'table drop' do
      it do
        expect(subject.can_prefix('table_drop', nil)).to eq(false)
      end
    end

    context 'table create' do
      it do
        expect(subject.can_prefix('table_create', nil)).to eq(false)
      end
    end

    ["db", "db_create", "db_drop", "json", "funcall",
             "args", "branch", "http", "binary", "javascript", "random",
             "time", "iso8601", "epoch_time", "now", "geojson", "point",
             "circle", "line", "polygon", "asc", "desc", "literal",
             "range", "error"].each do |name|
      it do
        expect(subject.can_prefix(name, nil)).to eq(false)
      end
    end

    it 'true with prefixable name' do
      expect(subject.can_prefix('asdf', nil)).to eq(true)
    end
  end

  context '.pp' do
    it 'should return error message on exception' do
      allow(subject).to receive(:pp_int) { raise RuntimeError }
      expect{ subject.pp(nil) }.not_to raise_error
      expect(subject.pp(nil)).to match("AN ERROR OCCURED DURING PRETTY-PRINTING")
      expect(subject.pp(nil)).to match("RuntimeError")
    end

    context 'with fake PP' do
      let(:fake_pp) { spy('pp') }
      let(:var) { RethinkDB::RQL.new.var(42) }

      before do
        allow(PrettyPrint).to receive(:new).and_return(fake_pp)
        subject.pp(var)
      end

      it 'should call flush' do
        expect(fake_pp).to have_received(:flush)
      end

      it 'should call output' do
        expect(fake_pp).to have_received(:output)
      end

      context 'with fake output' do
        it 'should strip leading \x7' do
          allow(fake_pp).to receive(:output).and_return("\x7\x7[abc]")
          expect(subject.pp(var)).to eq("[abc]\n")
        end
      end
    end
  end
end
