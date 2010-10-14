import os
import re
NEVENTS = 4

ctrl_str = 'sudo opcontrol'
rprt_str = 'opreport'
exec_name = 'rethinkdb'

def safe_div(x, y):
    if y == 0:
        return x
    else:
        return x / y

def dict_add(x, y):
    res = {}
    assert len(x) == len(y)
    for keyx, keyy in zip(sorted(x), sorted(y)):
        assert keyx == keyy
        res[keyx] = x[keyx]+ y[keyy]
    return res

class Event():
    name = ''
    count = 90000
    mask = 0x01
    kernel = 0
    user = 1
    def __init__(self, _name, _count = 90000, _mask = 0x01, _kernel = 0, _user = 1):
        self.name = _name
        self.count = _count
        self.mask = _mask
        self.kernel = _kernel
        self.user = _user
    def cmd_str(self):
        return '--event=%s:%d:%x:%d:%d' % (self.name, self.count, self.mask, self.kernel, self.user)

class OProfile():
    def start(events):
        os.system(ctrl_str + ' --reset')
        os.system(ctrl_str + ' --no-vmlinux')
        os.system(ctrl_str + ' --separate=lib,kernel,cpu')
        if len(events) > 0:
            event_str = ctrl_str + ' '
            for event in events:
                event_str += (event.cmd_str() + ' ')
            os.system(event_str)
        os.system(ctrl_str + ' --start')

    def stop_and_report():
        os.system(ctrl_str + ' --shutdown')
        os.system(rprt_str + ' --merge=cpu,lib,tid,tgid,unitmask,all -gdf | op2calltree')
        p = parser()
        return p.parse_file('oprof.out.' + exec_name)

    def clean():
        os.system('rm oprof.out.*')

#Parsing code
class line():
    regex = ""
    fields = [] #tuples of form name, type
    def __init__(self, _regex, _fields):
        self.regex = _regex
        self.fields = _fields
    def parse_line(self, line):
        matches = re.match(self.regex, line)
        if matches:
            result = {}
            for field, groupi in zip(self.fields, range(1, len(self.fields) + 1)):
                if (field[1] == 'd'):
                    val = int(matches.group(groupi))
                elif (field[1] == 's'):
                    val = matches.group(groupi)
                elif (field[1] == 'x'):
                    val = int(matches.group(groupi), 0)
                result[field[0]] = val
            return result
        else:
            return False
                    
class Function_report():
    function_name = ''
    counter_totals = {}
    source_file = ''
    lines = {} #number -> line_report

class Line_report():
    line_number = None
    counter_totals = {}
    def __init__(self, _line_number, _counter_totals):
        self.line_number = _line_number
        self.counter_totals = _counter_totals

class Program_report():
    object_name = ''
    counter_totals = {}
    counter_names = ('','','','')
    functions = {} #string -> function_report
    def __str__(self):
        res = ''
        for name, total in zip(self.counter_names, self.counter_totals):
            res += (str(name) + ": " + str(total) + ' ')
        res += '\n'
        res += str(self.functions)
        return res

class parser():
    positions_line = line("positions: (\w+) (\w+)\n", [('instr', 's'), ('line', 's')])
    events_line     = line("events:\s+(\w+)\s+(\w+)\s+(\w+)\s+(\w+)\n", [('event1', 's'), ('event2', 's'), ('event3', 's'), ('event4', 's')])
    summary_line    = line("summary: (\d+) (\d+) (\d+) (\d+)\n", [('event1', 'd'), ('event2', 'd'), ('event3', 'd'), ('event4', 'd')])
    obj_line        = line("ob=(.+)\n", [('obj_file', 's')])
    function_line   = line("fn=(.+)\n", [('function_name', 's')])
    source_line     = line("fi=\(\d+\)\s+(.+)\n", [('source_file', 's')])
    sample_line     = line("(0x[0-9a-fA-F]{8})\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\n", [('instruction', 'x'), ('line_number', 'i'), ('event1', 'd'), ('event2', 'd'), ('event3', 'd'), ('event4', 'd')])
    fident_line     = line("fi=\(\d+\)", [])
    prog_report     = Program_report()
    def __init__(self):
        pass

    #data should be an array of strings
    #pop the first line off the data and try to match it 
    def take(self, line, data):
        if len(data) == 0:
            return False
        matches = line.parse_line(data[len(data) - 1])
        data.pop()
        return matches

    #look through an array of data until you get a match (or run out of data)
    def until(self, line, data):
        while len(data) > 0:
            matches = line.parse_line(data[len(data) - 1])
            data.pop()
            if matches:
                return matches

    #iterate through lines while they match (and there's data)
    def read_while(self, lines, data):
        res = []
        while len(data) > 0:
            for line in lines:
                m = line.parse_line(data[len(data) - 1])
                if m:
                    break
            if m:
                res.append(m)
                data.pop()
            else:
                return res

    def parse_function(self, data):
        if (len(data) == 0):
            return False

        function_report = Function_report()

        f_name = self.until(self.function_line, data)
        if not f_name:
            return False
        function_report.function_name = f_name['function_name']

        source = self.take(self.source_line, data)
        if source:
            function_report.source_file = source['source_file']

