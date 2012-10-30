# Copyright 2010-2012 RethinkDB, all rights reserved.
import re
class line():
    regex = ""
    fields = [] #tuples of form name, type
    def __init__(self, _regex, _fields):
        self.regex = _regex
        self.fields = _fields

    def __repr__(self):
        return self.regex

    def parse_line(self, line):
        matches = re.match(self.regex, line)
        if matches:
            result = {}
            for field, groupi in zip(self.fields, range(1, len(self.fields) + 1)):
                try:
                    if (field[1] == 'd'):
                        val = int(matches.group(groupi))
                    elif (field[1] == 'f'):
                        val = float(matches.group(groupi))
                    elif (field[1] == 's'):
                        val = matches.group(groupi)
                    else:
                        assert 0
                except ValueError:
                    val = None
                result[field[0]] = val
            return result
        else:
            return False

def take(line, data):
    if len(data) == 0:
        return False
    matches = line.parse_line(data[len(data) - 1])
    data.pop()
    return matches

def take_maybe(line, data):
    if len(data) == 0:
        return False
    matches = line.parse_line(data[len(data) - 1])
    if matches != False: data.pop()
    return matches

#look through an array of data until you get a match (or run out of data)
def until(line, data):
    while len(data) > 0:
        matches = line.parse_line(data[len(data) - 1])
        data.pop()
        if matches != False:
            return matches
    return False

#iterate through lines while they match (and there's data)
def take_while(lines, data):
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
            break
    return res
