/* This file lists the files to be fetched from the node repo
 * in the /lib/ directory which will be placed in the ../lib/
 * directory after having each of the "replacements" in the
 * array for that file applied to it. The replacements are
 * simply the arguments to String#replace, so they can be
 * strings, regexes, functions.
 */

module.exports['string_decoder.js'] = [

    // pull in Bufer as a require
    [
        /^(\/\/ USE OR OTHER DEALINGS IN THE SOFTWARE\.)/m
      , '$1\n\nvar Buffer = require(\'buffer\').Buffer;'
    ]

]

module.exports['string_decoder.js'].out = 'index.js'