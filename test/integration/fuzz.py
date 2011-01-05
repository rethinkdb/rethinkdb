#!/usr/bin/python
import os, sys, socket, random, time
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
from test_common import *

# "I am a string" -> ["I a", "m a s", "trin", "g"]
def rand_split(string, nsub_strings):
    cutoffs = random.sample(range(len(string)), nsub_strings);
    cutoffs.sort();
    cutoffs.insert(0, 0)
    cutoffs.append(len(string))
    strings = []
    for (start, end) in zip(cutoffs[0:len(cutoffs) - 1], cutoffs[1:len(cutoffs)]):
        strings.append(string[start:end])

    return strings

meaningful_words = ['set', 'get', 'delete', 'gets', 'add', 'replace', 'append', 'prepend', 'cas', 'incr', 'decr', 'stat', 'rethinkdbctl', 'noreply'] 

def word():
    return random.choice(meaningful_words)

delims = ['\t', ' ', '\n', '\r']

def delim():
    return random.choice(delims)

def number():
    return str(random.randint(0, 999999999999))

def garbage():
    chars=['a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','1','2','3','4','5','6','7','8','9','0','!','@','#','$','%','^','&','*','(',')','_','+',',','.','/',';','[',']','<','>',':','{','}','?','\r\n','\r','\n','\t',' ']
    len = random.randint(1, 1000)
    res = ''
    for i in range(len):
        res += random.choice(chars)
    return res

def funny():
    return "Yo dawg"

def test_function(opts, port, test_dir):

    sent_log = open('fuzz_sent', 'w')
    recv_log = open('fuzz_recv', 'w')
    
    start_time = time.time()

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", port))

    time.sleep(2)

    while (time.time() - start_time < opts["duration"]):
        time.sleep(.05)
        str = ''
        for i in range(20):
            choice = random.random()
            if choice < .5:
                str += word()
            elif choice < .75:
                str += number()
            elif choice < .85:
                str += garbage()
            elif choice < .99:
                str += delim()
            else:
                str += funny()

        for substr in rand_split(str, random.randint(10, 40)):
            s.send(substr)
            sent_log.write(substr)

        s.settimeout(0)
        try:
            server_str = s.recv(100000)
            recv_log.write(server_str)
        except:
            pass

    s.send('quit\r\n')

    s.close()

if __name__ == "__main__":
    op = make_option_parser()
    op["duration"] = IntFlag("--duration", 1000)
    opts = op.parse(sys.argv)
    auto_server_test_main(test_function, opts, timeout = opts["duration"] + 30)
