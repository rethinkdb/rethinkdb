
def get_arg(args, _arg):
    for arg in args:
        if arg[0] == _arg:
            return arg[1]
        
def set_arg(args, _arg, value):
    new_args = []
    for arg in args:
        new_val = arg[1]
        if arg[0] == _arg:
            new_val = value
        new_args.append((arg[0], new_val))
    return new_args
        
