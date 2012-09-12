#!/usr/bin/python
import sys, random, time, os
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'common')))
import memcached_workload_common
from vcoptparse import *

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

op = memcached_workload_common.option_parser_for_socket()
op["duration"] = IntFlag("--duration", 1000)
opts = op.parse(sys.argv)

with memcached_workload_common.make_socket_connection(opts) as s:
    sent_log = open('fuzz_sent', 'w')
    recv_log = open('fuzz_recv', 'w')

    start_time = time.time()

    time.sleep(2)

    while (time.time() - start_time < opts["duration"]):
        time.sleep(.05)
        string = ''
        for i in range(20):
            choice = random.random()
            if choice < .5:
                string += word()
            elif choice < .75:
                string += number()
            elif choice < .85:
                string += garbage()
            elif choice < .99:
                string += delim()
            else:
                string += funny()

        for substr in rand_split(string, random.randint(10, 40)):
            s.send(substr)
            sent_log.write(substr)

        s.settimeout(0)
        try:
            server_str = s.recv(100000)
            recv_log.write(server_str)
        except:
            pass

    s.send('quit\r\n')
