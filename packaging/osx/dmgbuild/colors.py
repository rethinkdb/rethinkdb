# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import re
import math

class Color (object):
    def to_rgb(self):
        raise Exception('Must implement to_rgb() in subclasses')

class RGB (Color):
    def __init__(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b

    def to_rgb(self):
        return self
    
class HSL (Color):
    def __init__(self, h, s, l):
        self.h = h
        self.s = s
        self.l = l

    @staticmethod
    def _hue_to_rgb(t1, t2, hue):
        if hue < 0:
            hue += 6
        elif hue >= 6:
            hue -= 6

        if hue < 1:
            return (t2 - t1) * hue + t1
        elif hue < 3:
            return t2
        elif hue < 4:
            return (t2 - t1) * (4 - hue) + t1
        else:
            return t1
        
    def to_rgb(self):
        hue = self.h / 60.0
        if self.l <= 0.5:
            t2 = self.l * (self.s + 1)
        else:
            t2 = self.l + self.s - (self.l * self.s)
        t1 = self.l * 2 - t2
        r = self._hue_to_rgb(t1, t2, hue + 2)
        g = self._hue_to_rgb(t1, t2, hue)
        b = self._hue_to_rgb(t1, t2, hue - 2)
        return RGB(r, g, b)
    
class HWB (Color):
    def __init__(self, h, w, b):
        self.h = h
        self.w = w
        self.b = b

    @staticmethod
    def _hue_to_rgb(hue):
        if hue < 0:
            hue += 6
        elif hue >= 6:
            hue -= 6

        if hue < 1:
            return hue
        elif hue < 3:
            return 1
        elif hue < 4:
            return (4 - hue)
        else:
            return 0
 
    def to_rgb(self):
        hue = self.h / 60.0
        t1 = 1 - self.w - self.b
        r = self._hue_to_rgb(hue + 2) * t1 + self.w
        g = self._hue_to_rgb(hue) * t1 + self.w
        b = self._hue_to_rgb(hue - 2) * t1 + self.w
        return RGB(r, g, b)
    
class CMYK (Color):
    def __init__(self, c, m, y, k):
        self.c = c
        self.m = m
        self.y = y
        self.k = k

    def to_rgb(self):
        r = 1.0 - min(1.0, self.c + self.k)
        g = 1.0 - min(1.0, self.m + self.k)
        b = 1.0 - min(1.0, self.y + self.k)
        return RGB(r, g, b)
    
class Gray (Color):
    def __init__(self, g):
        self.g = g

    def to_rgb(self):
        return RGB(g, g, g)
    
_x11_colors = {
	'aliceblue': (240, 248, 255),
    'antiquewhite': (250, 235, 215),
    'aqua':	( 0, 255, 255),
    'aquamarine': (127, 255, 212),
    'azure': (240, 255, 255),
    'beige': (245, 245, 220),
    'bisque': (255, 228, 196),
    'black': ( 0, 0, 0),
    'blanchedalmond': (255, 235, 205),
    'blue': ( 0, 0, 255),
    'blueviolet': (138, 43, 226),
    'brown': (165, 42, 42),
    'burlywood': (222, 184, 135),
    'cadetblue': ( 95, 158, 160),
    'chartreuse': (127, 255, 0),
    'chocolate': (210, 105, 30),
    'coral': (255, 127, 80),
    'cornflowerblue': (100, 149, 237),
    'cornsilk': (255, 248, 220),
    'crimson': (220, 20, 60),
    'cyan': ( 0, 255, 255),
    'darkblue': ( 0, 0, 139),
    'darkcyan': ( 0, 139, 139),
    'darkgoldenrod': (184, 134, 11),
    'darkgray': (169, 169, 169),
    'darkgreen': ( 0, 100, 0),
    'darkgrey': (169, 169, 169),
    'darkkhaki': (189, 183, 107),
    'darkmagenta': (139, 0, 139),
    'darkolivegreen': ( 85, 107, 47),
    'darkorange': (255, 140, 0),
    'darkorchid': (153, 50, 204),
    'darkred': (139, 0, 0),
    'darksalmon': (233, 150, 122),
    'darkseagreen': (143, 188, 143),
    'darkslateblue': ( 72, 61, 139),
    'darkslategray': ( 47, 79, 79),
    'darkslategrey': ( 47, 79, 79),
    'darkturquoise': ( 0, 206, 209),
    'darkviolet': (148, 0, 211),
    'deeppink': (255, 20, 147),
    'deepskyblue': ( 0, 191, 255),
    'dimgray': (105, 105, 105),
    'dimgrey': (105, 105, 105),
    'dodgerblue': ( 30, 144, 255),
    'firebrick': (178, 34, 34),
    'floralwhite': (255, 250, 240),
    'forestgreen': ( 34, 139, 34),
    'fuchsia': (255, 0, 255),
    'gainsboro': (220, 220, 220),
    'ghostwhite': (248, 248, 255),
    'gold': (255, 215, 0),
    'goldenrod': (218, 165, 32),
    'gray': (128, 128, 128),
    'grey': (128, 128, 128),
    'green': ( 0, 128, 0),
    'greenyellow': (173, 255, 47),
    'honeydew': (240, 255, 240),
    'hotpink': (255, 105, 180),
    'indianred': (205, 92, 92),
    'indigo': ( 75, 0, 130),
    'ivory': (255, 255, 240),
    'khaki': (240, 230, 140),
    'lavender': (230, 230, 250),
    'lavenderblush': (255, 240, 245),
    'lawngreen': (124, 252, 0),
    'lemonchiffon': (255, 250, 205),
    'lightblue': (173, 216, 230),
    'lightcoral': (240, 128, 128),
    'lightcyan': (224, 255, 255),
    'lightgoldenrodyellow': (250, 250, 210),
    'lightgray': (211, 211, 211),
    'lightgreen': (144, 238, 144),
    'lightgrey': (211, 211, 211),
    'lightpink': (255, 182, 193),
    'lightsalmon': (255, 160, 122),
    'lightseagreen': ( 32, 178, 170),
    'lightskyblue': (135, 206, 250),
    'lightslategray': (119, 136, 153),
    'lightslategrey': (119, 136, 153),
    'lightsteelblue': (176, 196, 222),
    'lightyellow': (255, 255, 224),
    'lime': ( 0, 255, 0),
    'limegreen': ( 50, 205, 50),
    'linen': (250, 240, 230),
    'magenta': (255, 0, 255),
    'maroon': (128, 0, 0),
    'mediumaquamarine': (102, 205, 170),
    'mediumblue': ( 0, 0, 205),
    'mediumorchid': (186, 85, 211),
    'mediumpurple': (147, 112, 219),
    'mediumseagreen': ( 60, 179, 113),
    'mediumslateblue': (123, 104, 238),
    'mediumspringgreen': ( 0, 250, 154),
    'mediumturquoise': ( 72, 209, 204),
    'mediumvioletred': (199, 21, 133),
    'midnightblue': ( 25, 25, 112),
    'mintcream': (245, 255, 250),
    'mistyrose': (255, 228, 225),
    'moccasin': (255, 228, 181),
    'navajowhite': (255, 222, 173),
    'navy': ( 0, 0, 128),
    'oldlace': (253, 245, 230),
    'olive': (128, 128, 0),
    'olivedrab': (107, 142, 35),
    'orange': (255, 165, 0),
    'orangered': (255, 69, 0),
    'orchid': (218, 112, 214),
    'palegoldenrod': (238, 232, 170),
    'palegreen': (152, 251, 152),
    'paleturquoise': (175, 238, 238),
    'palevioletred': (219, 112, 147),
    'papayawhip': (255, 239, 213),
    'peachpuff': (255, 218, 185),
    'peru': (205, 133, 63),
    'pink': (255, 192, 203),
    'plum': (221, 160, 221),
    'powderblue': (176, 224, 230),
    'purple': (128, 0, 128),
    'red': (255, 0, 0),
    'rosybrown': (188, 143, 143),
    'royalblue': ( 65, 105, 225),
    'saddlebrown': (139, 69, 19),
    'salmon': (250, 128, 114),
    'sandybrown': (244, 164, 96),
    'seagreen': ( 46, 139, 87),
    'seashell': (255, 245, 238),
    'sienna': (160, 82, 45),
    'silver': (192, 192, 192),
    'skyblue': (135, 206, 235),
    'slateblue': (106, 90, 205),
    'slategray': (112, 128, 144),
    'slategrey': (112, 128, 144),
    'snow': (255, 250, 250),
    'springgreen': ( 0, 255, 127),
    'steelblue': ( 70, 130, 180),
    'tan': (210, 180, 140),
    'teal': ( 0, 128, 128),
    'thistle': (216, 191, 216),
    'tomato': (255, 99, 71),
    'turquoise': ( 64, 224, 208),
    'violet': (238, 130, 238),
    'wheat': (245, 222, 179),
    'white': (255, 255, 255),
    'whitesmoke': (245, 245, 245),
    'yellow': (255, 255, 0),
    'yellowgreen': (154, 205, 50)
    }
    
_ws_re = re.compile('\s+')
_token_re = re.compile('[A-Za-z_][A-Za-z0-9_]*')
_hex_re = re.compile('#([0-9a-f]{3}(?:[0-9a-f]{3}))')
_number_re = re.compile('[0-9]*(\.[0-9]*)')

class ColorParser (object):
    def __init__(self, s):
        self._string = s
        self._pos = 0

    def skipws(self):
        m = _ws_re.match(self._string, self._pos)
        if m:
            self._pos = m.end(0)
        
    def expect(self, s, context=''):
        if len(self._string) - self._pos < len(s) \
          or self._string[self._pos:self._pos + len(s)] != s:
            raise ValueError('bad color "%s" - expected "%s"%s'
                            % (self._string, s, context))
        self._pos += len(s)

    def expectEnd(self):
        if self._pos != len(self._string):
            raise ValueError('junk at end of color "%s"' % self._string)

    def getToken(self):
        m = _token_re.match(self._string, self._pos)
        if m:
            token = m.group(0)

            self._pos = m.end(0)
            return token
        return None
        
    def parseNumber(self, context=''):
        m = _number_re.match(self._string, self._pos)
        if m:
            self._pos = m.end(0)
            return float(m.group(0))
        raise ValueError('bad color "%s" - expected a number%s'
                         % (self._string, context))
    
    def parseColor(self):
        self.skipws()

        token = self.getToken()
        if token:
            if token == 'rgb':
                return self.parseRGB()
            elif token == 'hsl':
                return self.parseHSL()
            elif token == 'hwb':
                return self.parseHWB()
            elif token == 'cmyk':
                return self.parseCMYK()
            elif token == 'gray' or token == 'grey':
                return self.parseGray()

            try:
                r, g, b = _x11_colors[token]
            except KeyError:
                raise ValueError('unknown color name "%s"' % token)

            self.expectEnd()

            return RGB(r / 255.0, g / 255.0, b / 255.0)
        
        m = _hex_re.match(self._string, self._pos)
        if m:
            hrgb = m.group(1)

            if len(hrgb) == 3:
                r = int('0x' + 2 * hrgb[0])
                g = int('0x' + 2 * hrgb[1])
                b = int('0x' + 2 * hrgb[2])
            else:
                r = int('0x' + hrgb[0:2])
                g = int('0x' + hrgb[2:4])
                b = int('0x' + hrgb[4:6])

            self._pos = m.end(0)
            self.skipws()

            self.expectEnd()

            return RGB(r / 255.0, g / 255.0, b / 255.0)

        raise ValueError('bad color syntax "%s"' % self._string)

    def parseRGB(self):
        self.expect('(', 'after "rgb"')
        self.skipws()

        r = self.parseValue()

        self.skipws()
        self.expect(',', 'in "rgb"')
        self.skipws()

        g = self.parseValue()

        self.skipws()
        self.expect(',', 'in "rgb"')
        self.skipws()

        b = self.parseValue()

        self.skipws()
        self.expect(')', 'at end of "rgb"')

        self.skipws()
        self.expectEnd()

        return RGB(r, g, b)
        
    def parseHSL(self):
        self.expect('(', 'after "hsl"')
        self.skipws()

        h = self.parseAngle()

        self.skipws()
        self.expect(',', 'in "hsl"')
        self.skipws()

        s = self.parseValue()

        self.skipws()
        self.expect(',', 'in "hsl"')
        self.skipws()

        l = self.parseValue()

        self.skipws()
        self.expect(')', 'at end of "hsl"')

        self.skipws()
        self.expectEnd()

        return HSL(h, s, l)

    def parseHWB(self):
        self.expect('(', 'after "hwb"')
        self.skipws()

        h = self.parseAngle()

        self.skipws()
        self.expect(',', 'in "hwb"')
        self.skipws()

        w = self.parseValue()

        self.skipws()
        self.expect(',', 'in "hwb"')
        self.skipws()

        b = self.parseValue()

        self.skipws()
        self.expect(')', 'at end of "hwb"')

        self.skipws()
        self.expectEnd()

        return HWB(h, w, b)

    def parseCMYK(self):
        self.expect('(', 'after "cmyk"')
        self.skipws()

        c = self.parseValue()

        self.skipws()
        self.expect(',', 'in "cmyk"')
        self.skipws()

        m = self.parseValue()

        self.skipws()
        self.expect(',', 'in "cmyk"')
        self.skipws()

        y = self.parseValue()

        self.skipws()
        self.expect(',', 'in "cmyk"')
        self.skipws()

        k = self.parseValue()

        self.skipws()
        self.expect(')', 'at end of "cmyk"')

        self.skipws()
        self.expectEnd()

        return CMYK(c, m, y, k)

    def parseGray(self):
        self.expect('(', 'after "gray"')
        self.skipws()

        g = self.parseValue()

        self.skipws()
        self.expect(')', 'at end of "gray')

        self.skipws()
        self.expectEnd()

        return Gray(g)

    def parseValue(self):
        n = self.parseNumber()
        self.skipws()
        if self._string[self._pos] == '%':
            n = n / 100.0
            self.pos += 1
        return n
    
    def parseAngle(self):
        n = self.parseNumber()
        self.skipws()
        tok = self.getToken()
        if tok == 'rad':
            n = n * 180.0 / math.pi
        elif tok == 'grad' or tok == 'gon':
            n = n * 0.9
        elif tok != 'deg':
            raise ValueError('bad angle unit "%s"' % tok)
        return n

_color_re = re.compile('\s*(#|rgb|hsl|hwb|cmyk|gray|grey|%s)'
                       % '|'.join(_x11_colors.keys()))
def isAColor(s):
    return _color_re.match(s)

def parseColor(s):
    return ColorParser(s).parseColor()
