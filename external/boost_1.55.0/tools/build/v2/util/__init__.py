
import bjam
import re
import types

# Decorator the specifies bjam-side prototype for a Python function
def bjam_signature(s):

    def wrap(f):       
        f.bjam_signature = s
        return f

    return wrap

def metatarget(f):

    f.bjam_signature = (["name"], ["sources", "*"], ["requirements", "*"],
                        ["default_build", "*"], ["usage_requirements", "*"])
    return f

class cached(object):

    def __init__(self, function):
        self.function = function
        self.cache = {}

    def __call__(self, *args):
        try:
            return self.cache[args]
        except KeyError:
            v = self.function(*args)
            self.cache[args] = v
            return v

    def __get__(self, instance, type):
        return types.MethodType(self, instance, type)

def unquote(s):
    if s and s[0] == '"' and s[-1] == '"':
        return s[1:-1]
    else:
        return s

_extract_jamfile_and_rule = re.compile("(Jamfile<.*>)%(.*)")

def qualify_jam_action(action_name, context_module):

    if action_name.startswith("###"):
        # Callable exported from Python. Don't touch
        return action_name
    elif _extract_jamfile_and_rule.match(action_name):
        # Rule is already in indirect format
        return action_name
    else:
        ix = action_name.find('.')
        if ix != -1 and action_name[:ix] == context_module:
            return context_module + '%' + action_name[ix+1:]
        
        return context_module + '%' + action_name        
    

def set_jam_action(name, *args):

    m = _extract_jamfile_and_rule.match(name)
    if m:
        args = ("set-update-action-in-module", m.group(1), m.group(2)) + args
    else:
        args = ("set-update-action", name) + args

    return bjam.call(*args)


def call_jam_function(name, *args):

    m = _extract_jamfile_and_rule.match(name)
    if m:
        args = ("call-in-module", m.group(1), m.group(2)) + args
        return bjam.call(*args)
    else:
        return bjam.call(*((name,) + args))

__value_id = 0
__python_to_jam = {}
__jam_to_python = {}

def value_to_jam(value, methods=False):
    """Makes a token to refer to a Python value inside Jam language code.

    The token is merely a string that can be passed around in Jam code and
    eventually passed back. For example, we might want to pass PropertySet
    instance to a tag function and it might eventually call back
    to virtual_target.add_suffix_and_prefix, passing the same instance.

    For values that are classes, we'll also make class methods callable
    from Jam.

    Note that this is necessary to make a bit more of existing Jamfiles work.
    This trick should not be used to much, or else the performance benefits of
    Python port will be eaten.
    """

    global __value_id

    r = __python_to_jam.get(value, None)
    if r:
        return r

    exported_name = '###_' + str(__value_id)
    __value_id = __value_id + 1
    __python_to_jam[value] = exported_name
    __jam_to_python[exported_name] = value

    if methods and type(value) == types.InstanceType:
        for field_name in dir(value):
            field = getattr(value, field_name)
            if callable(field) and not field_name.startswith("__"):
                bjam.import_rule("", exported_name + "." + field_name, field)

    return exported_name

def record_jam_to_value_mapping(jam_value, python_value):
    __jam_to_python[jam_value] = python_value

def jam_to_value_maybe(jam_value):

    if type(jam_value) == type(""):
        return __jam_to_python.get(jam_value, jam_value)
    else:
        return jam_value

def stem(filename):
    i = filename.find('.')
    if i != -1:
        return filename[0:i]
    else:
        return filename
