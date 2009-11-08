
def get_arg(args, _arg):
    for arg in args:
        if arg[0] == _arg:
            return arg[1]
        
def set_arg(args, _arg, value):
    new_args = []
    arg_set = False
    for arg in args:
        new_val = arg[1]
        if arg[0] == _arg:
            new_val = value
            arg_set = True
        new_args.append((arg[0], new_val))
    if not arg_set:
        new_args.append((_arg, value))
    return new_args
        
def del_arg(args, _arg):
    new_args = []
    for arg in args:
        if not (arg[0] == _arg):
            new_args.append((arg[0], arg[1]))
    return new_args

