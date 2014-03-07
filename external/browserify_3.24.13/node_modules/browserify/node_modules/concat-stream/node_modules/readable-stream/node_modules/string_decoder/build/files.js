/* This file lists the files to be fetched from the node repo
 * in the /lib/ directory which will be placed in the ../lib/
 * directory after having each of the "replacements" in the
 * array for that file applied to it. The replacements are
 * simply the arguments to String#replace, so they can be
 * strings, regexes, functions.
 */

module.exports['string_decoder.js'] = [

    // pull in Bufer as a require
    // add Buffer.isEncoding where missing
    [
        /^(\/\/ USE OR OTHER DEALINGS IN THE SOFTWARE\.)/m
      ,   '$1\n\nvar Buffer = require(\'buffer\').Buffer;'
        + '\n'
        + '\nvar isBufferEncoding = Buffer.isEncoding'
        + '\n  || function(encoding) {'
        + '\n       switch (encoding && encoding.toLowerCase()) {'
        + '\n         case \'hex\': case \'utf8\': case \'utf-8\': case \'ascii\': case \'binary\': case \'base64\': case \'ucs2\': case \'ucs-2\': case \'utf16le\': case \'utf-16le\': case \'raw\': return true;'
        + '\n         default: return false;'
        + '\n       }'
        + '\n     }'
        + '\n'

    ]

    // use custom Buffer.isEncoding reference
  , [
        /Buffer\.isEncoding\(/g
      , 'isBufferEncoding\('
    ]

]

module.exports['string_decoder.js'].out = 'index.js'