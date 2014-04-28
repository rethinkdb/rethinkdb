
def module(module):
    __import__(module, level=0)
    return sys.modules[module]

# non-printable ascii characters and invalid utf8 bytes
non_text_bytes = \
  range(0x00, 0x09+1) + [0x0B, 0x0C] + range(0x0F, 0x1F+1) + \
  [0xC0, 0xC1] + range(0xF5, 0xFF+1)

def guess_is_text_file(name):
    with file(name, 'rb') as f:
        data = f.read(100)
    for byte in data:
        if ord(byte) in non_text_bytes:
            return False
    return True
