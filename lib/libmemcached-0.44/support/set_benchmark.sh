#!/bin/bash

src/memslap --concurrency=5 --execute-number=5000 --servers=localhost --test=set --flush
src/memslap --concurrency=5 --execute-number=5000 --non-blocking --servers=localhost --test=set --flush
src/memslap --concurrency=5 --execute-number=5000 --non-blocking --tcp-nodelay --servers=localhost --test=set --flush
