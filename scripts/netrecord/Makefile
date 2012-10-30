# Copyright 2010-2012 RethinkDB, all rights reserved.
netrecord: netrecord.c Makefile
	gcc -o netrecord netrecord.c -Wall -g -lpthread

install: netrecord
	install netrecord /usr/local/bin/netrecord

clean:
	rm -f netrecord
