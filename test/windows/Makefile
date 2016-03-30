RT := ../../x64/Debug/RethinkDB.exe
LIST := $(RT) test --gtest_list_tests | perl -ne 'chop; chop; $$t = "$$1" if /^([^ ].*)/; print "$$t$$1\n" if /^  (.*)/'

ifeq ($(V),1)
OUT := 2>&1 | tee
else
OUT := 2>&1 | cat >
endif

%.out: $(RT)
	$(RT) test --gtest_filter=$* $(OUT) $@

.PHONY: list
list:
	$(LIST)

list-failed:
	grep -L 'OK ]' *.out

cat-failed:
	cat `grep -L 'OK ]' *.out`

.PHONY: touch
touch:
	touch `$(LIST) | sed 's/$$/.out/'`

.PRECIOUS: %.out
