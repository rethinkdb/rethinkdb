# Copyright 2010-2012 RethinkDB, all rights reserved.
GHCFLAGS:= -fwarn-incomplete-patterns -Werror

adtproto: FORCE
	ghc $(GHCFLAGS) --make -o $@ Main

example.adt.proto: example.adt adtproto
	./adtproto $<

.PHONY: clean
clean:
	rm -f *.o *.hi
	rm -f adtproto example.adt.proto example.adt.cc

.PHONY: FORCE
FORCE:
