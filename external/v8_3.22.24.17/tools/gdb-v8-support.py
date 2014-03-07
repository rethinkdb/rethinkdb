# Copyright 2011 the V8 project authors. All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Google Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


kSmiTag = 0
kSmiTagSize = 1
kSmiTagMask = (1 << kSmiTagSize) - 1


kHeapObjectTag = 1
kHeapObjectTagSize = 2
kHeapObjectTagMask = (1 << kHeapObjectTagSize) - 1


kFailureTag = 3
kFailureTagSize = 2
kFailureTagMask = (1 << kFailureTagSize) - 1


kSmiShiftSize32 = 0
kSmiValueSize32 = 31
kSmiShiftBits32 = kSmiTagSize + kSmiShiftSize32


kSmiShiftSize64 = 31
kSmiValueSize64 = 32
kSmiShiftBits64 = kSmiTagSize + kSmiShiftSize64


kAllBits = 0xFFFFFFFF
kTopBit32 = 0x80000000
kTopBit64 = 0x8000000000000000


t_u32 = gdb.lookup_type('unsigned int')
t_u64 = gdb.lookup_type('unsigned long long')


def has_smi_tag(v):
  return v & kSmiTagMask == kSmiTag


def has_failure_tag(v):
  return v & kFailureTagMask == kFailureTag


def has_heap_object_tag(v):
  return v & kHeapObjectTagMask == kHeapObjectTag


def raw_heap_object(v):
  return v - kHeapObjectTag


def smi_to_int_32(v):
  v = v & kAllBits
  if (v & kTopBit32) == kTopBit32:
    return ((v & kAllBits) >> kSmiShiftBits32) - 2147483648
  else:
    return (v & kAllBits) >> kSmiShiftBits32


def smi_to_int_64(v):
  return (v >> kSmiShiftBits64)


def decode_v8_value(v, bitness):
  base_str = 'v8[%x]' % v
  if has_smi_tag(v):
    if bitness == 32:
      return base_str + (" SMI(%d)" % smi_to_int_32(v))
    else:
      return base_str + (" SMI(%d)" % smi_to_int_64(v))
  elif has_failure_tag(v):
    return base_str + " (failure)"
  elif has_heap_object_tag(v):
    return base_str + (" H(0x%x)" % raw_heap_object(v))
  else:
    return base_str


class V8ValuePrinter(object):
  "Print a v8value."
  def __init__(self, val):
    self.val = val
  def to_string(self):
    if self.val.type.sizeof == 4:
      v_u32 = self.val.cast(t_u32)
      return decode_v8_value(int(v_u32), 32)
    elif self.val.type.sizeof == 8:
      v_u64 = self.val.cast(t_u64)
      return decode_v8_value(int(v_u64), 64)
    else:
      return 'v8value?'
  def display_hint(self):
    return 'v8value'


def v8_pretty_printers(val):
  lookup_tag = val.type.tag
  if lookup_tag == None:
    return None
  elif lookup_tag == 'v8value':
    return V8ValuePrinter(val)
  return None
gdb.pretty_printers.append(v8_pretty_printers)


def v8_to_int(v):
  if v.type.sizeof == 4:
    return int(v.cast(t_u32))
  elif v.type.sizeof == 8:
    return int(v.cast(t_u64))
  else:
    return '?'


def v8_get_value(vstring):
  v = gdb.parse_and_eval(vstring)
  return v8_to_int(v)


class V8PrintObject (gdb.Command):
  """Prints a v8 object."""
  def __init__ (self):
    super (V8PrintObject, self).__init__ ("v8print", gdb.COMMAND_DATA)
  def invoke (self, arg, from_tty):
    v = v8_get_value(arg)
    gdb.execute('call __gdb_print_v8_object(%d)' % v)
V8PrintObject()
