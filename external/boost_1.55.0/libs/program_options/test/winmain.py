
from StringIO import StringIO
import string

testcases = r"""1 -> 1
1"2" -> 12
1"2  -> 12
1"\"2" -> 1"2
"1" "2" -> 1, 2
1\" -> 1"
1\\" -> 1\  
1\\\" -> 1\"  
1\\\\" -> 1\\  
1" 1 -> 1 1
1\" 1 -> 1", 1
1\1 -> 1\1
1\\1 -> 1\\1
"""

#testcases = r"""1\\\\" -> 1\\
#"""

t = StringIO(testcases)

def quote(s):
    result = s.replace("\\", r"\\")
    result = result.replace("\"", "\\\"")
    return '"' + result + '"'


for s in t:
    s = string.strip(s)
    (value, result) = string.split(s, "->")
#    print value, result
    tokens = string.split(result, ",")
    value = quote(value)
    tokens = map(string.strip, tokens)
    tokens = map(quote, tokens)
    print "TEST(%s, {%s});" % (value, string.join(tokens, ","))
