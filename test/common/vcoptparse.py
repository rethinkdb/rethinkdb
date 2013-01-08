# Copyright 2010-2012 RethinkDB, all rights reserved.
"""
vcoptparse is short for Value-Centric Option Parser. It's a tiny argument parsing library. It has
less features than optparse or argparse, but it kicks more ass.

optparse and argparse allow the client to specify the flags that should be parsed, and as an
afterthought specify what keys should appear in the options dictionary when the parse is over.
vcoptparse works the other way around: you specify the keys and how to determine the keys from the
command line. That's why it's called "value-centric".

Here is a simple example:
>>> op = OptParser()
>>> op["verbose"] = BoolFlag("--verbose")
>>> op["count"] = IntFlag("--count", 5)   # Default count is 5
>>> op["infiles"] = ManyPositionalArgs()
>>> op.parse(["foo.py", "--count", "5", "file1.txt", "file2.txt"])
{'count': 5, 'verbose': False, 'infiles': ['file1.txt', 'file2.txt']}
"""



class NoValueClass(object):
    pass
NoValue = NoValueClass()

class Arg(object):
    pass

class OptError(StandardError):
    pass
    
class OptParser(object):

    def __init__(self):
        self.parsers_by_key = {}
        self.parsers_in_order = []
    
    def __setitem__(self, key, parser):
        assert isinstance(parser, Arg)
        if key in self.parsers_by_key: del self[key]
        assert parser not in self.parsers_by_key.values()
        self.parsers_by_key[key] = parser
        self.parsers_in_order.append((key, parser))
    
    def __getitem__(self, key):
        return self.parsers_by_key[key]
    
    def __delitem__(self, key):
        self.parsers_in_order.remove((key, self.parsers_by_key[key]))
        del self.parsers_by_key[key]
        
    def parse(self, args):
    
        args = args[1:]   # Cut off name of program
        
        values = dict((key, NoValue) for key in self.parsers_by_key.keys())
        
        def name_for_key(key):
            return getattr(self.parsers_by_key[key], "name", key)
        
        def set_value(key, new_value):
            combiner = getattr(self.parsers_by_key[key], "combiner", enforce_one_combiner)
            try:
                values[key] = combiner(values[key], new_value)
            except OptError, e:
                raise OptError(str(e) % {"name": name_for_key(key)})
        
        # Build flag table
        
        flags = {}
        for (key, parser) in self.parsers_in_order:
            if hasattr(parser, "flags"):
                for flag in parser.flags:
                    assert flag.startswith("-")
                    if flag in flags:
                        raise ValueError("The flag %r has two different meanings." % flag)
                    flags[flag] = (key, parser)
        
        # Handle flag arguments and store positional arguments
        
        positionals = []
        
        while args:
            arg = args.pop(0)
            if arg.startswith("-"):
                if arg in flags:
                    key, parser = flags[arg]
                    set_value(key, parser.flag(arg, args))
                else:
                    raise OptError("Don't know how to handle flag %r" % arg)
            else:
                positionals.append(arg)
        
        # Handle positional arguments
        
        for (key, parser) in self.parsers_in_order:
            if hasattr(parser, "positional"):
                set_value(key, parser.positional(positionals))
        
        if positionals:
            raise OptError("Unexpected extra positional argument(s): %s" % \
                ", ".join(repr(x) for x in positionals))
        
        # Apply defaults
        
        for (key, parser) in self.parsers_by_key.iteritems():
            if values[key] is NoValue:
                if hasattr(parser, "default") and parser.default is not NoValue:
                    values[key] = parser.default
                else:
                    raise OptError("You need to specify a value for %r" % name_for_key(key))
        
        return values



# Combiners (indicate how to combine repeat specifications of the same flag)

def most_recent_combiner(old, new):
    return new

def enforce_one_combiner(old, new):
    if old is not NoValue:
        raise OptError("%(name)r should only be specified once.")
    return new

def append_combiner(old, new):
    if old is NoValue: old = []
    return old + [new]



# Converters (indicate how to convert from string arguments to values)

def bool_converter(x):
    if x.lower() in ["yes", "true", "y", "t"]: return True
    elif x.lower() in ["no", "false", "n", "f"]: return False
    else: raise OptError("Expected a yes/no value. Got %r." % x)

def int_converter(x):
    try: return int(x)
    except ValueError: raise OptError("Expected an integer. Got %r." % x)