#set counter_totals to 0
        function_report.counter_totals = {}
        for i in range(len(self.prog_report.counter_names)):
            function_report.counter_totals[self.prog_report.counter_names[i]] = 0

        samples = []
        while True:
            samples += self.read_while([self.sample_line], data)
            res = self.read_while([self.source_line, self.fident_line], data)
            if not res:
                break
        for sample in samples:
            line_report = Line_report(sample['line_number'], {self.prog_report.counter_names[0] : sample['event1'], self.prog_report.counter_names[1] : sample['event2'], self.prog_report.counter_names[2] : sample['event3'], self.prog_report.counter_names[3] : sample['event4']})
            function_report.counter_totals = dict_add(function_report.counter_totals, line_report.counter_totals)
            function_report.lines[line_report.line_number] = line_report
        return function_report
            
    def parse_file(self, file_name):
        file = open(file_name)
        data = file.readlines()
        data.reverse() #for some reason pop takes things off the back (it's shit like this Guido, shit like this)

        positions = self.until(self.positions_line, data)
        assert positions['instr'] == 'instr' and positions['line'] == 'line'

        events = self.take(self.events_line, data)
        assert events
        self.prog_report.counter_names = (events['event1'], events['event2'], events['event3'], events['event4'])

        self.prog_report.counter_totals = {}
        for i in range(len(self.prog_report.counter_names)):
            self.prog_report.counter_totals[self.prog_report.counter_names[i]] = 0


        summary = self.take(self.summary_line, data)
        assert summary
        self.prog_report.counter_totals = {self.prog_report.counter_names[0] : summary['event1'], self.prog_report.counter_names[1] : summary['event2'], self.prog_report.counter_names[2] : summary['event3'], self.prog_report.counter_names[3] : summary['event4']}
        
        object_name = self.until(self.obj_line, data)
        assert object_name 
        self.prog_report.object_name = object_name

        while True:
            function_report = self.parse_function(data)
            if not function_report:
                break
            self.prog_report.functions[function_report.function_name] = function_report

        return self.prog_report

class Profile():
    events = []
    ratios = []
    def __init__(self, _events, _ratios):
        self.events = _events
        self.ratios = _ratios
        for r in self.ratios:
            assert r.numerator in [e.name for e in self.events]
            assert r.denominator in [e.name for e in self.events]

#small packet ratios
class Ratio():
    numerator = ''
    denominator = ''
    top_n = 5
#_numerator and denominator should be Event()s
    def __init__(self, _numerator, _denominator):
        self.numerator = _numerator.name
        self.denominator = _denominator.name
    def report(self, prog_report):
        import StringIO
        res = StringIO.StringIO()
        print >>res, "%s / %s = " % (self.numerator, self.denominator)
        print >>res, safe_div(prog_report.counter_totals[self.numerator], prog_report.counter_totals[self.denominator])

        print >>res, "Top %d functions:" % self.top_n
        function_list = sorted(prog_report.functions.iteritems(), key = lambda x: safe_div(x[1].counter_totals[self.numerator], x[1].counter_totals[self.denominator]))
        function_list.reverse()
        for function in function_list[0:self.top_n]:
            print >>res, function[0], ' = ',
            print >>res, safe_div(function[1].counter_totals[self.numerator], function[1].counter_totals[self.denominator])
        return res.getvalue()
