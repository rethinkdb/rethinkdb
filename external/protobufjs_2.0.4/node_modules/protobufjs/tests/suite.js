/*
 Copyright 2013 Daniel Wirtz <dcode@dcode.io>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/**
 * ProtoBuf.js Test Suite.
 * @author Daniel Wirtz <dcode@dcode.io>
 */
(function(global) {

    var FILE = "ProtoBuf.js";
    var BROWSER = !!global.window;
    
    var ProtoBuf = BROWSER ? global.dcodeIO.ProtoBuf : require(__dirname+"/../"+FILE),
        ByteBuffer = BROWSER ? global.dcodeIO.ByteBuffer : ByteBuffer || require("bytebuffer"),
        util = BROWSER ? null : require("util"),
        fs = BROWSER ? null : require("fs");
        
        if (typeof __dirname == 'undefined') {
            __dirname = document.location.href.replace(/[\/\\][^\/\\]*$/, "");
        }
    
    /**
     * Constructs a new Sandbox for module loaders and shim testing.
     * @param {Object.<string,*>} properties Additional properties to set
     * @constructor
     */
    var Sandbox = function(properties) {
        this.ByteBuffer = function() {};
        for (var i in properties) {
            this[i] = properties[i];
        }
        this.console = {
            log: function(s) {
                console.log(s);
            }
        };
    };
    
    function fail(e) {
        throw(e);
    }
    
    /**
     * Validates the complexDotProto and complexInline tests.
     * @param {*} test Nodeunit test
     * @param {Object} Game Game namespace
     */
    function validateComplex(test, Game) {
        var Car = Game.Cars.Car,
            Vendor = Car.Vendor,
            Speed = Car.Speed;
    
        var vendor;
        // Car from class with argument list properties
        var car = new Car(
            "Rusty",
            // Vendor from class with object properties
            vendor = new Vendor({
                "name": "Iron Inc.",
                // Address from object
                "address": {
                    "country": "US"
                }
            }),
            // Speed from enum object
            Speed.SUPERFAST
        );
        test.equal(car.model, "Rusty");
        test.equal(car.vendor.name, "Iron Inc.");
        test.equal(car.vendor.address.country, "US");
        test.equal(car.vendor.address.country, car.getVendor().get_address().country);
        var bb = new ByteBuffer(28);
        car.encode(bb);
        test.equal(bb.toString("debug"), "<0A 05 52 75 73 74 79 12 11 0A 09 49 72 6F 6E 20 49 6E 63 2E 12 04 0A 02 55 53 18 02>");
        var carDec = Car.decode(bb);
        test.equal(carDec.model, "Rusty");
        test.equal(carDec.vendor.name, "Iron Inc.");
        test.equal(carDec.vendor.address.country, "US");
        test.equal(carDec.vendor.address.country, carDec.getVendor().get_address().country);
    }
    
    /**
     * Test suite.
     * @type {Object.<string,function>}
     */
    var suite = {
    
        "init": function(test) {
            test.ok(typeof ProtoBuf == "object");
            test.ok(typeof ProtoBuf.Reflect == 'object');
            test.ok(typeof ProtoBuf.protoFromFile == "function");
            test.done();
        },

        // Example "A Simple Message" from the protobuf docs
        // https://developers.google.com/protocol-buffers/docs/encoding#simple
        "example1": function(test) {
            try{
                var builder = ProtoBuf.loadProtoFile(__dirname+"/example1.proto");
                var Test1 = builder.build("Test1");
                test.ok(typeof Test1 == 'function');
                var inst = new Test1(150);
                test.ok(inst instanceof ProtoBuf.Builder.Message);
                test.equal(inst.a, 150);
                test.equal(inst.getA(), 150);
                test.equal(inst.get_a(), 150);
                inst.setA(151);
                test.equal(inst.a, 151);
                test.equal(inst.getA(), 151);
                test.equal(inst.get_a(), 151);
                inst.set_a(152);
                test.equal(inst.a, 152);
                test.equal(inst.toString(), ".Test1");
                test.throws(function() {
                    inst.setA(null); // required
                });
                test.throws(function() {
                    inst.setA([]);
                });
                var bb = new ByteBuffer(3);
                inst.encode(bb);
                test.equal(bb.toString("debug"), "<08 98 01>");
                var instDec = Test1.decode(bb);
                test.equal(instDec.a, 152);
                
            } catch (e) {
                fail(e);
            }
            test.done();
        },
    
        // Basically the same as example1, but with an unsigned value.
        "example1u": function(test) {
            try{
                var builder = ProtoBuf.loadProtoFile(__dirname+"/example1u.proto");
                var Test1u = builder.build("Test1u");
                test.ok(typeof Test1u == 'function');
                var inst = new Test1u(-1);
                test.equal(inst.a, 4294967295);
                var bb = new ByteBuffer(6);
                inst.encode(bb);
                test.equal(bb.toString("debug"), "<08 FF FF FF FF 7F>");
                var instDec = Test1u.decode(bb);
                test.equal(instDec.a, 4294967295);
                
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        // Example "Strings" from the protobuf docs
        // https://developers.google.com/protocol-buffers/docs/encoding#types
        "example2": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/example2.proto");
                var Test2 = builder.build("Test2");
                var inst = new Test2("testing");
                var bb = new ByteBuffer(9);
                inst.encode(bb);
                test.equal(bb.toString("debug"), "<12 07 74 65 73 74 69 6E 67>");
                var instDec = Test2.decode(bb);
                test.equal(instDec.b, "testing");
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        // Example "Embedded Messages" from the protobuf docs
        // https://developers.google.com/protocol-buffers/docs/encoding#embedded
        "example3": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/example3.proto");
                var root = builder.build();
                var Test1 = root.Test1;
                var Test3 = root.Test3;
                var inst = new Test3(new Test1(150));
                var bb = new ByteBuffer(5);
                test.equal(inst.c.a, 150);
                inst.encode(bb);
                test.equal(bb.toString("debug"), "<1A 03 08 96 01>");
                var instDec = Test3.decode(bb);
                test.equal(instDec.c.a, 150);
            } catch(e) {
                fail(e);
            }
            test.done();
        },
        
        "example4": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/example4.proto");
                var Test4 = builder.build("Test4");
                var inst = new Test4([3, 270, 86942]);
                var bb = new ByteBuffer(8);
                test.equal(inst.d.length, 3);
                inst.encode(bb);
                test.equal(bb.toString("debug"), "<22 06 03 8E 02 9E A7 05>");
                var instDec = Test4.decode(bb);
                test.equal(bb.toString("debug"), " 22 06 03 8E 02 9E A7 05|");
                test.equal(instDec.d.length, 3);
                test.equal(instDec.d[2], 86942);
            } catch(e) {
                fail(e);
            }
            test.done();
        },
    
        "numberFormats": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/numberformats.proto");
                var Formats = builder.build("Formats");
                test.strictEqual(Formats.DEC, 1);
                test.strictEqual(Formats.HEX, 31);
                test.strictEqual(Formats.OCT, 15);
                var Msg = builder.build("Msg");
                var msg = new Msg();
                test.strictEqual(msg.dec, -1);
                test.strictEqual(msg.hex, -31);
                test.strictEqual(msg.oct, -15);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
    
        // Check encode/decode against a table of known correct pairs.
        // Note that javascript ArrayBuffer does not support signed Zero or NaN
        // bertdouglas (https://github.com/bertdouglas)
        "float": function(test) {
            try {
                var str_proto = "message Float {"
                    + " required float f = 1;"
                    + "}";
                var builder = ProtoBuf.loadProto(str_proto);
                var root = builder.build();
                var Float = root.Float;
    
                var in_tolerance = function (reference,actual) {
                    var tol = 1e-6;
                    var scale = 1.0;
                    if (reference != 0.0 ) {
                        scale = reference;
                    };
                    var err = Math.abs(reference - actual)/scale;
                    return err < tol;
                };
    
                var f_vals = [
                    // hex values are shown here in big-endian following IEEE754 notation
                    // protobuf is little-endian
                    // { f: -0.0 , b: "80 00 00 00" },
                    { f: +0.0 , b: "00 00 00 00" },
                    { f: -1e-10 , b: "AE DB E6 FF" },
                    { f: +1e-10 , b: "2E DB E6 FF" },
                    { f: -2e+10 , b: "D0 95 02 F9" },
                    { f: +2e+10 , b: "50 95 02 F9" },
                    { f: -3e-30 , b: "8E 73 63 90" },
                    { f: +3e-30 , b: "0E 73 63 90" },
                    { f: -4e+30 , b: "F2 49 F2 CA" },
                    { f: +4e+30 , b: "72 49 F2 CA" },
                    { f: -123456789.0 , b: "CC EB 79 A3" },
                    { f: +123456789.0 , b: "4C EB 79 A3" },
                    { f: -0.987654321 , b: "BF 7C D6 EA" },
                    { f: +0.987654321 , b: "3F 7C D6 EA" },
                    { f: -Infinity , b: "FF 80 00 00" },
                    { f: +Infinity , b: "7F 80 00 00" }
                    // { f: -NaN , b: "FF C0 00 00>" },
                    // { f: +NaN , b: "7F C0 00 00" }
                ];
    
                f_vals.map( function(x) {
                    // check encode
                    var m1 = new Float();
                    var b1 = new ByteBuffer();
                    m1.f = x.f;
                    m1.encode(b1);
                    var q1 = b1.slice(1,5).compact().reverse();
                    test.strictEqual('<' + x.b + '>', q1.toString("debug"));
    
                    // check decode
                    var b2 = new ByteBuffer();
                    var s1 = x.b + ' 0D';
                    var s2 = s1.split(" ");
                    var s3 = s2.reverse();
                    var i1 = s3.map(function(y) { return parseInt(y,16) } );
                    i1.map(function(y) { b2.writeUint8(y) });
                    b2.length = b2.offset;
                    b2.offset = 0;
                    var m2 = Float.decode(b2);
    
                    var s4 = "" + x.f +" " + m2.f;
                    if ( isNaN(x.f) ) {
                        test.ok( isNaN(m2.f), s4 );
                    }
                    else if ( ! isFinite( x.f) ) {
                        test.ok( x.f === m2.f, s4 );
                    }
                    else {
                        test.ok( in_tolerance(x.f, m2.f), s4 );
                    }
                });
            } catch(e) {
                fail(e);
            }
            test.done();
        },
        
        "bytes": function(test) {
            try {
                var str_proto = "message Test { required bytes b = 1; }";
                var builder = ProtoBuf.loadProto(str_proto);
                var Test = builder.build("Test");
                var bb = new ByteBuffer(4).writeUint32(0x12345678).flip();
                var myTest = new Test(bb);
                test.strictEqual(myTest.b.array, bb.array);
                var bb2 = new ByteBuffer(6);
                myTest.encode(bb2);
                test.equal(bb2.toString("debug"), "<0A 04 12 34 56 78>");
                myTest = Test.decode(bb2);
                test.equal(myTest.b.BE().readUint32(), 0x12345678);
            } catch (e) {
                fail(e);
            }
            test.done();
        },

        "bytesFromFile": function(test) {
            try {
                var builder = ProtoBuf.loadProto("message Image { required bytes data = 1; }"),
                    Image = builder.build("Image"),
                    data = fs.readFileSync(__dirname+"/../ProtoBuf.png"),
                    image = new Image({ data: data }),
                    bb = image.encode(),
                    imageDec = Image.decode(bb),
                    dataDec = imageDec.data.toBuffer();
                test.strictEqual(data.length, dataDec.length);
                test.deepEqual(data, dataDec);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "notEnoughBytes": function(test) {
            var builder = ProtoBuf.loadProto("message Test { required bytes b = 1; }");
            var Test = builder.build("Test");
            var bb = new ByteBuffer().writeUint32(0x12345678).flip();
            var encoded = new ByteBuffer(6);
            new Test(bb).encode(encoded);
            test.equal(encoded.toString("debug"), "<0A 04 12 34 56 78>");
            encoded = encoded.slice(0, 5); // chop off the last byte
            var err = null;
            try {
                Test.decode(encoded);
            } catch (caught) {
                err = caught;
            }
            test.ok(err && err.message && err.message.indexOf(": 4 required but got only 3") >= 0);
            test.done();
        },

        "bool": function(test) {
            try {
                var builder = ProtoBuf.loadProto("message Test { optional bool ok = 1 [ default = false ]; }"),
                    Test = builder.build("Test"),
                    t =  new Test();
                test.strictEqual(t.ok, false);
                t.setOk("true");
                test.strictEqual(t.ok, true);
                test.strictEqual(Test.decode(t.encode()).ok, true);
                t.setOk("false");
                test.strictEqual(t.ok, false);
                test.strictEqual(Test.decode(t.encode()).ok, false);
            } catch (err) {
                fail(err);
            }
            test.done();
        },

        // As mentioned by Bill Katz
        "T139": function(test) {
            try{
                var builder = ProtoBuf.loadProtoFile(__dirname+"/T139.proto");
                var T139 = builder.build("T139");
                test.ok(typeof T139 == 'function');
                var inst = new T139(139,139);
                test.equal(inst.a, 139);
                test.equal(inst.b, 139);
                inst.setA(139);
                inst.setB(139);
                test.equal(inst.a, 139);
                test.equal(inst.b, 139);
                var bb = new ByteBuffer(3);
                inst.encode(bb);
                test.equal(bb.toString("debug"), "<08 8B 01 10 8B 01>");
                var instDec = T139.decode(bb);
                test.equal(instDec.a, 139);
                test.equal(instDec.b, 139);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "emptyDefaultString": function(test) {
            try {
                var builder = ProtoBuf.loadProto("message Test1 { optional string test = 1 [default = \"\"]; }");
                var Test1;
                test.doesNotThrow(function() {
                    Test1 = builder.build("Test1");
                });
                var test1 = new Test1();
                test.strictEqual(test1.test, "");
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "trailingSemicolon": function(test) {
            try {
                var builder = ProtoBuf.loadProto("message Test1 { optional string test = 1; };");
                test.doesNotThrow(function() {
                    var Test1 = builder.build("Test1");
                });
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "inner": {

            "longstr": function(test) {
                try {
                    var builder = ProtoBuf.loadProto("message Test { required Inner a = 1; message Inner { required string b = 1; } }");
                    var Test = builder.build("Test");
                    var t = new Test();
                    var data = "0123456789"; // 10: 20, 40, 80, 160, 320 bytes
                    for (var i=0; i<5; i++) data += data;
                    test.equal(data.length, 320);
                    t.a = new Test.Inner(data);
                    var bb = t.encode();
                    var t2 = Test.decode(bb);
                    test.equal(t2.a.b.length, 320);
                    test.equal(data, t2.a.b);
                } catch (e) {
                    fail(e);
                }
                test.done();
            },

            "multiple": function(test) {
                try {
                    var str = "";
                    for (var i=0; i<200; i++) str += 'a';
                    var builder = ProtoBuf.loadProtoFile(__dirname+"/inner.proto");
                    var fooCls = builder.build("Foo");
                    var barCls = builder.build("Bar");
                    var bazCls = builder.build("Baz");
                    var foo = new fooCls(new barCls(str), new bazCls(str));
                    var fooEncoded = foo.encode();
                    test.doesNotThrow(function() {
                        fooCls.decode(fooEncoded);
                    });
                } catch (e) {
                    fail(e);
                }
                test.done();
            },

            "float": function(test) {
                try {
                    var builder = ProtoBuf.loadProto("message Foo { required Bar bar = 1; } message Bar { required float baz = 1; }");
                    var root = builder.build();
                    var foo = new root.Foo(new root.Bar(4));
                    var bb = foo.encode();
                    var foo2 = root.Foo.decode(bb);
                    test.equal(foo.bar.baz, 4);
                    test.equal(foo2.bar.baz, foo.bar.baz);
                } catch (e) {
                    fail(e);
                }
                test.done();
            }
            
        },
        
        "truncated": function(test) {
            try {
                var builder = ProtoBuf.loadProto("message Test { required int32 a = 1; required int32 b = 2; }");
                var Test = builder.build("Test");
                var t = new Test(), bb = new ByteBuffer(2);
                t.setA(1);
                try {
                    bb = t.encode(bb).flip();
                    test.ok(false);
                } catch (e) {
                    test.ok(e.encoded);
                    bb = e.encoded.flip();
                    test.equal(bb.toString("debug"), "<08 01>");
                }
                var t2;
                try /* to decode truncated message */ {
                    t2 = Test.decode(bb);
                    test.ok(false); // ^ throws
                } catch (e) {
                    // But still be able to access the rest
                    var t3 = e.decoded;
                    test.strictEqual(t3.a, 1);
                    test.strictEqual(t3.b, null);
                }
                test.strictEqual(t2, undefined);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        // Options on all levels
        "options": {
            
            "parse": function(test) {
                try {
                    var parser = new ProtoBuf.DotProto.Parser(ProtoBuf.Util.fetch(__dirname+"/options.proto"));
                    var root = parser.parse();
                    test.equal(root["package"], "My");
                    test.strictEqual(root["options"]["(toplevel_1)"], 10);
                    test.equal(root["options"]["(toplevel_2)"], "Hello world!");
                    test.equal(root["messages"][0]["fields"][0]["options"]["default"], "Max");
                    test.equal(root["messages"][0]["options"]["(inmessage)"], "My.Test")
                } catch (e) {
                    fail(e);
                }
                test.done();
            },
            
            "export": function(test) {
                try {
                    var builder = ProtoBuf.loadProtoFile(__dirname+"/options.proto");
                    var My = builder.build("My");
                    test.deepEqual(My.$options, {
                        "(toplevel_1)": 10,
                        "(toplevel_2)": "Hello world!"
                    });
                    test.strictEqual(My.$options['(toplevel_1)'], 10);
                    test.deepEqual(My.Test.$options, {
                        "(inmessage)": "My.Test" // TODO: Options are not resolved, yet.
                    });
                } catch (e) {
                    fail(e);
                }
                test.done();
            }
        },
        
        // Comments
        "comments": function(test) {
            try {
                var tn = new ProtoBuf.DotProto.Tokenizer(ProtoBuf.Util.fetch(__dirname+'/comments.proto'));
                var token, tokens = [];
                do {
                    token = tn.next();
                    tokens.push(token);
                } while (token !== null);
                test.deepEqual(tokens, ['message', 'TestC', '{', 'required', 'int32', 'a', '=', '1', ';', '}', null]);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
    
        // A more or less complex proto with type references
        "complexProto": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/complex.proto");
                validateComplex(test, builder.build("Game"));
                var TCars = builder.lookup("Game.Cars");
                test.strictEqual(TCars.fqn(), ".Game.Cars");
            } catch(e) {
                fail(e);
            }
            test.done();
        },
        
        // The same created without calling upon the parser to do so (noparse)
        "complexJSON": function(test) {
            try {
                var builder = ProtoBuf.loadJsonFile(__dirname+"/complex.json");
                validateComplex(test, builder.build("Game"));
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        // Builder reused to add definitions from multiple sources
        "multiBuilder": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/example1.proto");
                ProtoBuf.loadProtoFile(__dirname+"/example2.proto", builder);
                var ns = builder.build();
                test.ok(!!ns.Test1);
                test.ok(!!ns.Test2);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        // Inner messages test
        "repeated": {
            "legacy": function(test) {
                try {
                    var builder = ProtoBuf.loadProtoFile(__dirname+"/repeated.proto");
                    var root = builder.build();
                    var Outer = root.Outer;
                    var Inner = root.Inner;
                    var outer = new Outer({ inner: [new Inner(1), new Inner(2)] });
                    var bb = new ByteBuffer(8);
                    outer.encode(bb);
                    test.equal("<0A 02 08 01 0A 02 08 02>", bb.toString("debug"));
                    var douter = Outer.decode(bb);
                    test.ok(douter.inner instanceof Array);
                    test.equal(douter.inner.length, 2);
                    test.equal(douter.inner[0].inner_value, 1);
                    test.equal(douter.inner[1].inner_value, 2);
                    test.ok(douter.innerPacked instanceof Array);
                    test.equal(douter.innerPacked.length, 0);
                } catch (e) {
                    fail(e);
                }
                test.done();
            },
            
            "packed": function(test) {
                try {
                    var builder = ProtoBuf.loadProtoFile(__dirname+"/repeated.proto");
                    var root = builder.build();
                    var Outer = root.Outer;
                    var Inner = root.Inner;
                    var outer = new Outer({ innerPacked: [new Inner(1), new Inner(2)] });
                    var bb = new ByteBuffer(8);
                    outer.encode(bb);
                    test.equal("<12 06 02 08 01 02 08 02>", bb.toString("debug"));
                    var douter = Outer.decode(bb);
                    test.ok(douter.inner instanceof Array);
                    test.equal(douter.inner.length, 0);
                    test.ok(douter.innerPacked instanceof Array);
                    test.equal(douter.innerPacked.length, 2);
                    test.equal(douter.innerPacked[0].inner_value, 1);
                    test.equal(douter.innerPacked[1].inner_value, 2);
                } catch (e) {
                    fail(e);
                }
                test.done();
            },
            
            "both": function(test) {
                try {
                    var builder = ProtoBuf.loadProtoFile(__dirname+"/repeated.proto");
                    var root = builder.build();
                    var Outer = root.Outer;
                    var Inner = root.Inner;
                    var outer = new Outer({ inner: [new Inner(1), new Inner(2)], innerPacked: [new Inner(3), new Inner(4)] });
                    var bb = new ByteBuffer(16);
                    outer.encode(bb);
                    test.equal("<0A 02 08 01 0A 02 08 02 12 06 02 08 03 02 08 04>", bb.toString("debug"));
                    var douter = Outer.decode(bb);
                    test.ok(douter.inner instanceof Array);
                    test.equal(douter.inner.length, 2);
                    test.equal(douter.inner[0].inner_value, 1);
                    test.equal(douter.inner[1].inner_value, 2);
                    test.ok(douter.innerPacked instanceof Array);
                    test.equal(douter.innerPacked.length, 2);
                    test.equal(douter.innerPacked[0].inner_value, 3);
                    test.equal(douter.innerPacked[1].inner_value, 4);
                } catch (e) {
                    fail(e);
                }
                test.done();
            },
            
            "none": function(test) {
                try {
                    var builder = ProtoBuf.loadProtoFile(__dirname+"/repeated.proto");
                    var Outer = builder.build("Outer");
                    var outer = new Outer();
                    var bb = new ByteBuffer(1);
                    outer.encode(bb);
                    test.equal("|00 ", bb.toString("debug"));
                    var douter = Outer.decode(bb);
                    test.ok(douter.inner instanceof Array);
                    test.equal(douter.inner.length, 0);
                    test.ok(douter.innerPacked instanceof Array);
                    test.equal(douter.innerPacked.length, 0);
                } catch (e) {
                    fail(e);
                }
                test.done();
            }
        },
        
        "x64Fixed": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/x64.proto");
                var Test = builder.build("Test");
                var myTest = new Test();
                test.ok(myTest.val instanceof ByteBuffer.Long);
                test.equal(myTest.val.unsigned, false);
                test.equal(myTest.val.toNumber(), -1);
                test.ok(myTest.uval instanceof ByteBuffer.Long);
                test.equal(myTest.uval.unsigned, true);
                test.equal(myTest.uval.toNumber(), 1);
                
                myTest.setVal(-2);
                myTest.setUval(2);
                var bb = new ByteBuffer(18); // 2x tag + 2x 64bit
                myTest.encode(bb);
                test.equal(bb.toString("debug"), "<09 FE FF FF FF FF FF FF FF 11 02 00 00 00 00 00 00 00>");
                //                         ^ wireType=1, id=1         ^ wireType=1, id=2
                myTest = Test.decode(bb);
                test.ok(myTest.val instanceof ByteBuffer.Long);
                test.equal(myTest.val.unsigned, false);
                test.equal(myTest.val.toNumber(), -2);
                test.ok(myTest.uval instanceof ByteBuffer.Long);
                test.equal(myTest.uval.unsigned, true);
                test.equal(myTest.uval.toNumber(), 2);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
    
        "x64Varint": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/x64.proto");
                var Test = builder.build("Test2");
                var myTest = new Test();
                test.ok(myTest.val instanceof ByteBuffer.Long);
                test.equal(myTest.val.unsigned, false);
                test.equal(myTest.val.toNumber(), -1);
                test.ok(myTest.uval instanceof ByteBuffer.Long);
                test.equal(myTest.uval.unsigned, true);
                test.equal(myTest.uval.toNumber(), 1);
                test.ok(myTest.sval instanceof ByteBuffer.Long);
                test.equal(myTest.sval.unsigned, false);
                test.equal(myTest.sval.toNumber(), -2);
    
                myTest.setVal(-2);
                myTest.setUval(2);
                myTest.setSval(-3);
                var bb = new ByteBuffer(3+10+2); // 3x tag + 1x varint 10byte + 2x varint 1byte
                myTest.encode(bb);
                test.equal(bb.toString("debug"), "<08 FE FF FF FF FF FF FF FF FF 01 10 02 18 05>");
                // 08: wireType=0, id=1, 18: wireType=0, id=2, ?: wireType=0, id=3
                myTest = Test.decode(bb);
                test.ok(myTest.val instanceof ByteBuffer.Long);
                test.equal(myTest.val.unsigned, false);
                test.equal(myTest.val.toNumber(), -2);
                test.ok(myTest.uval instanceof ByteBuffer.Long);
                test.equal(myTest.uval.unsigned, true);
                test.equal(myTest.uval.toNumber(), 2);
                test.ok(myTest.sval instanceof ByteBuffer.Long);
                test.equal(myTest.sval.unsigned, false);
                test.equal(myTest.sval.toNumber(), -3);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "imports": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/imports.proto");
                var root = builder.build();
                test.ok(!!root.Test1);
                test.ok(!!root.Test2);
                test.ok(!!root.My.Test2);
                test.notEqual(root.Test2, root.My.Test2);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "toplevel": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/toplevel.proto");
                var My = builder.build("My");
                test.ok(!!My.MyEnum);
                test.equal(My.MyEnum.ONE, 1);
                test.equal(My.MyEnum.TWO, 2);
                test.ok(!!My.Test);
                var myTest = new My.Test();
                test.equal(myTest.num, My.MyEnum.ONE);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "importsToplevel": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/imports-toplevel.proto");
                var My = builder.build("My");
                test.ok(!!My.MyEnum);
                test.equal(My.MyEnum1.ONE, 1);
                test.equal(My.MyEnum1.TWO, 2);
                test.ok(!!My.Test1);
                var myTest = new My.Test1();
                test.equal(myTest.num, My.MyEnum.ONE);
                test.equal(myTest.num1, My.MyEnum1.ONE);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "importDuplicate": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/import_a.proto");
                test.doesNotThrow(function() {
                    ProtoBuf.loadProtoFile(__dirname+"/import_b.proto", builder);
                });
                var root = builder.build();
                test.ok(root.A);
                test.ok(root.B);
                test.ok(root.Common);                
            } catch (e) {
                fail(e);
            }
            test.done();
        },

        "importDuplicateDifferentBuilder": function(test) {
            try {
                var builderA = ProtoBuf.loadProtoFile(__dirname+"/import_a.proto");
                var builderB;
                test.doesNotThrow(function() {
                    builderB = ProtoBuf.loadProtoFile(__dirname+"/import_b.proto");
                });
                var rootA = builderA.build();
                var rootB = builderB.build();
                test.ok(rootA.A);
                test.ok(rootB.B);
                test.ok(rootA.Common);                
                test.ok(rootB.Common);                
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "importRoot": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile({
                    root: __dirname,
                    file: "importRoot/file1.proto"
                });
                var Test = builder.build("Test");
                test.ok(new Test() instanceof ProtoBuf.Builder.Message);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "extend": function(test) {
            try {
                var ast = new ProtoBuf.DotProto.Parser(fs.readFileSync(__dirname+"/extend.proto")).parse();
                test.deepEqual(ast,  {
                    "package": null,
                    "messages": [
                        {
                            "ref": "google.protobuf.MessageOptions",
                            "fields": [
                                {
                                    "rule": "optional",
                                    "type": "int32",
                                    "name": "foo",
                                    "id": 123,
                                    "options": {}
                                }
                            ]
                        },
                        {
                            "name": "Foo",
                            "fields": [],
                            "enums": [],
                            "messages": [],
                            "options": {},
                            "extensions": [2,536870911]
                        },
                        {
                            "ref": "Foo",
                            "fields":[
                                {
                                    "rule": "optional",
                                    "type": "string",
                                    "name": "bar",
                                    "id": 2,
                                    "options": {}
                                }
                            ]
                        },
                        {
                            "name": "Bar",
                            "fields": [],
                            "enums": [],
                            "messages": [
                                {
                                    "ref": "Foo",
                                    "fields": [
                                        {
                                            "rule": "optional",
                                            "type": "Bar",
                                            "name": "bar2",
                                            "id": 3,
                                            "options": {}
                                        }
                                    ]
                                }
                            ],
                            "options": {}
                        }
                    ],
                    "enums": [],
                    "imports": [
                        "google/protobuf/descriptor.proto"
                    ],
                    "options": {},
                    "services": []
                });
                
                var builder = ProtoBuf.loadProtoFile(__dirname+"/extend.proto");
                var TFoo = builder.lookup("Foo"),
                    TBar = builder.lookup("Bar"),
                    fields = TFoo.getChildren(ProtoBuf.Reflect.Message.Field);
                test.strictEqual(fields.length, 2);
                test.strictEqual(fields[0].name, "bar");
                test.strictEqual(fields[0].id, 2);
                test.strictEqual(fields[1].name, "bar2");
                test.strictEqual(fields[1].id, 3);
                test.deepEqual(TFoo.extensions, [2, ProtoBuf.Lang.ID_MAX]); // Defined
                test.deepEqual(TBar.extensions, [ProtoBuf.Lang.ID_MIN, ProtoBuf.Lang.ID_MAX]); // Undefined
                // test.strictEqual(TBar.getChildren(ProtoBuf.Reflect.Message.Field).length, 0);
                var root = builder.build();
                var foo = new root.Foo(),
                    bar = new root.Bar();
                test.ok(typeof foo.setBar === 'function');
                // test.ok(foo instanceof root.Bar.Foo);
                foo.bar = "123";
                foo.bar2 = bar;
                test.equal(foo.encode().compact().toString("debug"), "<12 03 31 32 33 1A 00>");
            } catch (e) {
                fail(e);
            }
            test.done();
        },

        // Custom options on all levels
        // victorr (https://github.com/victorr)
        "customOptions": function(test) {
            try {
                var parser = new ProtoBuf.DotProto.Parser(ProtoBuf.Util.fetch(__dirname+"/custom-options.proto"));
                var root = parser.parse();
                test.equal(root["options"]["(my_file_option)"], "Hello world!");
                test.equal(root["messages"][7]["options"]["(my_message_option)"], 1234);
                test.equal(root["messages"][7]["fields"][0]["options"]["(my_field_option)"], 4.5);
                // test.equal(root["services"]["MyService"]["options"]["my_service_option"], "FOO");
                // TODO: add tests for my_enum_option, my_enum_value_option
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "services": function(test) {
            try {
                var parser = new ProtoBuf.DotProto.Parser(ProtoBuf.Util.fetch(__dirname+"/custom-options.proto"));
                var root = parser.parse();
                test.deepEqual(root["services"], [{
                    "name": "MyService",
                    "rpc": {
                        "MyMethod": {
                            "request": "RequestType",
                            "response": "ResponseType",
                            "options": {
                                "(my_method_option).foo": 567,
                                "(my_method_option).bar": "Some string"
                            }
                        }
                    },
                    "options": {
                        "(my_service_option)": "FOO"
                    }
                }]);
                
                var builder = ProtoBuf.loadProtoFile(__dirname+"/custom-options.proto");
                var root = builder.build(),
                    MyService = root.MyService,
                    RequestType = root.RequestType,
                    ResponseType = root.ResponseType,
                    called = false;
                
                test.deepEqual(MyService.$options, {
                    "(my_service_option)": "FOO"
                });
                test.deepEqual(MyService.MyMethod.$options, {
                    "(my_method_option).foo": 567,
                    "(my_method_option).bar": "Some string"
                });
                
                // Provide the service with your actual RPC implementation based on whatever framework you like most.
                var myService = new MyService(function(method, req, callback) {
                    test.strictEqual(method, ".MyService.MyMethod");
                    test.ok(req instanceof RequestType);
                    called = true;
                    
                    // In this case we just return no error and our pre-built response. This must be properly async!
                    setTimeout(callback.bind(this, null, (new ResponseType()).encode() /* as raw bytes for debugging */ ));
                });
                
                test.deepEqual(myService.$options, MyService.$options);
                test.deepEqual(myService.MyMethod.$options, MyService.MyMethod.$options);
                
                // Call the service with your request message and provide a callback. This will call your actual service
                // implementation to perform the request and gather a response before calling the callback. If the
                // request or response type is invalid i.e. not an instance of RequestType or ResponseType, your
                // implementation will not be called as ProtoBuf.js handles this case internally and directly hands the
                // error to your callback below.
                myService.MyMethod(new RequestType(), function(err, res) {
                    // We get: err = null, res = our prebuilt response. And that's it.
                    if (err !== null) {
                        fail(err);
                    }
                    test.strictEqual(called, true);
                    test.ok(res instanceof ResponseType);
                    test.done();
                });
            } catch (e) {
                fail(e);
            }
        },
        
        // Properly ignore "syntax" and "extensions" keywords
        "gtfs-realtime": function(test) {
            try {
                test.doesNotThrow(function() {
                    ProtoBuf.loadProtoFile(__dirname+"/gtfs-realtime.proto");
                });
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "fields": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/optional.proto");
                var Test1 = builder.build("Test1");
                var test1 = new Test1();
                test.strictEqual(test1.a, null);
                test.deepEqual(Object.keys(test1), ['a']);
                var bb = test1.encode();
                test1 = Test1.decode(bb);
                test.strictEqual(test1.a, null);
                test.deepEqual(Object.keys(test1), ['a']);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "fieldsToCamelCase": function(test) {
            try {
                ProtoBuf.convertFieldsToCamelCase = true;
                var builder = ProtoBuf.loadProtoFile(__dirname+"/camelcase.proto");
                var Test = builder.build("Test"),
                    TTest = builder.lookup("Test");
                var msg = new Test();

                // Reverted collision on 1st
                test.strictEqual(msg.some_field, null);
                test.strictEqual(msg.someField, null);
                test.equal(TTest.getChild("some_field").id, 1);
                test.equal(TTest.getChild("someField").id, 2);


                // Reverted collision on 2nd
                test.strictEqual(msg.aField, null);
                test.strictEqual(msg.a_field, null);
                test.equal(TTest.getChild("aField").id, 3);
                test.equal(TTest.getChild("a_field").id, 4);
                
                // No collision
                test.strictEqual(msg.itsAField, null);
                test.equal(TTest.getChild("itsAField").id, 5);
                
                test.ok(typeof msg.set_its_a_field === "function");
                test.ok(typeof msg.setItsAField === "function");
                
                ProtoBuf.convertFieldsToCamelCase = false;
            } catch (e) {
                ProtoBuf.convertFieldsToCamelCase = false;
                fail(e);
            }
            test.done();
        },
        
        "setarray": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/setarray.proto");
                var root = builder.build(),
                    Outer = root.Outer,
                    Inner = root.Inner,
                    inners = [];
                
                // Array of repeated messages
                inners.push(new Inner("a"), new Inner("b"), new Inner("c"));
                var outer = new Outer();
                outer.setInners(inners);
                test.deepEqual(outer.inners, inners);
                
                // Array of repeated message objects
                inners = [];
                inners.push({ str: 'a' }, { str: 'b' }, { str: 'c' });
                outer.setInners(inners); // Converts
                test.ok(outer.inners[0] instanceof Inner);
                test.deepEqual(outer.inners, inners);
            } catch (e) {
                fail(e);
            }
            test.done();
        },


        // Make sure that our example at https://github.com/dcodeIO/ProtoBuf.js/wiki is not nonsense
        "pingexample": function(test) {
            try {
                var builder = ProtoBuf.loadProtoFile(__dirname+"/PingExample.proto");
                var Message = builder.build("Message");
                var msg = new Message();
                msg.ping = new Message.Ping(123456789);
                var bb = msg.encode();
                test.strictEqual(bb.length, 7);
                msg = Message.decode(bb);
                test.ok(msg.ping);
                test.notOk(msg.pong);
                test.strictEqual(msg.ping.time, 123456789);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "negEnumId": function(test) {
            try {
                test.doesNotThrow(function() {
                    var builder = ProtoBuf.loadProtoFile(__dirname+"/negid.proto");
                    var Test = builder.build("Test");
                    test.strictEqual(Test.LobbyType.INVALID, -1);
                    var t = new Test(Test.LobbyType.INVALID);
                    test.strictEqual(t.type, -1);
                    var bb = t.encode();
                    t = Test.decode(bb);
                    test.strictEqual(t.type, -1);
                });                
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "base64": function(test) {
            try {
                var Message = ProtoBuf.loadProto("message Message { required string s = 1; }").build("Message");
                var msg = new Message("ProtoBuf.js");
                var b64 = msg.toBase64();
                test.strictEqual(b64, "CgtQcm90b0J1Zi5qcw==");
                var msg2 = Message.decode64(b64);
                test.deepEqual(msg, msg2);
                msg2 = Message.decode(b64, "base64");
                test.deepEqual(msg, msg2);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "hex": function(test) {
            try {
                var Message = ProtoBuf.loadProto("message Message { required string s = 1; }").build("Message");
                var msg = new Message("ProtoBuf.js");
                var hex = msg.toHex();
                test.strictEqual(hex, "0A0B50726F746F4275662E6A73");
                var msg2 = Message.decodeHex(hex);
                test.deepEqual(msg, msg2);
                msg2 = Message.decode(hex, "hex");
                test.deepEqual(msg, msg2);
            } catch (e) {
                fail(e);
            }
            test.done();
        },

        "forwardComp": function(test) {
            try {
                var Message = ProtoBuf.loadProto("message Message { required int32 a = 1; required string b = 2; required float c = 3; }").build("Message");
                var msg = new Message(123, "abc", 0.123);
                var bb = msg.encode();
                Message = ProtoBuf.loadProto("message Message {}").build("Message");
                test.doesNotThrow(function() {
                    Message.decode(bb);
                });
                test.strictEqual(bb.offset, bb.length);
            } catch (e) {
                fail(e);
            }
            test.done();
        },
        
        "tokenizerLine": function(test) {
            try {
                var parser = new ProtoBuf.DotProto.Parser("package test;\n\nmessage Message {\n\trequired string invalid = 1;}ERROR\n"),
                    ast = null, err = null;
                try {
                    ast = parser.parse();
                } catch (caught) {
                    err = caught;
                }
                test.ok(err);
                test.notOk(ast);
                test.ok(err.message.indexOf("line 4:") >= 0);
            } catch (e) {
                fail(e);
            }
            test.done();
        },

        "excludeFields": function(test) {
            try {
                var builder = ProtoBuf.loadProto("message A { required int32 i = 1; } message B { required A A = 1; }");
                builder.build();
            } catch (e) {
                fail(e);
            }
            test.done();
        },

        "proto2jsExtend": function(test) {
            try {
                var builder = ProtoBuf.loadJsonFile(__dirname+"/proto2js/Bar.json");
                builder.build();
            } catch (e) {
                fail(e);
            }
            test.done();
        },

        "emptyMessage": function(test) {
            try {
                var builder = ProtoBuf.loadProto("message EmptyMessage {}"),
                    EmptyMessage = builder.build("EmptyMessage");

                var msg = new EmptyMessage(),
                    ab = msg.toArrayBuffer();
                test.strictEqual(ab.byteLength, 0);
            } catch (e) {
                fail(e);
            }
            test.done();
        },

        // Node.js only
        "loaders": BROWSER ? {} : {
            
            "commonjs": function(test) {
                var fs = require("fs")
                  , vm = require("vm")
                  , util = require('util');
        
                var code = fs.readFileSync(__dirname+"/../"+FILE);
                var sandbox = new Sandbox({
                    module: {
                        exports: {}
                    },
                    require: (function() {
                        function require(mod) {
                            if (mod == 'bytebuffer') require.called = true;
                            return ByteBuffer;
                        }
                        require.called = false;
                        return require;
                    })()
                });
                vm.runInNewContext(code, sandbox, "ProtoBuf.js in CommonJS-VM");
                // console.log(util.inspect(sandbox));
                test.ok(typeof sandbox.module.exports == 'object');
                test.ok(typeof sandbox.require != 'undefined' && sandbox.require.called);
                test.done();
            },
        
            "amd": function(test) {
                var fs = require("fs")
                  , vm = require("vm")
                  , util = require('util');
        
                var code = fs.readFileSync(__dirname+"/../"+FILE);
                var sandbox = new Sandbox({
                    define: (function() {
                        function define() {
                            define.called = true;
                        }
                        define.amd = true;
                        define.called = false;
                        return define;
                    })()
                });
                vm.runInNewContext(code, sandbox, "ProtoBuf.js in AMD-VM");
                // console.log(util.inspect(sandbox));
                test.ok(sandbox.define.called == true);
                test.done();
            },
        
            "shim": function(test) {
                var fs = require("fs")
                  , vm = require("vm")
                  , util = require('util');
        
                var code = fs.readFileSync(__dirname+"/../"+FILE);
                var sandbox = new Sandbox({
                    dcodeIO: {
                        ByteBuffer: ByteBuffer
                    }
                });
                vm.runInNewContext(code, sandbox, "ProtoBuf.js in shim-VM");
                // console.log(util.inspect(sandbox));
                test.ok(typeof sandbox.dcodeIO != 'undefined' && typeof sandbox.dcodeIO.ProtoBuf != 'undefined');
                test.done();
            }
        }
    };
    
    if (typeof module != 'undefined' && module.exports) {
        module.exports = suite;
    } else {
        global["suite"] = suite;
    }

})(this);
