
import string

def chr_or_question_mark( c ):
    if chr(c) in string.printable and c < 128 and c not in ( 0x09, 0x0b, 0x0c ):
        return chr(c)
    else:
        return '?'

char_translation_table = string.maketrans( 
      ''.join( map( chr, range(0, 256) ) )
    , ''.join( map( chr_or_question_mark, range(0, 256) ) )
    )
