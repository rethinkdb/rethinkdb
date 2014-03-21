/*
 Copyright 2013 Daniel Wirtz <dcode@dcode.io>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS-IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/**
 * Long.js Pretty Simple Test Suite.
 */
var Long = require(__dirname+"/../Long.min.js");

var suite = {
    
    "basic": function(test) {
        var longVal = new Long(0xFFFFFFFF, 0x7FFFFFFF);
        test.equal(longVal.toNumber(), 9223372036854775807);
        test.equal(longVal.toString(), "9223372036854775807");
        test.done();
    },
    
    // Let's assume signed goog.math.Long works as expected because the people at Google are pretty smart.
    
    // So let's focus on my probably not-so-smart extensions:
    
    "from28Bits": function(test) {
        var val = Long.from28Bits(0xFFFFFFF, 0xFFFFFFF, 0xFF);
        test.equal(val.toString(), "-1");
        test.done();
    },
    
    "unsigned": {
        
        "min/max": function(test) {
            test.equal(Long.MIN_SIGNED_VALUE.toString(), "-9223372036854775808");
            test.equal(Long.MAX_SIGNED_VALUE.toString(), "9223372036854775807");
            test.equal(Long.MIN_UNSIGNED_VALUE.toString(), "0");
            test.equal(Long.MAX_UNSIGNED_VALUE.toString(), "18446744073709551615");
            test.done();
        },
        
        "construct_negint": function(test) {
            var longVal = Long.fromInt(-1, true);
            test.equal(longVal.low, -1);
            test.equal(longVal.high, -1);
            test.equal(longVal.unsigned, true);
            test.equal(longVal.toNumber(), 18446744073709551615);
            test.equal(longVal.toString(), "18446744073709551615");
            test.done();
        },
        
        "construct_highlow": function(test) {
            var longVal = new Long(0xFFFFFFFF, 0xFFFFFFFF, true);
            test.equal(longVal.low, -1);
            test.equal(longVal.high, -1);
            test.equal(longVal.unsigned, true);
            test.equal(longVal.toNumber(), 18446744073709551615);
            test.equal(longVal.toString(), "18446744073709551615");
            test.done();
        },
        
        "construct_number": function(test) {
            var longVal = Long.fromNumber(0xFFFFFFFFFFFFFFFF, true);
            test.equal(longVal.low, -1);
            test.equal(longVal.high, -1);
            test.equal(longVal.unsigned, true);
            test.equal(longVal.toNumber(), 18446744073709551615);
            test.equal(longVal.toString(), "18446744073709551615");
            test.done();
        },

        "toSigned/Unsigned": function(test) {
            var longVal = Long.fromNumber(-1, false);
            test.equal(longVal.toNumber(), -1);
            longVal = longVal.toUnsigned();
            test.equal(longVal.toNumber(), 0xFFFFFFFFFFFFFFFF);
            longVal = longVal.toSigned();
            test.equal(longVal.toNumber(), -1);
            test.done();
        },
        
        "max_unsigned_sub_max_signed": function(test) {
            var longVal = Long.MAX_UNSIGNED_VALUE.subtract(Long.MAX_SIGNED_VALUE).subtract(Long.ONE);
            test.equal(longVal.toNumber(), Long.MAX_SIGNED_VALUE);
            test.equal(longVal.toString(), Long.MAX_SIGNED_VALUE.toString());
            test.done();
        },
        
        "max_sub_max": function(test) {
            var longVal = Long.MAX_UNSIGNED_VALUE.subtract(Long.MAX_UNSIGNED_VALUE);
            test.equal(longVal, 0);
            test.equal(longVal.low, 0);
            test.equal(longVal.high, 0);
            test.equal(longVal.unsigned, true);
            test.equal(longVal.toNumber(), 0);
            test.equal(longVal.toString(), "0");
            test.done();
        },
        
        "zero_sub_signed": function(test) {
            var longVal = Long.fromInt(0, true).add(Long.fromInt(-1, false));
            test.equal(longVal.low, -1);
            test.equal(longVal.high, -1);
            test.equal(longVal.unsigned, true);
            test.equal(longVal.toNumber(), 18446744073709551615);
            test.equal(longVal.toString(), "18446744073709551615");
            test.done();
        },
        
        "max_unsigned_div_max_signed": function(test) {
            var longVal = Long.MAX_UNSIGNED_VALUE.div(Long.MAX_SIGNED_VALUE);
            test.equal(longVal.toNumber(), 2);
            test.equal(longVal.toString(), "2");
            test.done();
        },
        
        "max_unsigned_div_neg_signed": function(test) {
            var longVal = Long.MAX_UNSIGNED_VALUE.div(Long.fromInt(-2));
            test.equal(longVal.toNumber(), -Long.MAX_SIGNED_VALUE);
            test.done();
        }
    },

    // FIXME: There is no support for NaN or +/-Infinity
    // "NaN": function(test) {
    //     test.ok(isNan(Long.fromNumber(NaN).toNumber());
    //     test.strictEqual(Long.fromNumber(+Infinity).toNumber(), +Infinity);
    //     test.strictEqual(Long.fromNumber(-Infinity).toNumber(), -Infinity);
    //     test.done();
    // }
    // One option could be to store NaN, Infinity etc. in high and set low to 0, while making sure to convert it in a
    // proper way as soon as any math is called upon it. This might be as simple as that each of these values always
    // "infects" other values, thus remaining untouched respectively changing the base long to its value.
    //
    // To clarify that, it all becomes zero atm, which usually is good enough but not perfect:
    "NaN": function(test) {
        test.strictEqual(Long.fromNumber(NaN), Long.ZERO);
        test.strictEqual(Long.fromNumber(+Infinity), Long.ZERO);
        test.strictEqual(Long.fromNumber(-Infinity), Long.ZERO);
        test.strictEqual(Long.fromString(NaN+""), Long.ZERO);
        test.strictEqual(Long.fromString(+Infinity+""), Long.ZERO);
        test.strictEqual(Long.fromString(-Infinity+""), Long.ZERO);
        test.done();
    }
};

module.exports = suite;
