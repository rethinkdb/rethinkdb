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
