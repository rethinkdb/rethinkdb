__all__ = [
    'asm',
    'cpp',
    'exe',
    'html',
    'lib',
    'obj',
    'preprocessed',
    'rsp',
]

def register_all ():
    for i in __all__:
        m = __import__ (__name__ + '.' + i)
        reg = i + '.register ()'
        #exec (reg)

# TODO: (PF) I thought these would be imported automatically. Anyone knows why they aren't?
register_all ()
