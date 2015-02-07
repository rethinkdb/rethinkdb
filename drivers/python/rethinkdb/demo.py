import asyncio
import rethinkdb as r


queue = asyncio.Queue()


@asyncio.coroutine
def adder(change):
    current = 0
    while True:
        value = yield from change.next()
        current += value
        yield from queue.put(('adder', current))


@asyncio.coroutine
def multiplier(change):
    current = 1
    while True:
        value = yield from change.next()
        current *= value
        yield from queue.put(('multiplier', current))


@asyncio.coroutine
def combiner:
    best_adder = 0
    best_multiplier = 1
    while True:
        component, value = queue.get()
        if component == 'adder':
            best_adder = max(best_adder, value)
            yield (best_adder, best_multiplier)
        elif component == 'multiplier':
            best_multiplier = max(best_multiplier, value)
            yield (best_adder, best_multiplier)


@asyncio.coroutine
def printer:
    seen = 0
    for adder, multiplier in combiner():
        seen += 1
        print "Saw adder, multiplier pair %d, %d" % (adder, multiplier)
        if seen > 20:
            loop.stop()


loop = asyncio.get_event_loop()
asyncio.async(adder(r.table('a').count().changes().arun(conn)))
asyncio.async(multiplier(r.table('b').count().changes().arun(conn)))
loop.call_soon(printer, loop)
try:
    loop.run_forever()
finally:
    loop.close()


@asyncio.coroutine
def a(change):
    while True:
        value = yield from change.next()
        pass


@asyncio.coroutine
def b(change):
    while True:
        value = yield from change.next()
        pass


loop = asyncio.get_event_loop()
asyncio.async(a(r.table('a').count().changes().arun(conn)))
asyncio.async(b(r.table('b').count().changes().arun(conn)))
loop.run_forever()
