import threading, sys, time, select, os

def readable(file):
    return bool(select.select([file],[],[],0)[0])

class NBReader(object):
    def __init__(self, stream = sys.stdin):
        self.stream = stream
        self.buffer = ""
        self.locked = False
        self.do_die = False
        self.idle_time = 0
        self.subthread = threading.Thread(target=self.run)
        self.subthread.daemon = True
        self.subthread.start()
    def run(self):
        self.loop()
    def loop(self):
        while True:
            if not self.locked:
                self.stream.flush()
                while readable(self.stream):
                    self.idle_time = 0
                    input = os.read(self.stream.fileno(), 1)
                    if input: self.buffer += input
                    else: break   # EOF
            if self.do_die: return
            time.sleep(0.1)
            self.idle_time += 0.1
    def get_lines(self):
        self.locked = True
        lines = self.buffer.split("\n")
        self.buffer = lines.pop(-1)
        self.locked = False
        return lines
    def get_buffer(self):
        self.locked = True
        buffer = self.buffer
        self.buffer = ""
        self.locked = False
        return buffer
    def kill(self):
        self.do_die = True

if __name__ == "__main__":
    r = NBReader()
    while True:
        lines = r.get_lines()
        print lines
        if "die" in lines:
            r.kill()
            break
        time.sleep(2)