def float_converter(x):
    try: return float(x)
    except ValueError: raise OptError("Expected a float. Got %r." % x)

def choice_converter(choices):
    def check(x):
        if x in choices: return x
        else: raise OptError("Expected one of %s. Got %r." % (", ".join(choices), x))
    return check



# Standard argument parsers for common situations

class BoolFlag(Arg):
    def __init__(self, arg, invert=False):
        assert isinstance(invert, bool)
        self.flags = [arg]
        self.default = invert
    def flag(self, flag, args):
        return not self.default

class ChoiceFlags(Arg):
    def __init__(self, choices, default = NoValue):
        assert all(isinstance(x, str) for x in choices)
        self.flags = choices
        self.default = default
    def flag(self, flag, args):
        return flag.lstrip("-")



class ValueFlag(Arg):
    def __init__(self, name, converter = str, default = NoValue, combiner = enforce_one_combiner):
        assert isinstance(name, str)
        assert callable(converter)
        assert callable(combiner)
        self.flags = [name]
        self.converter = converter
        self.combiner = combiner
        self.default = default
    def flag(self, flag, args):
        try: value = args.pop(0)
        except IndexError:
            raise OptError("Flag %r expects an argument." % flag)
        try: value2 = self.converter(value)
        except OptError, e:
            raise OptError("Problem in argument to flag %r: %s" % (flag, e))
        return value2

class StringFlag(ValueFlag):
    def __init__(self, name, default = NoValue):
        ValueFlag.__init__(self, name, str, default = default)

class IntFlag(ValueFlag):
    def __init__(self, name, default = NoValue):
        ValueFlag.__init__(self, name, int_converter, default = default)

class FloatFlag(ValueFlag):
    def __init__(self, name, default = NoValue):
        ValueFlag.__init__(self, name, float_converter, default = default)

class ChoiceFlag(ValueFlag):
    def __init__(self, name, choices, default = NoValue):
        ValueFlag.__init__(self, name, choice_converter(choices), default = default)



class MultiValueFlag(Arg):
    def __init__(self, name, converters = [str], default = NoValue, combiner = enforce_one_combiner):
        assert isinstance(name, str)
        assert all(callable(x) for x in converters)
        assert callable(combiner)
        self.flags = [name]
        self.converters = converters
        self.combiner = combiner
        self.default = default
    def flag(self, flag, args):
        new_values = ()
        args_gotten = 0
        for converter in self.converters:
            try: value = args.pop(0)
            except IndexError:
                raise OptError("Flag %r expects %d argument(s), but only got %d." % \
                    (flag, len(self.converters), args_gotten))
            try: value2 = converter(value)
            except OptError, e:
                raise OptError("Problem in argument %d to flag %r: %s" % (args_gotten + 1, flag, e))
            new_values += (value2, )
            args_gotten += 1
        return new_values



class AllArgsAfterFlag(Arg):
    def __init__(self, name, converter = str, default = NoValue):
        assert isinstance(name, str)
        assert callable(converter)
        self.flags = [name]
        self.converter = converter
        self.default = default
    def flag(self, flag, args):
        args2 = []
        for arg in args:
            try: args2.append(self.converter(arg))
            except OptError, e: raise OptError("For %r: %s" % (flag, e))
        del args[:]   # We consume all arguments remaining
        return args2



class PositionalArg(Arg):
    def __init__(self, name = None, converter = str, default = NoValue):
        assert callable(converter)
        self.name = name
        self.converter = converter
        self.default = default
    def positional(self, args):
        try: value = args.pop(0)
        except IndexError:
            if self.default is NoValue:
                if self.name is None:
                    raise OptError("Too few positional arguments.")
                else:
                    raise OptError("Too few positional arguments; need a value for %r." % self.name)
            else:
                return NoValue
        try: value2 = self.converter(value)
        except OptError, e:
            if self.name is None: raise
            else: raise OptError("For %r: %s" % (self.name, e))
        return value2

class ManyPositionalArgs(Arg):
    def __init__(self, name = None, converter = str):
        assert callable(converter)
        self.name = name
        self.converter = converter
    def positional(self, args):
        args2 = []
        for arg in args:
            try: args2.append(self.converter(arg))
            except OptError, e:
                if self.name is None: raise
                else: raise OptError("For %r: %s" % (self.name, e))
        del args[:]   # We consume all arguments remaining
        return args2

