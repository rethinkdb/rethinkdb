/*
  Copyright (C) 2012 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2012 Joost-Wim Boekesteijn <joost-wim@boekesteijn.nl>
  Copyright (C) 2012 Yusuke Suzuki <utatane.tea@gmail.com>
  Copyright (C) 2012 Arpad Borsos <arpad.borsos@googlemail.com>
  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Yusuke Suzuki <utatane.tea@gmail.com>
  Copyright (C) 2011 Arpad Borsos <arpad.borsos@googlemail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

var testFixture;

var harmonyTestFixture = {

    'ES6 Unicode Code Point Escape Sequence': {

        '"\\u{714E}\\u{8336}"': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: '煎茶',
                raw: '"\\u{714E}\\u{8336}"',
                range: [0, 18],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 18 }
                }
            },
            range: [0, 18],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 18 }
            }
        },

        '"\\u{20BB7}\\u{91CE}\\u{5BB6}"': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: '\ud842\udfb7\u91ce\u5bb6',
                raw: '"\\u{20BB7}\\u{91CE}\\u{5BB6}"',
                range: [0, 27],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 27 }
                }
            },
            range: [0, 27],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 27 }
            }
        }
    },

    // ECMAScript 6th Syntax, 7.8.3 Numeric Literals

    'ES6: Numeric Literal': {

        '00': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 0,
                raw: '00',
                range: [0, 2],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 2 }
                }
            },
            range: [0, 2],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 2 }
            }
        },

        '0o0': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 0,
                raw: '0o0',
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        'function test() {\'use strict\'; 0o0; }': {
            type: 'FunctionDeclaration',
            id: {
                type: 'Identifier',
                name: 'test',
                range: [9, 13],
                loc: {
                    start: { line: 1, column: 9 },
                    end: { line: 1, column: 13 }
                }
            },
            params: [],
            defaults: [],
            body: {
                type: 'BlockStatement',
                body: [{
                    type: 'ExpressionStatement',
                    expression: {
                        type: 'Literal',
                        value: 'use strict',
                        raw: '\'use strict\'',
                        range: [17, 29],
                        loc: {
                            start: { line: 1, column: 17 },
                            end: { line: 1, column: 29 }
                        }
                    },
                    range: [17, 30],
                    loc: {
                        start: { line: 1, column: 17 },
                        end: { line: 1, column: 30 }
                    }
                }, {
                    type: 'ExpressionStatement',
                    expression: {
                        type: 'Literal',
                        value: 0,
                        raw: '0o0',
                        range: [31, 34],
                        loc: {
                            start: { line: 1, column: 31 },
                            end: { line: 1, column: 34 }
                        }
                    },
                    range: [31, 35],
                    loc: {
                        start: { line: 1, column: 31 },
                        end: { line: 1, column: 35 }
                    }
                }],
                range: [16, 37],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 37 }
                }
            },
            rest: null,
            generator: false,
            expression: false,
            range: [0, 37],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 37 }
            }
        },

        '0o2': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 2,
                raw: '0o2',
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        '0o12': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 10,
                raw: '0o12',
                range: [0, 4],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 4 }
                }
            },
            range: [0, 4],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 4 }
            }
        },

        '0O0': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 0,
                raw: '0O0',
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        'function test() {\'use strict\'; 0O0; }': {
            type: 'FunctionDeclaration',
            id: {
                type: 'Identifier',
                name: 'test',
                range: [9, 13],
                loc: {
                    start: { line: 1, column: 9 },
                    end: { line: 1, column: 13 }
                }
            },
            params: [],
            defaults: [],
            body: {
                type: 'BlockStatement',
                body: [{
                    type: 'ExpressionStatement',
                    expression: {
                        type: 'Literal',
                        value: 'use strict',
                        raw: '\'use strict\'',
                        range: [17, 29],
                        loc: {
                            start: { line: 1, column: 17 },
                            end: { line: 1, column: 29 }
                        }
                    },
                    range: [17, 30],
                    loc: {
                        start: { line: 1, column: 17 },
                        end: { line: 1, column: 30 }
                    }
                }, {
                    type: 'ExpressionStatement',
                    expression: {
                        type: 'Literal',
                        value: 0,
                        raw: '0O0',
                        range: [31, 34],
                        loc: {
                            start: { line: 1, column: 31 },
                            end: { line: 1, column: 34 }
                        }
                    },
                    range: [31, 35],
                    loc: {
                        start: { line: 1, column: 31 },
                        end: { line: 1, column: 35 }
                    }
                }],
                range: [16, 37],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 37 }
                }
            },
            rest: null,
            generator: false,
            expression: false,
            range: [0, 37],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 37 }
            }
        },

        '0O2': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 2,
                raw: '0O2',
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        '0O12': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 10,
                raw: '0O12',
                range: [0, 4],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 4 }
                }
            },
            range: [0, 4],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 4 }
            }
        },


        '0b0': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 0,
                raw: '0b0',
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        '0b1': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 1,
                raw: '0b1',
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        '0b10': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 2,
                raw: '0b10',
                range: [0, 4],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 4 }
                }
            },
            range: [0, 4],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 4 }
            }
        },

        '0B0': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 0,
                raw: '0B0',
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        '0B1': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 1,
                raw: '0B1',
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        '0B10': {
            type: 'ExpressionStatement',
            expression: {
                type: 'Literal',
                value: 2,
                raw: '0B10',
                range: [0, 4],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 4 }
                }
            },
            range: [0, 4],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 4 }
            }
        }

    },

    // ECMAScript 6th Syntax, 11.1. 9 Template Literals

    'ES6 Template Strings': {
        '`42`': {
            type: 'ExpressionStatement',
            expression: {
                type: 'TemplateLiteral',
                quasis: [{
                    type: 'TemplateElement',
                    value: {
                        raw: '42',
                        cooked: '42'
                    },
                    tail: true,
                    range: [0, 4],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 4 }
                    }
                }],
                expressions: [],
                range: [0, 4],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 4 }
                }
            },
            range: [0, 4],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 4 }
            }
        },

        'raw`42`': {
            type: 'ExpressionStatement',
            expression: {
                type: 'TaggedTemplateExpression',
                tag: {
                    type: 'Identifier',
                    name: 'raw',
                    range: [0, 3],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 3 }
                    }
                },
                quasi: {
                    type: 'TemplateLiteral',
                    quasis: [{
                        type: 'TemplateElement',
                        value: {
                            raw: '42',
                            cooked: '42'
                        },
                        tail: true,
                        range: [3, 7],
                        loc: {
                            start: { line: 1, column: 3 },
                            end: { line: 1, column: 7 }
                        }
                    }],
                    expressions: [],
                    range: [3, 7],
                    loc: {
                        start: { line: 1, column: 3 },
                        end: { line: 1, column: 7 }
                    }
                },
                range: [0, 7],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 7 }
                }
            },
            range: [0, 7],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 7 }
            }
        },

        'raw`hello ${name}`': {
            type: 'ExpressionStatement',
            expression: {
                type: 'TaggedTemplateExpression',
                tag: {
                    type: 'Identifier',
                    name: 'raw',
                    range: [0, 3],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 3 }
                    }
                },
                quasi: {
                    type: 'TemplateLiteral',
                    quasis: [{
                        type: 'TemplateElement',
                        value: {
                            raw: 'hello ',
                            cooked: 'hello '
                        },
                        tail: false,
                        range: [3, 12],
                        loc: {
                            start: { line: 1, column: 3 },
                            end: { line: 1, column: 12 }
                        }
                    }, {
                        type: 'TemplateElement',
                        value: {
                            raw: '',
                            cooked: ''
                        },
                        tail: true,
                        range: [16, 18],
                        loc: {
                            start: { line: 1, column: 16 },
                            end: { line: 1, column: 18 }
                        }
                    }],
                    expressions: [{
                        type: 'Identifier',
                        name: 'name',
                        range: [12, 16],
                        loc: {
                            start: { line: 1, column: 12 },
                            end: { line: 1, column: 16 }
                        }
                    }],
                    range: [3, 18],
                    loc: {
                        start: { line: 1, column: 3 },
                        end: { line: 1, column: 18 }
                    }
                },
                range: [0, 18],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 18 }
                }
            },
            range: [0, 18],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 18 }
            }
        },

        '`$`': {
            type: 'ExpressionStatement',
            expression: {
                type: 'TemplateLiteral',
                quasis: [{
                    type: 'TemplateElement',
                    value: {
                        raw: '$',
                        cooked: '$'
                    },
                    tail: true,
                    range: [0, 3],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 3 }
                    }
                }],
                expressions: [],
                range: [0, 3],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 3 }
                }
            },
            range: [0, 3],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 3 }
            }
        },

        '`\\n\\r\\b\\v\\t\\f\\\n\\\r\n`': {
            type: 'ExpressionStatement',
            expression: {
                type: 'TemplateLiteral',
                quasis: [{
                    type: 'TemplateElement',
                    value: {
                        raw: '\\n\\r\\b\\v\\t\\f\\\n\\\r\n',
                        cooked: '\n\r\b\v\t\f'
                    },
                    tail: true,
                    range: [0, 19],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 3, column: 19 }
                    }
                }],
                expressions: [],
                range: [0, 19],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 3, column: 19 }
                }
            },
            range: [0, 19],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 3, column: 19 }
            }
        },

        '`\n\r\n`': {
            type: 'ExpressionStatement',
            expression: {
                type: 'TemplateLiteral',
                quasis: [{
                    type: 'TemplateElement',
                    value: {
                        raw: '\n\r\n',
                        cooked: ''
                    },
                    tail: true,
                    range: [0, 5],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 3, column: 5 }
                    }
                }],
                expressions: [],
                range: [0, 5],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 3, column: 5 }
                }
            },
            range: [0, 5],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 3, column: 5 }
            }
        },

        '`\\u{000042}\\u0042\\x42\\u0\\102\\A`': {
            type: 'ExpressionStatement',
            expression: {
                type: 'TemplateLiteral',
                quasis: [{
                    type: 'TemplateElement',
                    value: {
                        raw: '\\u{000042}\\u0042\\x42\\u0\\102\\A',
                        cooked: 'BBBu0BA'
                    },
                    tail: true,
                    range: [0, 31],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 31 }
                    }
                }],
                expressions: [],
                range: [0, 31],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 31 }
                }
            },
            range: [0, 31],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 31 }
            }
        },

        'new raw`42`': {
            type: 'ExpressionStatement',
            expression: {
                type: 'NewExpression',
                callee: {
                    type: 'TaggedTemplateExpression',
                    tag: {
                        type: 'Identifier',
                        name: 'raw',
                        range: [4, 7],
                        loc: {
                            start: { line: 1, column: 4 },
                            end: { line: 1, column: 7 }
                        }
                    },
                    quasi: {
                        type: 'TemplateLiteral',
                        quasis: [{
                            type: 'TemplateElement',
                            value: {
                                raw: '42',
                                cooked: '42'
                            },
                            tail: true,
                            range: [7, 11],
                            loc: {
                                start: { line: 1, column: 7 },
                                end: { line: 1, column: 11 }
                            }
                        }],
                        expressions: [],
                        range: [7, 11],
                        loc: {
                            start: { line: 1, column: 7 },
                            end: { line: 1, column: 11 }
                        }
                    },
                    range: [4, 11],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 11 }
                    }
                },
                'arguments': [],
                range: [0, 11],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 11 }
                }
            },
            range: [0, 11],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 11 }
            }
        }

    },


    // ECMAScript 6th Syntax, 12.11 The switch statement

    'ES6: Switch Case Declaration': {

        'switch (answer) { case 42: let t = 42; break; }': {
            type: 'SwitchStatement',
            discriminant: {
                type: 'Identifier',
                name: 'answer',
                range: [8, 14],
                loc: {
                    start: { line: 1, column: 8 },
                    end: { line: 1, column: 14 }
                }
            },
            cases: [{
                type: 'SwitchCase',
                test: {
                    type: 'Literal',
                    value: 42,
                    raw: '42',
                    range: [23, 25],
                    loc: {
                        start: { line: 1, column: 23 },
                        end: { line: 1, column: 25 }
                    }
                },
                consequent: [{
                    type: 'VariableDeclaration',
                    declarations: [{
                        type: 'VariableDeclarator',
                        id: {
                            type: 'Identifier',
                            name: 't',
                            range: [31, 32],
                            loc: {
                                start: { line: 1, column: 31 },
                                end: { line: 1, column: 32 }
                            }
                        },
                        init: {
                            type: 'Literal',
                            value: 42,
                            raw: '42',
                            range: [35, 37],
                            loc: {
                                start: { line: 1, column: 35 },
                                end: { line: 1, column: 37 }
                            }
                        },
                        range: [31, 37],
                        loc: {
                            start: { line: 1, column: 31 },
                            end: { line: 1, column: 37 }
                        }
                    }],
                    kind: 'let',
                    range: [27, 38],
                    loc: {
                        start: { line: 1, column: 27 },
                        end: { line: 1, column: 38 }
                    }
                }, {
                    type: 'BreakStatement',
                    label: null,
                    range: [39, 45],
                    loc: {
                        start: { line: 1, column: 39 },
                        end: { line: 1, column: 45 }
                    }
                }],
                range: [18, 45],
                loc: {
                    start: { line: 1, column: 18 },
                    end: { line: 1, column: 45 }
                }
            }],
            range: [0, 47],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 47 }
            }
        }
    },

    // ECMAScript 6th Syntax, 13.13 Equality Operators
    // http://wiki.ecmascript.org/doku.php?id=harmony:egal#is_and_isnt_operators

    'ES6: Egal Operators': {

        'x is y': {
            type: 'ExpressionStatement',
            expression: {
                type: 'BinaryExpression',
                operator: 'is',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'Identifier',
                    name: 'y',
                    range: [5, 6],
                    loc: {
                        start: { line: 1, column: 5 },
                        end: { line: 1, column: 6 }
                    }
                },
                range: [0, 6],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 6 }
                }
            },
            range: [0, 6],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 6 }
            }
        },

        'x isnt y': {
            type: 'ExpressionStatement',
            expression: {
                type: 'BinaryExpression',
                operator: 'isnt',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'Identifier',
                    name: 'y',
                    range: [7, 8],
                    loc: {
                        start: { line: 1, column: 7 },
                        end: { line: 1, column: 8 }
                    }
                },
                range: [0, 8],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 8 }
                }
            },
            range: [0, 8],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 8 }
            }
        }

    },

    // ECMAScript 6th Syntax, 13.2 Arrow Function Definitions

    'ES6: Arrow Function': {
        '() => "test"': {
            type: 'ExpressionStatement',
            expression: {
                type: 'ArrowFunctionExpression',
                id: null,
                params: [],
                defaults: [],
                body: {
                    type: 'Literal',
                    value: 'test',
                    raw: '"test"',
                    range: [6, 12],
                    loc: {
                        start: { line: 1, column: 6 },
                        end: { line: 1, column: 12 }
                    }
                },
                rest: null,
                generator: false,
                expression: true,
                range: [0, 12],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 12 }
                }
            },
            range: [0, 12],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 12 }
            }
        },

        'e => "test"': {
            type: 'ExpressionStatement',
            expression: {
                type: 'ArrowFunctionExpression',
                id: null,
                params: [{
                    type: 'Identifier',
                    name: 'e',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                }],
                defaults: [],
                body: {
                    type: 'Literal',
                    value: 'test',
                    raw: '"test"',
                    range: [5, 11],
                    loc: {
                        start: { line: 1, column: 5 },
                        end: { line: 1, column: 11 }
                    }
                },
                rest: null,
                generator: false,
                expression: true,
                range: [0, 11],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 11 }
                }
            },
            range: [0, 11],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 11 }
            }
        },

        '(e) => "test"': {
            type: 'ExpressionStatement',
            expression: {
                type: 'ArrowFunctionExpression',
                id: null,
                params: [{
                    type: 'Identifier',
                    name: 'e',
                    range: [1, 2],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 2 }
                    }
                }],
                defaults: [],
                body: {
                    type: 'Literal',
                    value: 'test',
                    raw: '"test"',
                    range: [7, 13],
                    loc: {
                        start: { line: 1, column: 7 },
                        end: { line: 1, column: 13 }
                    }
                },
                rest: null,
                generator: false,
                expression: true,
                range: [0, 13],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 13 }
                }
            },
            range: [0, 13],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 13 }
            }
        },

        '(a, b) => "test"': {
            type: 'ExpressionStatement',
            expression: {
                type: 'ArrowFunctionExpression',
                id: null,
                params: [{
                    type: 'Identifier',
                    name: 'a',
                    range: [1, 2],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 2 }
                    }
                }, {
                    type: 'Identifier',
                    name: 'b',
                    range: [4, 5],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 5 }
                    }
                }],
                defaults: [],
                body: {
                    type: 'Literal',
                    value: 'test',
                    raw: '"test"',
                    range: [10, 16],
                    loc: {
                        start: { line: 1, column: 10 },
                        end: { line: 1, column: 16 }
                    }
                },
                rest: null,
                generator: false,
                expression: true,
                range: [0, 16],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 16 }
                }
            },
            range: [0, 16],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 16 }
            }
        },

        'e => { 42; }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'ArrowFunctionExpression',
                id: null,
                params: [{
                    type: 'Identifier',
                    name: 'e',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                }],
                defaults: [],
                body: {
                    type: 'BlockStatement',
                    body: [{
                        type: 'ExpressionStatement',
                        expression: {
                            type: 'Literal',
                            value: 42,
                            raw: '42',
                            range: [7, 9],
                            loc: {
                                start: { line: 1, column: 7 },
                                end: { line: 1, column: 9 }
                            }
                        },
                        range: [7, 10],
                        loc: {
                            start: { line: 1, column: 7 },
                            end: { line: 1, column: 10 }
                        }
                    }],
                    range: [5, 12],
                    loc: {
                        start: { line: 1, column: 5 },
                        end: { line: 1, column: 12 }
                    }
                },
                rest: null,
                generator: false,
                expression: false,
                range: [0, 12],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 12 }
                }
            },
            range: [0, 12],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 12 }
            }
        },

        '(a, b) => { 42; }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'ArrowFunctionExpression',
                id: null,
                params: [{
                    type: 'Identifier',
                    name: 'a',
                    range: [1, 2],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 2 }
                    }
                }, {
                    type: 'Identifier',
                    name: 'b',
                    range: [4, 5],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 5 }
                    }
                }],
                defaults: [],
                body: {
                    type: 'BlockStatement',
                    body: [{
                        type: 'ExpressionStatement',
                        expression: {
                            type: 'Literal',
                            value: 42,
                            raw: '42',
                            range: [12, 14],
                            loc: {
                                start: { line: 1, column: 12 },
                                end: { line: 1, column: 14 }
                            }
                        },
                        range: [12, 15],
                        loc: {
                            start: { line: 1, column: 12 },
                            end: { line: 1, column: 15 }
                        }
                    }],
                    range: [10, 17],
                    loc: {
                        start: { line: 1, column: 10 },
                        end: { line: 1, column: 17 }
                    }
                },
                rest: null,
                generator: false,
                expression: false,
                range: [0, 17],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 17 }
                }
            },
            range: [0, 17],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 17 }
            }
        }
    },


    // ECMAScript 6th Syntax, 13.13 Method Definitions

    'ES6: Method Definition': {

        'x = { method() { } }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'method',
                            range: [6, 12],
                            loc: {
                                start: { line: 1, column: 6 },
                                end: { line: 1, column: 12 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [],
                            defaults: [],
                            body: {
                                type: 'BlockStatement',
                                body: [],
                                range: [15, 18],
                                loc: {
                                    start: { line: 1, column: 15 },
                                    end: { line: 1, column: 18 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: false,
                            range: [15, 18],
                            loc: {
                                start: { line: 1, column: 15 },
                                end: { line: 1, column: 18 }
                            }
                        },
                        kind: 'init',
                        method: true,
                        range: [6, 18],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 18 }
                        }
                    }],
                    range: [4, 20],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 20 }
                    }
                },
                range: [0, 20],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 20 }
                }
            },
            range: [0, 20],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 20 }
            }
        },

        'x = { method(test) { } }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'method',
                            range: [6, 12],
                            loc: {
                                start: { line: 1, column: 6 },
                                end: { line: 1, column: 12 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [{
                                type: 'Identifier',
                                name: 'test',
                                range: [13, 17],
                                loc: {
                                    start: { line: 1, column: 13 },
                                    end: { line: 1, column: 17 }
                                }
                            }],
                            defaults: [],
                            body: {
                                type: 'BlockStatement',
                                body: [],
                                range: [19, 22],
                                loc: {
                                    start: { line: 1, column: 19 },
                                    end: { line: 1, column: 22 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: false,
                            range: [19, 22],
                            loc: {
                                start: { line: 1, column: 19 },
                                end: { line: 1, column: 22 }
                            }
                        },
                        kind: 'init',
                        method: true,
                        range: [6, 22],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 22 }
                        }
                    }],
                    range: [4, 24],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 24 }
                    }
                },
                range: [0, 24],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 24 }
                }
            },
            range: [0, 24],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 24 }
            }
        },

        'x = { \'method\'() { } }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Literal',
                            value: 'method',
                            raw: '\'method\'',
                            range: [6, 14],
                            loc: {
                                start: { line: 1, column: 6 },
                                end: { line: 1, column: 14 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [],
                            defaults: [],
                            body: {
                                type: 'BlockStatement',
                                body: [],
                                range: [17, 20],
                                loc: {
                                    start: { line: 1, column: 17 },
                                    end: { line: 1, column: 20 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: false,
                            range: [17, 20],
                            loc: {
                                start: { line: 1, column: 17 },
                                end: { line: 1, column: 20 }
                            }
                        },
                        kind: 'init',
                        method: true,
                        range: [6, 20],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 20 }
                        }
                    }],
                    range: [4, 22],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 22 }
                    }
                },
                range: [0, 22],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 22 }
                }
            },
            range: [0, 22],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 22 }
            }
        },

        'x = { get() { } }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'get',
                            range: [6, 9],
                            loc: {
                                start: { line: 1, column: 6 },
                                end: { line: 1, column: 9 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [],
                            defaults: [],
                            body: {
                                type: 'BlockStatement',
                                body: [],
                                range: [12, 15],
                                loc: {
                                    start: { line: 1, column: 12 },
                                    end: { line: 1, column: 15 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: false,
                            range: [12, 15],
                            loc: {
                                start: { line: 1, column: 12 },
                                end: { line: 1, column: 15 }
                            }
                        },
                        kind: 'init',
                        method: true,
                        range: [6, 15],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 15 }
                        }
                    }],
                    range: [4, 17],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 17 }
                    }
                },
                range: [0, 17],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 17 }
                }
            },
            range: [0, 17],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 17 }
            }
        },

        'x = { set() { } }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'set',
                            range: [6, 9],
                            loc: {
                                start: { line: 1, column: 6 },
                                end: { line: 1, column: 9 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [],
                            defaults: [],
                            body: {
                                type: 'BlockStatement',
                                body: [],
                                range: [12, 15],
                                loc: {
                                    start: { line: 1, column: 12 },
                                    end: { line: 1, column: 15 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: false,
                            range: [12, 15],
                            loc: {
                                start: { line: 1, column: 12 },
                                end: { line: 1, column: 15 }
                            }
                        },
                        kind: 'init',
                        method: true,
                        range: [6, 15],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 15 }
                        }
                    }],
                    range: [4, 17],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 17 }
                    }
                },
                range: [0, 17],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 17 }
                }
            },
            range: [0, 17],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 17 }
            }
        },

        'x = { method() 42 }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'method',
                            range: [6, 12],
                            loc: {
                                start: { line: 1, column: 6 },
                                end: { line: 1, column: 12 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [],
                            defaults: [],
                            body: {
                                type: 'Literal',
                                value: 42,
                                raw: '42',
                                range: [15, 17],
                                loc: {
                                    start: { line: 1, column: 15 },
                                    end: { line: 1, column: 17 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: true,
                            range: [15, 17],
                            loc: {
                                start: { line: 1, column: 15 },
                                end: { line: 1, column: 17 }
                            }
                        },
                        kind: 'init',
                        method: true,
                        range: [6, 17],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 17 }
                        }
                    }],
                    range: [4, 19],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 19 }
                    }
                },
                range: [0, 19],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 19 }
                }
            },
            range: [0, 19],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 19 }
            }
        },

        'x = { get method() 42 }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'method',
                            range: [10, 16],
                            loc: {
                                start: { line: 1, column: 10 },
                                end: { line: 1, column: 16 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [],
                            defaults: [],
                            body: {
                                type: 'Literal',
                                value: 42,
                                raw: '42',
                                range: [19, 21],
                                loc: {
                                    start: { line: 1, column: 19 },
                                    end: { line: 1, column: 21 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: true,
                            range: [19, 21],
                            loc: {
                                start: { line: 1, column: 19 },
                                end: { line: 1, column: 21 }
                            }
                        },
                        kind: 'get',
                        range: [6, 21],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 21 }
                        }
                    }],
                    range: [4, 23],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 23 }
                    }
                },
                range: [0, 23],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 23 }
                }
            },
            range: [0, 23],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 23 }
            }
        },

        'x = { set method(val) v = val }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'method',
                            range: [10, 16],
                            loc: {
                                start: { line: 1, column: 10 },
                                end: { line: 1, column: 16 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [{
                                type: 'Identifier',
                                name: 'val',
                                range: [17, 20],
                                loc: {
                                    start: { line: 1, column: 17 },
                                    end: { line: 1, column: 20 }
                                }
                            }],
                            defaults: [],
                            body: {
                                type: 'AssignmentExpression',
                                operator: '=',
                                left: {
                                    type: 'Identifier',
                                    name: 'v',
                                    range: [22, 23],
                                    loc: {
                                        start: { line: 1, column: 22 },
                                        end: { line: 1, column: 23 }
                                    }
                                },
                                right: {
                                    type: 'Identifier',
                                    name: 'val',
                                    range: [26, 29],
                                    loc: {
                                        start: { line: 1, column: 26 },
                                        end: { line: 1, column: 29 }
                                    }
                                },
                                range: [22, 29],
                                loc: {
                                    start: { line: 1, column: 22 },
                                    end: { line: 1, column: 29 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: true,
                            range: [22, 29],
                            loc: {
                                start: { line: 1, column: 22 },
                                end: { line: 1, column: 29 }
                            }
                        },
                        kind: 'set',
                        range: [6, 29],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 29 }
                        }
                    }],
                    range: [4, 31],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 31 }
                    }
                },
                range: [0, 31],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 31 }
                }
            },
            range: [0, 31],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 31 }
            }
        }

    },

    'Array Comprehension': {

        '[[x,b,c] for (x in []) for (b in []) if (b && c)]' : {
            type: 'ExpressionStatement',
            expression: {
                type: 'ComprehensionExpression',
                filter: {
                    type: 'LogicalExpression',
                    operator: '&&',
                    left: {
                        type: 'Identifier',
                        name: 'b',
                        range: [41, 42],
                        loc: {
                            start: { line: 1, column: 41 },
                            end: { line: 1, column: 42 }
                        }
                    },
                    right: {
                        type: 'Identifier',
                        name: 'c',
                        range: [46, 47],
                        loc: {
                            start: { line: 1, column: 46 },
                            end: { line: 1, column: 47 }
                        }
                    },
                    range: [41, 47],
                    loc: {
                        start: { line: 1, column: 41 },
                        end: { line: 1, column: 47 }
                    }
                },
                blocks: [{
                    type: 'ComprehensionBlock',
                    left: {
                        type: 'Identifier',
                        name: 'x',
                        range: [14, 15],
                        loc: {
                            start: { line: 1, column: 14 },
                            end: { line: 1, column: 15 }
                        }
                    },
                    right: {
                        type: 'ArrayExpression',
                        elements: [],
                        range: [19, 21],
                        loc: {
                            start: { line: 1, column: 19 },
                            end: { line: 1, column: 21 }
                        }
                    },
                    each: false,
                    of: false
                }, {
                    type: 'ComprehensionBlock',
                    left: {
                        type: 'Identifier',
                        name: 'b',
                        range: [28, 29],
                        loc: {
                            start: { line: 1, column: 28 },
                            end: { line: 1, column: 29 }
                        }
                    },
                    right: {
                        type: 'ArrayExpression',
                        elements: [],
                        range: [33, 35],
                        loc: {
                            start: { line: 1, column: 33 },
                            end: { line: 1, column: 35 }
                        }
                    },
                    each: false,
                    of: false
                }],
                body: {
                    type: 'ArrayExpression',
                    elements: [{
                        type: 'Identifier',
                        name: 'x',
                        range: [2, 3],
                        loc: {
                            start: { line: 1, column: 2 },
                            end: { line: 1, column: 3 }
                        }
                    }, {
                        type: 'Identifier',
                        name: 'b',
                        range: [4, 5],
                        loc: {
                            start: { line: 1, column: 4 },
                            end: { line: 1, column: 5 }
                        }
                    }, {
                        type: 'Identifier',
                        name: 'c',
                        range: [6, 7],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 7 }
                        }
                    }],
                    range: [1, 8],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 8 }
                    }
                },
                range: [0, 49],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 49 }
                }
            },
            range: [0, 49],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 49 }
            }
        },

        '[x for (a in [])]' : {
            type: 'ExpressionStatement',
            expression: {
                type: 'ComprehensionExpression',
                filter: null,
                blocks: [{
                    type: 'ComprehensionBlock',
                    left: {
                        type: 'Identifier',
                        name: 'a',
                        range: [8, 9],
                        loc: {
                            start: { line: 1, column: 8 },
                            end: { line: 1, column: 9 }
                        }
                    },
                    right: {
                        type: 'ArrayExpression',
                        elements: [],
                        range: [13, 15],
                        loc: {
                            start: { line: 1, column: 13 },
                            end: { line: 1, column: 15 }
                        }
                    },
                    each: false,
                    of: false
                }],
                body: {
                    type: 'Identifier',
                    name: 'x',
                    range: [1, 2],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 2 }
                    }
                },
                range: [0, 17],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 17 }
                }
            },
            range: [0, 17],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 17 }
            }
        },

        '[1 for (x in [])]' : {
            type: 'ExpressionStatement',
            expression: {
                type: 'ComprehensionExpression',
                filter: null,
                blocks: [{
                    type: 'ComprehensionBlock',
                    left: {
                        type: 'Identifier',
                        name: 'x',
                        range: [8, 9],
                        loc: {
                            start: { line: 1, column: 8 },
                            end: { line: 1, column: 9 }
                        }
                    },
                    right: {
                        type: 'ArrayExpression',
                        elements: [],
                        range: [13, 15],
                        loc: {
                            start: { line: 1, column: 13 },
                            end: { line: 1, column: 15 }
                        }
                    },
                    each: false,
                    of: false
                }],
                body: {
                    type: 'Literal',
                    value: 1,
                    raw: '1',
                    range: [1, 2],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 2 }
                    }
                },
                range: [0, 17],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 17 }
                }
            },
            range: [0, 17],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 17 }
            }
        },

        '[(x,1) for (x in [])]' : {
            type: 'ExpressionStatement',
            expression: {
                type: 'ComprehensionExpression',
                filter: null,
                blocks: [{
                    type: 'ComprehensionBlock',
                    left: {
                        type: 'Identifier',
                        name: 'x',
                        range: [12, 13],
                        loc: {
                            start: { line: 1, column: 12 },
                            end: { line: 1, column: 13 }
                        }
                    },
                    right: {
                        type: 'ArrayExpression',
                        elements: [],
                        range: [17, 19],
                        loc: {
                            start: { line: 1, column: 17 },
                            end: { line: 1, column: 19 }
                        }
                    },
                    each: false,
                    of: false
                }],
                body: {
                    type: 'SequenceExpression',
                    expressions: [{
                        type: 'Identifier',
                        name: 'x',
                        range: [2, 3],
                        loc: {
                            start: { line: 1, column: 2 },
                            end: { line: 1, column: 3 }
                        }
                    }, {
                        type: 'Literal',
                        value: 1,
                        raw: '1',
                        range: [4, 5],
                        loc: {
                            start: { line: 1, column: 4 },
                            end: { line: 1, column: 5 }
                        }
                    }],
                    range: [2, 5],
                    loc: {
                        start: { line: 1, column: 2 },
                        end: { line: 1, column: 5 }
                    }
                },
                range: [0, 21],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 21 }
                }
            },
            range: [0, 21],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 21 }
            }
        },


        '[x for (x of array)]': {
            type: 'ExpressionStatement',
            expression: {
                type: 'ComprehensionExpression',
                filter: null,
                blocks: [{
                    type: 'ComprehensionBlock',
                    left: {
                        type: 'Identifier',
                        name: 'x',
                        range: [8, 9],
                        loc: {
                            start: { line: 1, column: 8 },
                            end: { line: 1, column: 9 }
                        }
                    },
                    right: {
                        type: 'Identifier',
                        name: 'array',
                        range: [13, 18],
                        loc: {
                            start: { line: 1, column: 13 },
                            end: { line: 1, column: 18 }
                        }
                    },
                    of: true
                }],
                body: {
                    type: 'Identifier',
                    name: 'x',
                    range: [1, 2],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 2 }
                    }
                },
                range: [0, 20],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 20 }
                }
            },
            range: [0, 20],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 20 }
            }
        },

        '[x for (x of array) for (y of array2) if (x === test)]': {
            type: 'ExpressionStatement',
            expression: {
                type: 'ComprehensionExpression',
                filter: {
                    type: 'BinaryExpression',
                    operator: '===',
                    left: {
                        type: 'Identifier',
                        name: 'x',
                        range: [42, 43],
                        loc: {
                            start: { line: 1, column: 42 },
                            end: { line: 1, column: 43 }
                        }
                    },
                    right: {
                        type: 'Identifier',
                        name: 'test',
                        range: [48, 52],
                        loc: {
                            start: { line: 1, column: 48 },
                            end: { line: 1, column: 52 }
                        }
                    },
                    range: [42, 52],
                    loc: {
                        start: { line: 1, column: 42 },
                        end: { line: 1, column: 52 }
                    }
                },
                blocks: [{
                    type: 'ComprehensionBlock',
                    left: {
                        type: 'Identifier',
                        name: 'x',
                        range: [8, 9],
                        loc: {
                            start: { line: 1, column: 8 },
                            end: { line: 1, column: 9 }
                        }
                    },
                    right: {
                        type: 'Identifier',
                        name: 'array',
                        range: [13, 18],
                        loc: {
                            start: { line: 1, column: 13 },
                            end: { line: 1, column: 18 }
                        }
                    },
                    of: true
                }, {
                    type: 'ComprehensionBlock',
                    left: {
                        type: 'Identifier',
                        name: 'y',
                        range: [25, 26],
                        loc: {
                            start: { line: 1, column: 25 },
                            end: { line: 1, column: 26 }
                        }
                    },
                    right: {
                        type: 'Identifier',
                        name: 'array2',
                        range: [30, 36],
                        loc: {
                            start: { line: 1, column: 30 },
                            end: { line: 1, column: 36 }
                        }
                    },
                    of: true
                }],
                body: {
                    type: 'Identifier',
                    name: 'x',
                    range: [1, 2],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 2 }
                    }
                },
                range: [0, 54],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 54 }
                }
            },
            range: [0, 54],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 54 }
            }
        }

    },

    // http://wiki.ecmascript.org/doku.php?id=harmony:object_literals#object_literal_property_value_shorthand

    'Harmony: Object Literal Property Value Shorthand': {

        'x = { y, z }': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'Identifier',
                    name: 'x',
                    range: [0, 1],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 1 }
                    }
                },
                right: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'y',
                            range: [6, 7],
                            loc: {
                                start: { line: 1, column: 6 },
                                end: { line: 1, column: 7 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'y',
                            range: [6, 7],
                            loc: {
                                start: { line: 1, column: 6 },
                                end: { line: 1, column: 7 }
                            }
                        },
                        kind: 'init',
                        shorthand: true,
                        range: [6, 7],
                        loc: {
                            start: { line: 1, column: 6 },
                            end: { line: 1, column: 7 }
                        }
                    }, {
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'z',
                            range: [9, 10],
                            loc: {
                                start: { line: 1, column: 9 },
                                end: { line: 1, column: 10 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'z',
                            range: [9, 10],
                            loc: {
                                start: { line: 1, column: 9 },
                                end: { line: 1, column: 10 }
                            }
                        },
                        kind: 'init',
                        shorthand: true,
                        range: [9, 10],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 10 }
                        }
                    }],
                    range: [4, 12],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 12 }
                    }
                },
                range: [0, 12],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 12 }
                }
            },
            range: [0, 12],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 12 }
            }
        }

    },

    // http://wiki.ecmascript.org/doku.php?id=harmony:destructuring

    'Harmony: Destructuring': {

        '[a, b] = [b, a]': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'ArrayPattern',
                    elements: [{
                        type: 'Identifier',
                        name: 'a',
                        range: [1, 2],
                        loc: {
                            start: { line: 1, column: 1 },
                            end: { line: 1, column: 2 }
                        }
                    }, {
                        type: 'Identifier',
                        name: 'b',
                        range: [4, 5],
                        loc: {
                            start: { line: 1, column: 4 },
                            end: { line: 1, column: 5 }
                        }
                    }],
                    range: [0, 6],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 6 }
                    }
                },
                right: {
                    type: 'ArrayExpression',
                    elements: [{
                        type: 'Identifier',
                        name: 'b',
                        range: [10, 11],
                        loc: {
                            start: { line: 1, column: 10 },
                            end: { line: 1, column: 11 }
                        }
                    }, {
                        type: 'Identifier',
                        name: 'a',
                        range: [13, 14],
                        loc: {
                            start: { line: 1, column: 13 },
                            end: { line: 1, column: 14 }
                        }
                    }],
                    range: [9, 15],
                    loc: {
                        start: { line: 1, column: 9 },
                        end: { line: 1, column: 15 }
                    }
                },
                range: [0, 15],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 15 }
                }
            },
            range: [0, 15],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 15 }
            }
        },

        '({ responseText: text }) = res': {
            type: 'ExpressionStatement',
            expression: {
                type: 'AssignmentExpression',
                operator: '=',
                left: {
                    type: 'ObjectPattern',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'responseText',
                            range: [3, 15],
                            loc: {
                                start: { line: 1, column: 3 },
                                end: { line: 1, column: 15 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'text',
                            range: [17, 21],
                            loc: {
                                start: { line: 1, column: 17 },
                                end: { line: 1, column: 21 }
                            }
                        },
                        kind: 'init',
                        range: [3, 21],
                        loc: {
                            start: { line: 1, column: 3 },
                            end: { line: 1, column: 21 }
                        }
                    }],
                    range: [1, 23],
                    loc: {
                        start: { line: 1, column: 1 },
                        end: { line: 1, column: 23 }
                    }
                },
                right: {
                    type: 'Identifier',
                    name: 'res',
                    range: [27, 30],
                    loc: {
                        start: { line: 1, column: 27 },
                        end: { line: 1, column: 30 }
                    }
                },
                range: [0, 30],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 30 }
                }
            },
            range: [0, 30],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 30 }
            }
        },

        'const {a} = {}': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ObjectPattern',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'a',
                            range: [7, 8],
                            loc: {
                                start: { line: 1, column: 7 },
                                end: { line: 1, column: 8 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'a',
                            range: [7, 8],
                            loc: {
                                start: { line: 1, column: 7 },
                                end: { line: 1, column: 8 }
                            }
                        },
                        kind: 'init',
                        shorthand: true,
                        range: [7, 8],
                        loc: {
                            start: { line: 1, column: 7 },
                            end: { line: 1, column: 8 }
                        }
                    }]
                },
                init: {
                    type: 'ObjectExpression',
                    properties: [],
                    range: [12, 14],
                    loc: {
                        start: { line: 1, column: 12 },
                        end: { line: 1, column: 14 }
                    }
                },
                range: [6, 14],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 14 }
                }
            }],
            kind: 'const',
            range: [0, 14],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 14 }
            }
        },

        'const [a] = []': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ArrayPattern',
                    elements: [{
                        type: 'Identifier',
                        name: 'a',
                        range: [7, 8],
                        loc: {
                            start: { line: 1, column: 7 },
                            end: { line: 1, column: 8 }
                        }
                    }]
                },
                init: {
                    type: 'ArrayExpression',
                    elements: [],
                    range: [12, 14],
                    loc: {
                        start: { line: 1, column: 12 },
                        end: { line: 1, column: 14 }
                    }
                },
                range: [6, 14],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 14 }
                }
            }],
            kind: 'const',
            range: [0, 14],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 14 }
            }
        },

        'let {a} = {}': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ObjectPattern',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'a',
                            range: [5, 6],
                            loc: {
                                start: { line: 1, column: 5 },
                                end: { line: 1, column: 6 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'a',
                            range: [5, 6],
                            loc: {
                                start: { line: 1, column: 5 },
                                end: { line: 1, column: 6 }
                            }
                        },
                        kind: 'init',
                        shorthand: true,
                        range: [5, 6],
                        loc: {
                            start: { line: 1, column: 5 },
                            end: { line: 1, column: 6 }
                        }
                    }]
                },
                init: {
                    type: 'ObjectExpression',
                    properties: [],
                    range: [10, 12],
                    loc: {
                        start: { line: 1, column: 10 },
                        end: { line: 1, column: 12 }
                    }
                },
                range: [4, 12],
                loc: {
                    start: { line: 1, column: 4 },
                    end: { line: 1, column: 12 }
                }
            }],
            kind: 'let',
            range: [0, 12],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 12 }
            }
        },

        'let [a] = []': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ArrayPattern',
                    elements: [{
                        type: 'Identifier',
                        name: 'a',
                        range: [5, 6],
                        loc: {
                            start: { line: 1, column: 5 },
                            end: { line: 1, column: 6 }
                        }
                    }]
                },
                init: {
                    type: 'ArrayExpression',
                    elements: [],
                    range: [10, 12],
                    loc: {
                        start: { line: 1, column: 10 },
                        end: { line: 1, column: 12 }
                    }
                },
                range: [4, 12],
                loc: {
                    start: { line: 1, column: 4 },
                    end: { line: 1, column: 12 }
                }
            }],
            kind: 'let',
            range: [0, 12],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 12 }
            }
        },

        'var {a} = {}': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ObjectPattern',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'a',
                            range: [5, 6],
                            loc: {
                                start: { line: 1, column: 5 },
                                end: { line: 1, column: 6 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'a',
                            range: [5, 6],
                            loc: {
                                start: { line: 1, column: 5 },
                                end: { line: 1, column: 6 }
                            }
                        },
                        kind: 'init',
                        shorthand: true,
                        range: [5, 6],
                        loc: {
                            start: { line: 1, column: 5 },
                            end: { line: 1, column: 6 }
                        }
                    }]
                },
                init: {
                    type: 'ObjectExpression',
                    properties: [],
                    range: [10, 12],
                    loc: {
                        start: { line: 1, column: 10 },
                        end: { line: 1, column: 12 }
                    }
                },
                range: [4, 12],
                loc: {
                    start: { line: 1, column: 4 },
                    end: { line: 1, column: 12 }
                }
            }],
            kind: 'var',
            range: [0, 12],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 12 }
            }
        },

        'var [a] = []': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ArrayPattern',
                    elements: [{
                        type: 'Identifier',
                        name: 'a',
                        range: [5, 6],
                        loc: {
                            start: { line: 1, column: 5 },
                            end: { line: 1, column: 6 }
                        }
                    }]
                },
                init: {
                    type: 'ArrayExpression',
                    elements: [],
                    range: [10, 12],
                    loc: {
                        start: { line: 1, column: 10 },
                        end: { line: 1, column: 12 }
                    }
                },
                range: [4, 12],
                loc: {
                    start: { line: 1, column: 4 },
                    end: { line: 1, column: 12 }
                }
            }],
            kind: 'var',
            range: [0, 12],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 12 }
            }
        },

        'const {a:b} = {}': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ObjectPattern',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'a',
                            range: [7, 8],
                            loc: {
                                start: { line: 1, column: 7 },
                                end: { line: 1, column: 8 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'b',
                            range: [9, 10],
                            loc: {
                                start: { line: 1, column: 9 },
                                end: { line: 1, column: 10 }
                            }
                        },
                        kind: 'init',
                        range: [7, 10],
                        loc: {
                            start: { line: 1, column: 7 },
                            end: { line: 1, column: 10 }
                        }
                    }]
                },
                init: {
                    type: 'ObjectExpression',
                    properties: [],
                    range: [14, 16],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 16 }
                    }
                },
                range: [6, 16],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 16 }
                }
            }],
            kind: 'const',
            range: [0, 16],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 16 }
            }
        },

        'let {a:b} = {}': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ObjectPattern',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'a',
                            range: [5, 6],
                            loc: {
                                start: { line: 1, column: 5 },
                                end: { line: 1, column: 6 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'b',
                            range: [7, 8],
                            loc: {
                                start: { line: 1, column: 7 },
                                end: { line: 1, column: 8 }
                            }
                        },
                        kind: 'init',
                        range: [5, 8],
                        loc: {
                            start: { line: 1, column: 5 },
                            end: { line: 1, column: 8 }
                        }
                    }]
                },
                init: {
                    type: 'ObjectExpression',
                    properties: [],
                    range: [12, 14],
                    loc: {
                        start: { line: 1, column: 12 },
                        end: { line: 1, column: 14 }
                    }
                },
                range: [4, 14],
                loc: {
                    start: { line: 1, column: 4 },
                    end: { line: 1, column: 14 }
                }
            }],
            kind: 'let',
            range: [0, 14],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 14 }
            }
        },

        'var {a:b} = {}': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'ObjectPattern',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'a',
                            range: [5, 6],
                            loc: {
                                start: { line: 1, column: 5 },
                                end: { line: 1, column: 6 }
                            }
                        },
                        value: {
                            type: 'Identifier',
                            name: 'b',
                            range: [7, 8],
                            loc: {
                                start: { line: 1, column: 7 },
                                end: { line: 1, column: 8 }
                            }
                        },
                        kind: 'init',
                        range: [5, 8],
                        loc: {
                            start: { line: 1, column: 5 },
                            end: { line: 1, column: 8 }
                        }
                    }]
                },
                init: {
                    type: 'ObjectExpression',
                    properties: [],
                    range: [12, 14],
                    loc: {
                        start: { line: 1, column: 12 },
                        end: { line: 1, column: 14 }
                    }
                },
                range: [4, 14],
                loc: {
                    start: { line: 1, column: 4 },
                    end: { line: 1, column: 14 }
                }
            }],
            kind: 'var',
            range: [0, 14],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 14 }
            }
        }


    },

    // http://wiki.ecmascript.org/doku.php?id=harmony:modules

    'Harmony: Modules': {


        'module Universe { module MilkyWay {} }': {
            type: 'ModuleDeclaration',
            id: {
                type: 'Identifier',
                name: 'Universe',
                range: [7, 15],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 15 }
                }
            },
            body: {
                type: 'BlockStatement',
                body: [{
                    type: 'ModuleDeclaration',
                    id: {
                        type: 'Identifier',
                        name: 'MilkyWay',
                        range: [25, 33],
                        loc: {
                            start: { line: 1, column: 25 },
                            end: { line: 1, column: 33 }
                        }
                    },
                    body: {
                        type: 'BlockStatement',
                        body: [],
                        range: [34, 36],
                        loc: {
                            start: { line: 1, column: 34 },
                            end: { line: 1, column: 36 }
                        }
                    },
                    range: [18, 36],
                    loc: {
                        start: { line: 1, column: 18 },
                        end: { line: 1, column: 36 }
                    }
                }],
                range: [16, 38],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 38 }
                }
            },
            range: [0, 38],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 38 }
            }
        },

        'module MilkyWay = "Universe/MilkyWay"': {
            type: 'ModuleDeclaration',
            id: {
                type: 'Identifier',
                name: 'MilkyWay',
                range: [7, 15],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 15 }
                }
            },
            from: {
                type: 'Literal',
                value: 'Universe/MilkyWay',
                raw: '"Universe/MilkyWay"',
                range: [18, 37],
                loc: {
                    start: { line: 1, column: 18 },
                    end: { line: 1, column: 37 }
                }
            },
            range: [0, 37],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 37 }
            }
        },

        'module System = Universe.MilkyWay.SolarSystem': {
            type: 'ModuleDeclaration',
            id: {
                type: 'Identifier',
                name: 'System',
                range: [7, 13],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 13 }
                }
            },
            from: {
                type: 'Path',
                body: [{
                    type: 'Identifier',
                    name: 'Universe',
                    range: [16, 24],
                    loc: {
                        start: { line: 1, column: 16 },
                        end: { line: 1, column: 24 }
                    }
                }, {
                    type: 'Identifier',
                    name: 'MilkyWay',
                    range: [25, 33],
                    loc: {
                        start: { line: 1, column: 25 },
                        end: { line: 1, column: 33 }
                    }
                }, {
                    type: 'Identifier',
                    name: 'SolarSystem',
                    range: [34, 45],
                    loc: {
                        start: { line: 1, column: 34 },
                        end: { line: 1, column: 45 }
                    }
                }],
                range: [16, 45],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 45 }
                }
            },
            range: [0, 45],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 45 }
            }
        },

        'module System = SolarSystem': {
            type: 'ModuleDeclaration',
            id: {
                type: 'Identifier',
                name: 'System',
                range: [7, 13],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 13 }
                }
            },
            from: {
                type: 'Path',
                body: [{
                    type: 'Identifier',
                    name: 'SolarSystem',
                    range: [16, 27],
                    loc: {
                        start: { line: 1, column: 16 },
                        end: { line: 1, column: 27 }
                    }
                }],
                range: [16, 27],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 27 }
                }
            },
            range: [0, 27],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 27 }
            }
        },

        'export var document': {
            type: 'ExportDeclaration',
            declaration: {
                type: 'VariableDeclaration',
                declarations: [{
                    type: 'VariableDeclarator',
                    id: {
                        type: 'Identifier',
                        name: 'document',
                        range: [ 11, 19 ],
                        loc: {
                            start: { line: 1, column: 11 },
                            end: { line: 1, column: 19 }
                        }
                    },
                    init: null,
                    range: [ 11, 19 ],
                    loc: {
                        start: { line: 1, column: 11 },
                        end: { line: 1, column: 19 }
                    }
                }],
                kind: 'var',
                range: [ 7, 19 ],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 19 }
                }
            },
            range: [ 0, 19 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 19 }
            }
        },

        'export var document = { }': {
            type: 'ExportDeclaration',
            declaration: {
                type: 'VariableDeclaration',
                declarations: [{
                    type: 'VariableDeclarator',
                    id: {
                        type: 'Identifier',
                        name: 'document',
                        range: [ 11, 19 ],
                        loc: {
                            start: { line: 1, column: 11 },
                            end: { line: 1, column: 19 }
                        }
                    },
                    init: {
                        type: 'ObjectExpression',
                        properties: [],
                        range: [ 22, 25 ],
                        loc: {
                            start: { line: 1, column: 22 },
                            end: { line: 1, column: 25 }
                        }
                    },
                    range: [ 11, 25 ],
                    loc: {
                        start: { line: 1, column: 11 },
                        end: { line: 1, column: 25 }
                    }
                }],
                kind: 'var',
                range: [ 7, 25 ],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 25 }
                }
            },
            range: [ 0, 25 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 25 }
            }
        },

        'export let document': {
            type: 'ExportDeclaration',
            declaration: {
                type: 'VariableDeclaration',
                declarations: [{
                    type: 'VariableDeclarator',
                    id: {
                        type: 'Identifier',
                        name: 'document',
                        range: [ 11, 19 ],
                        loc: {
                            start: { line: 1, column: 11 },
                            end: { line: 1, column: 19 }
                        }
                    },
                    init: null,
                    range: [ 11, 19 ],
                    loc: {
                        start: { line: 1, column: 11 },
                        end: { line: 1, column: 19 }
                    }
                }],
                kind: 'let',
                range: [ 7, 19 ],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 19 }
                }
            },
            range: [ 0, 19 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 19 }
            }
        },

        'export let document = { }': {
            type: 'ExportDeclaration',
            declaration: {
                type: 'VariableDeclaration',
                declarations: [{
                    type: 'VariableDeclarator',
                    id: {
                        type: 'Identifier',
                        name: 'document',
                        range: [ 11, 19 ],
                        loc: {
                            start: { line: 1, column: 11 },
                            end: { line: 1, column: 19 }
                        }
                    },
                    init: {
                        type: 'ObjectExpression',
                        properties: [],
                        range: [ 22, 25 ],
                        loc: {
                            start: { line: 1, column: 22 },
                            end: { line: 1, column: 25 }
                        }
                    },
                    range: [ 11, 25 ],
                    loc: {
                        start: { line: 1, column: 11 },
                        end: { line: 1, column: 25 }
                    }
                }],
                kind: 'let',
                range: [ 7, 25 ],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 25 }
                }
            },
            range: [ 0, 25 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 25 }
            }
        },

        'export const document = { }': {
            type: 'ExportDeclaration',
            declaration: {
                type: 'VariableDeclaration',
                declarations: [{
                    type: 'VariableDeclarator',
                    id: {
                        type: 'Identifier',
                        name: 'document',
                        range: [ 13, 21 ],
                        loc: {
                            start: { line: 1, column: 13 },
                            end: { line: 1, column: 21 }
                        }
                    },
                    init: {
                        type: 'ObjectExpression',
                        properties: [],
                        range: [ 24, 27 ],
                        loc: {
                            start: { line: 1, column: 24 },
                            end: { line: 1, column: 27 }
                        }
                    },
                    range: [ 13, 27 ],
                    loc: {
                        start: { line: 1, column: 13 },
                        end: { line: 1, column: 27 }
                    }
                }],
                kind: 'const',
                range: [ 7, 27 ],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 27 }
                }
            },
            range: [ 0, 27 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 27 }
            }
        },

        'export function parse() { }': {
            type: 'ExportDeclaration',
            declaration: {
                type: 'FunctionDeclaration',
                id: {
                    type: 'Identifier',
                    name: 'parse',
                    range: [ 16, 21 ],
                    loc: {
                        start: { line: 1, column: 16 },
                        end: { line: 1, column: 21 }
                    }
                },
                params: [],
                defaults: [],
                body: {
                    type: 'BlockStatement',
                    body: [],
                    range: [ 24, 27 ],
                    loc: {
                        start: { line: 1, column: 24 },
                        end: { line: 1, column: 27 }
                    }
                },
                rest: null,
                generator: false,
                expression: false,
                range: [ 7, 27 ],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 27 }
                }
            },
            range: [ 0, 27 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 27 }
            }
        },

        'export module System = SolarSystem': {
            type: 'ExportDeclaration',
            declaration: {
                type: 'ModuleDeclaration',
                id: {
                    type: 'Identifier',
                    name: 'System',
                    range: [14, 20],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 20 }
                    }
                },
                from: {
                    type: 'Path',
                    body: [{
                        type: 'Identifier',
                        name: 'SolarSystem',
                        range: [23, 34],
                        loc: {
                            start: { line: 1, column: 23 },
                            end: { line: 1, column: 34 }
                        }
                    }],
                    range: [23, 34],
                    loc: {
                        start: { line: 1, column: 23 },
                        end: { line: 1, column: 34 }
                    }
                },
                range: [7, 34],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 34 }
                }
            },
            range: [ 0, 34 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 34 }
            }
        },

        'export SolarSystem': {
            type: 'ExportDeclaration',
            specifiers: [{
                type: 'ExportSpecifier',
                id: {
                    type: 'Identifier',
                    name: 'SolarSystem',
                    range: [7, 18],
                    loc: {
                        start: { line: 1, column: 7 },
                        end: { line: 1, column: 18 }
                    }
                },
                from: null,
                range: [7, 18],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 18 }
                }
            }],
            range: [ 0, 18 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 18 }
            }
        },

        'export Mercury, Venus, Earth': {
            type: 'ExportDeclaration',
            specifiers: [{
                type: 'ExportSpecifier',
                id: {
                    type: 'Identifier',
                    name: 'Mercury',
                    range: [7, 14],
                    loc: {
                        start: { line: 1, column: 7 },
                        end: { line: 1, column: 14 }
                    }
                },
                from: null,
                range: [7, 14],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 14 }
                }
            }, {
                type: 'ExportSpecifier',
                id: {
                    type: 'Identifier',
                    name: 'Venus',
                    range: [16, 21],
                    loc: {
                        start: { line: 1, column: 16 },
                        end: { line: 1, column: 21 }
                    }
                },
                from: null,
                range: [16, 21],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 21 }
                }
            }, {
                type: 'ExportSpecifier',
                id: {
                    type: 'Identifier',
                    name: 'Earth',
                    range: [23, 28],
                    loc: {
                        start: { line: 1, column: 23 },
                        end: { line: 1, column: 28 }
                    }
                },
                from: null,
                range: [23, 28],
                loc: {
                    start: { line: 1, column: 23 },
                    end: { line: 1, column: 28 }
                }
            }],
            range: [ 0, 28 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 28 }
            }
        },

        'export *': {
            type: 'ExportDeclaration',
            specifiers: [{
                type: 'ExportSpecifier',
                id: {
                    type: 'Glob',
                    range: [7, 8],
                    loc: {
                        start: { line: 1, column: 7 },
                        end: { line: 1, column: 8 }
                    }
                },
                from: null,
                range: [7, 8],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 8 }
                }
            }],
            range: [ 0, 8 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 8 }
            }
        },

        'export * from SolarSystem': {
            type: 'ExportDeclaration',
            specifiers: [{
                type: 'ExportSpecifier',
                id: {
                    type: 'Glob',
                    range: [7, 8],
                    loc: {
                        start: { line: 1, column: 7 },
                        end: { line: 1, column: 8 }
                    }
                },
                from: {
                    type: 'Path',
                    body: [{
                        type: 'Identifier',
                        name: 'SolarSystem',
                        range: [ 14, 25 ],
                        loc: {
                            start: { line: 1, column: 14 },
                            end: { line: 1, column: 25 }
                        }
                    }],
                    range: [ 14, 25 ],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 25 }
                    }
                },
                range: [ 7, 25 ],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 25 }
                }
            }],
            range: [ 0, 25 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 25 }
            }
        },

        'export { Mercury: SolarSystem.Mercury, Earth: SolarSystem.Earth }': {
            type: 'ExportDeclaration',
            specifiers: [{
                type: 'ExportSpecifierSet',
                specifiers: [
                    {
                        type: 'ExportSpecifier',
                        id: {
                            type: 'Identifier',
                            name: 'Mercury',
                            range: [ 9, 16 ],
                            loc: {
                                start: { line: 1, column: 9 },
                                end: { line: 1, column: 16 }
                            }
                        },
                        from: {
                            type: 'Path',
                            body: [
                                {
                                    type: 'Identifier',
                                    name: 'SolarSystem',
                                    range: [ 18, 29 ],
                                    loc: {
                                        start: { line: 1, column: 18 },
                                        end: { line: 1, column: 29 }
                                    }
                                },
                                {
                                    type: 'Identifier',
                                    name: 'Mercury',
                                    range: [ 30, 37 ],
                                    loc: {
                                        start: { line: 1, column: 30 },
                                        end: { line: 1, column: 37 }
                                    }
                                }
                            ],
                            range: [ 18, 37 ],
                            loc: {
                                start: { line: 1, column: 18 },
                                end: { line: 1, column: 37 }
                            }
                        },
                        range: [ 9, 37 ],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 37 }
                        }
                    },
                    {
                        type: 'ExportSpecifier',
                        id: {
                            type: 'Identifier',
                            name: 'Earth',
                            range: [ 39, 44 ],
                            loc: {
                                start: { line: 1, column: 39 },
                                end: { line: 1, column: 44 }
                            }
                        },
                        from: {
                            type: 'Path',
                            body: [
                                {
                                    type: 'Identifier',
                                    name: 'SolarSystem',
                                    range: [ 46, 57 ],
                                    loc: {
                                        start: { line: 1, column: 46 },
                                        end: { line: 1, column: 57 }
                                    }
                                },
                                {
                                    type: 'Identifier',
                                    name: 'Earth',
                                    range: [ 58, 63 ],
                                    loc: {
                                        start: { line: 1, column: 58 },
                                        end: { line: 1, column: 63 }
                                    }
                                }
                            ],
                            range: [ 46, 63 ],
                            loc: {
                                start: { line: 1, column: 46 },
                                end: { line: 1, column: 63 }
                            }
                        },
                        range: [ 39, 63 ],
                        loc: {
                            start: { line: 1, column: 39 },
                            end: { line: 1, column: 63 }
                        }
                    }
                ],
                range: [7, 65],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 65 }
                }
            }],
            range: [0, 65],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 65 }
            }
        },

        'module foo {\n module bar = baz }': {
            type: "ModuleDeclaration",
            id: {
                type: "Identifier",
                name: "foo",
                range: [7, 10],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 10 }
                }
            },
            body: {
                type: "BlockStatement",
                body: [
                    {
                        type: "ModuleDeclaration",
                        id: {
                            type: "Identifier",
                            name: "bar",
                            range: [21, 24],
                            loc: {
                                start: { line: 2, column: 8 },
                                end: { line: 2, column: 11 }
                            }
                        },
                        from: {
                            type: "Path",
                            body: [
                                {
                                    type: "Identifier",
                                    name: "baz",
                                    range: [27, 30],
                                    loc: {
                                        start: { line: 2, column: 14 },
                                        end: { line: 2, column: 17 }
                                    }
                                }
                            ],
                            range: [27, 30],
                            loc: {
                                start: { line: 2, column: 14 },
                                end: { line: 2, column: 17 }
                            }
                        },
                        range: [14, 31],
                        loc: {
                            start: { line: 2, column: 1 },
                            end: { line: 2, column: 18 }
                        }
                    }
                ],
                range: [11, 32],
                loc: {
                    start: { line: 1, column: 11 },
                    end: { line: 2, column: 19 }
                }
            },
            range: [0, 32],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 2, column: 19 }
            }
        },

        'import * from foo': {
            type: 'ImportDeclaration',
            specifiers: [{
                type: 'Glob',
                range: [7, 8],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 8 }
                }
            }],
            from: {
                type: 'Path',
                body: [{
                    type: 'Identifier',
                    name: 'foo',
                    range: [14, 17],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 17 }
                    }
                }],
                range: [14, 17],
                loc: {
                    start: { line: 1, column: 14 },
                    end: { line: 1, column: 17 }
                }
            },
            range: [0, 17],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 17 }
            }
        },

        'import * from \'SolarSystem.js\'': {
            type: 'ImportDeclaration',
            specifiers: [{
                type: 'Glob',
                range: [7, 8],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 8 }
                }
            }],
            from: {
                type: 'Literal',
                value: 'SolarSystem.js',
                raw: '\'SolarSystem.js\'',
                range: [14, 30],
                loc: {
                    start: { line: 1, column: 14 },
                    end: { line: 1, column: 30 }
                }
            },
            range: [0, 30],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 30 }
            }
        },

        'import foo from bar': {
            type: 'ImportDeclaration',
            specifiers: [{
                type: 'Identifier',
                name: 'foo',
                range: [7, 10],
                loc: {
                    start: { line: 1, column: 7 },
                    end: { line: 1, column: 10 }
                }
            }],
            from: {
                type: 'Path',
                body: [{
                    type: 'Identifier',
                    name: 'bar',
                    range: [16, 19],
                    loc: {
                        start: { line: 1, column: 16 },
                        end: { line: 1, column: 19 }
                    }
                }],
                range: [16, 19],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 19 }
                }
            },
            range: [0, 19],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 19 }
            }
        },

        'import { foo } from bar': {
            type: 'ImportDeclaration',
            specifiers: [{
                type: 'ImportSpecifier',
                id: {
                    type: 'Identifier',
                    name: 'foo',
                    range: [9, 12],
                    loc: {
                        start: { line: 1, column: 9 },
                        end: { line: 1, column: 12 }
                    }
                },
                from: null,
                range: [9, 12],
                loc: {
                    start: { line: 1, column: 9 },
                    end: { line: 1, column: 12 }
                }
            }],
            from: {
                type: 'Path',
                body: [{
                    type: 'Identifier',
                    name: 'bar',
                    range: [20, 23],
                    loc: {
                        start: { line: 1, column: 20 },
                        end: { line: 1, column: 23 }
                    }
                }],
                range: [20, 23],
                loc: {
                    start: { line: 1, column: 20 },
                    end: { line: 1, column: 23 }
                }
            },
            range: [0, 23],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 23 }
            }
        },

        'import { foo: bar, baz: quux } from quuux': {
            type: 'ImportDeclaration',
            specifiers: [{
                type: 'ImportSpecifier',
                id: {
                    type: 'Identifier',
                    name: 'foo',
                    range: [9, 12],
                    loc: {
                        start: { line: 1, column: 9 },
                        end: { line: 1, column: 12 }
                    }
                },
                from: {
                    type: 'Path',
                    body: [{
                        type: 'Identifier',
                        name: 'bar',
                        range: [14, 17],
                        loc: {
                            start: { line: 1, column: 14 },
                            end: { line: 1, column: 17 }
                        }
                    }],
                    range: [14, 17],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 17 }
                    }
                },
                range: [9, 17],
                loc: {
                    start: { line: 1, column: 9 },
                    end: { line: 1, column: 17 }
                }
            }, {
                type: 'ImportSpecifier',
                id: {
                    type: 'Identifier',
                    name: 'baz',
                    range: [19, 22],
                    loc: {
                        start: { line: 1, column: 19 },
                        end: { line: 1, column: 22 }
                    }
                },
                from: {
                    type: 'Path',
                    body: [{
                        type: 'Identifier',
                        name: 'quux',
                        range: [24, 28],
                        loc: {
                            start: { line: 1, column: 24 },
                            end: { line: 1, column: 28 }
                        }
                    }],
                    range: [24, 28],
                    loc: {
                        start: { line: 1, column: 24 },
                        end: { line: 1, column: 28 }
                    }
                },
                range: [19, 28],
                loc: {
                    start: { line: 1, column: 19 },
                    end: { line: 1, column: 28 }
                }
            }],
            from: {
                type: 'Path',
                body: [{
                    type: 'Identifier',
                    name: 'quuux',
                    range: [36, 41],
                    loc: {
                        start: { line: 1, column: 36 },
                        end: { line: 1, column: 41 }
                    }
                }],
                range: [36, 41],
                loc: {
                    start: { line: 1, column: 36 },
                    end: { line: 1, column: 41 }
                }
            },
            range: [0, 41],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 41 }
            }
        },

        'module\n X = Y': {
            type: "ExpressionStatement",
            expression: {
                type: "Identifier",
                name: "module",
                range: [0, 6],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 6 }
                }
            },
            range: [0, 8],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 2, column: 1 }
            }
        },

        'module.export = Foo': {
            type: "ExpressionStatement",
            expression: {
                type: "AssignmentExpression",
                operator: "=",
                left: {
                    type: "MemberExpression",
                    computed: false,
                    object: {
                        type: "Identifier",
                        name: "module",
                        range: [0, 6],
                        loc: {
                            start: { line: 1, column: 0 },
                            end: { line: 1, column: 6 }
                        }
                    },
                    property: {
                        type: "Identifier",
                        name: "export",
                        range: [7, 13],
                        loc: {
                            start: { line: 1, column: 7 },
                            end: { line: 1, column: 13 }
                        }
                    },
                    range: [0, 13],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 13 }
                    }
                },
                right: {
                    type: "Identifier",
                    name: "Foo",
                    range: [16, 19],
                    loc: {
                        start: { line: 1, column: 16 },
                        end: { line: 1, column: 19 }
                    }
                },
                range: [0, 19],
                loc: {
                    start: { line: 1, column: 0 },
                    end: { line: 1, column: 19 }
                }
            },
            range: [0, 19],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 19 }
            }
        }
    },


    // http://wiki.ecmascript.org/doku.php?id=harmony:generators

    'Harmony: Yield Expression': {
        '(function* () { yield v })': {
            type: 'ExpressionStatement',
            expression: {
                type: 'FunctionExpression',
                id: null,
                params: [],
                defaults: [],
                body: {
                    type: 'BlockStatement',
                    body: [{
                        type: 'ExpressionStatement',
                        expression: {
                            type: 'YieldExpression',
                            argument: {
                                type: 'Identifier',
                                name: 'v',
                                range: [22, 23],
                                loc: {
                                    start: { line: 1, column: 22 },
                                    end: { line: 1, column: 23 }
                                }
                            },
                            delegate: false,
                            range: [16, 23],
                            loc: {
                                start: { line: 1, column: 16 },
                                end: { line: 1, column: 23 }
                            }
                        },
                        range: [16, 24],
                        loc: {
                            start: { line: 1, column: 16 },
                            end: { line: 1, column: 24 }
                        }
                    }],
                    range: [14, 25],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 25 }
                    }
                },
                rest: null,
                generator: true,
                expression: false,
                range: [1, 25],
                loc: {
                    start: { line: 1, column: 1 },
                    end: { line: 1, column: 25 }
                }
            },
            range: [0, 26],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 26 }
            }
        },

        '(function* () { yield *v })': {
            type: 'ExpressionStatement',
            expression: {
                type: 'FunctionExpression',
                id: null,
                params: [],
                defaults: [],
                body: {
                    type: 'BlockStatement',
                    body: [{
                        type: 'ExpressionStatement',
                        expression: {
                            type: 'YieldExpression',
                            argument: {
                                type: 'Identifier',
                                name: 'v',
                                range: [23, 24],
                                loc: {
                                    start: { line: 1, column: 23 },
                                    end: { line: 1, column: 24 }
                                }
                            },
                            delegate: true,
                            range: [16, 24],
                            loc: {
                                start: { line: 1, column: 16 },
                                end: { line: 1, column: 24 }
                            }
                        },
                        range: [16, 25],
                        loc: {
                            start: { line: 1, column: 16 },
                            end: { line: 1, column: 25 }
                        }
                    }],
                    range: [14, 26],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 26 }
                    }
                },
                rest: null,
                generator: true,
                expression: false,
                range: [1, 26],
                loc: {
                    start: { line: 1, column: 1 },
                    end: { line: 1, column: 26 }
                }
            },
            range: [0, 27],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 27 }
            }
        },

        'function* test () { yield *v }': {
            type: 'FunctionDeclaration',
            id: {
                type: 'Identifier',
                name: 'test',
                range: [10, 14],
                loc: {
                    start: { line: 1, column: 10 },
                    end: { line: 1, column: 14}
                }
            },
            params: [],
            defaults: [],
            body: {
                type: 'BlockStatement',
                body: [{
                    type: 'ExpressionStatement',
                    expression: {
                        type: 'YieldExpression',
                        argument: {
                            type: 'Identifier',
                            name: 'v',
                            range: [27, 28],
                            loc: {
                                start: { line: 1, column: 27 },
                                end: { line: 1, column: 28 }
                            }
                        },
                        delegate: true,
                        range: [20, 28],
                        loc: {
                            start: { line: 1, column: 20 },
                            end: { line: 1, column: 28 }
                        }
                    },
                    range: [20, 29],
                    loc: {
                        start: { line: 1, column: 20 },
                        end: { line: 1, column: 29 }
                    }
                }],
                range: [18, 30],
                loc: {
                    start: { line: 1, column: 18 },
                    end: { line: 1, column: 30 }
                }
            },
            rest: null,
            generator: true,
            expression: false,
            range: [0, 30],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 30 }
            }
        },

        'var x = { *test () { yield *v } };': {
            type: 'VariableDeclaration',
            declarations: [{
                type: 'VariableDeclarator',
                id: {
                    type: 'Identifier',
                    name: 'x',
                    range: [4, 5],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 5 }
                    }
                },
                init: {
                    type: 'ObjectExpression',
                    properties: [{
                        type: 'Property',
                        key: {
                            type: 'Identifier',
                            name: 'test',
                            range: [11, 15],
                            loc: {
                                start: { line: 1, column: 11 },
                                end: { line: 1, column: 15 }
                            }
                        },
                        value: {
                            type: 'FunctionExpression',
                            id: null,
                            params: [],
                            defaults: [],
                            body: {
                                type: 'BlockStatement',
                                body: [{
                                    type: 'ExpressionStatement',
                                    expression: {
                                        type: 'YieldExpression',
                                        argument: {
                                            type: 'Identifier',
                                            name: 'v',
                                            range: [28, 29],
                                            loc: {
                                                start: { line: 1, column: 28 },
                                                end: { line: 1, column: 29 }
                                            }
                                        },
                                        delegate: true,
                                        range: [21, 29],
                                        loc: {
                                            start: { line: 1, column: 21 },
                                            end: { line: 1, column: 29 }
                                        }
                                    },
                                    range: [21, 30],
                                    loc: {
                                        start: { line: 1, column: 21 },
                                        end: { line: 1, column: 30 }
                                    }
                                }],
                                range: [19, 31],
                                loc: {
                                    start: { line: 1, column: 19 },
                                    end: { line: 1, column: 31 }
                                }
                            },
                            rest: null,
                            generator: true,
                            expression: false,
                            range: [19, 31],
                            loc: {
                                start: { line: 1, column: 19 },
                                end: { line: 1, column: 31 }
                            }
                        },
                        kind: 'init',
                        method: true,
                        range: [10, 31],
                        loc: {
                            start: { line: 1, column: 10 },
                            end: { line: 1, column: 31 }
                        }
                    }],
                    range: [8, 33],
                    loc: {
                        start: { line: 1, column: 8 },
                        end: { line: 1, column: 33 }
                    }
                },
                range: [4, 33],
                loc: {
                    start: { line: 1, column: 4 },
                    end: { line: 1, column: 33 }
                }
            }],
            kind: 'var',
            range: [0, 34],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 34 }
            }
        }

    },



    // http://wiki.ecmascript.org/doku.php?id=harmony:iterators

    'Harmony: Iterators': {

        'for(x of list) process(x);': {
            type: 'ForOfStatement',
            left: {
                type: 'Identifier',
                name: 'x',
                range: [4, 5],
                loc: {
                    start: { line: 1, column: 4 },
                    end: { line: 1, column: 5 }
                }
            },
            right: {
                type: 'Identifier',
                name: 'list',
                range: [9, 13],
                loc: {
                    start: { line: 1, column: 9 },
                    end: { line: 1, column: 13 }
                }
            },
            body: {
                type: 'ExpressionStatement',
                expression: {
                    type: 'CallExpression',
                    callee: {
                        type: 'Identifier',
                        name: 'process',
                        range: [15, 22],
                        loc: {
                            start: { line: 1, column: 15 },
                            end: { line: 1, column: 22 }
                        }
                    },
                    'arguments': [{
                        type: 'Identifier',
                        name: 'x',
                        range: [23, 24],
                        loc: {
                            start: { line: 1, column: 23 },
                            end: { line: 1, column: 24 }
                        }
                    }],
                    range: [15, 25],
                    loc: {
                        start: { line: 1, column: 15 },
                        end: { line: 1, column: 25 }
                    }
                },
                range: [15, 26],
                loc: {
                    start: { line: 1, column: 15 },
                    end: { line: 1, column: 26 }
                }
            },
            range: [0, 26],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 26 }
            }
        },

        'for (var x of list) process(x);': {
            type: 'ForOfStatement',
            left: {
                type: 'VariableDeclaration',
                declarations: [{
                    type: 'VariableDeclarator',
                    id: {
                        type: 'Identifier',
                        name: 'x',
                        range: [9, 10],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 10 }
                        }
                    },
                    init: null,
                    range: [9, 10],
                    loc: {
                        start: { line: 1, column: 9 },
                        end: { line: 1, column: 10 }
                    }
                }],
                kind: 'var',
                range: [5, 10],
                loc: {
                    start: { line: 1, column: 5 },
                    end: { line: 1, column: 10 }
                }
            },
            right: {
                type: 'Identifier',
                name: 'list',
                range: [14, 18],
                loc: {
                    start: { line: 1, column: 14 },
                    end: { line: 1, column: 18 }
                }
            },
            body: {
                type: 'ExpressionStatement',
                expression: {
                    type: 'CallExpression',
                    callee: {
                        type: 'Identifier',
                        name: 'process',
                        range: [20, 27],
                        loc: {
                            start: { line: 1, column: 20 },
                            end: { line: 1, column: 27 }
                        }
                    },
                    'arguments': [{
                        type: 'Identifier',
                        name: 'x',
                        range: [28, 29],
                        loc: {
                            start: { line: 1, column: 28 },
                            end: { line: 1, column: 29 }
                        }
                    }],
                    range: [20, 30],
                    loc: {
                        start: { line: 1, column: 20 },
                        end: { line: 1, column: 30 }
                    }
                },
                range: [20, 31],
                loc: {
                    start: { line: 1, column: 20 },
                    end: { line: 1, column: 31 }
                }
            },
            range: [0, 31],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 31 }
            }
        },

        'for (var x = 42 of list) process(x);': {
            type: 'ForOfStatement',
            left: {
                type: 'VariableDeclaration',
                declarations: [{
                    type: 'VariableDeclarator',
                    id: {
                        type: 'Identifier',
                        name: 'x',
                        range: [9, 10],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 10 }
                        }
                    },
                    init: {
                        type: 'Literal',
                        value: 42,
                        raw: '42',
                        range: [13, 15],
                        loc: {
                            start: { line: 1, column: 13 },
                            end: { line: 1, column: 15 }
                        }
                    },
                    range: [9, 15],
                    loc: {
                        start: { line: 1, column: 9 },
                        end: { line: 1, column: 15 }
                    }
                }],
                kind: 'var',
                range: [5, 15],
                loc: {
                    start: { line: 1, column: 5 },
                    end: { line: 1, column: 15 }
                }
            },
            right: {
                type: 'Identifier',
                name: 'list',
                range: [19, 23],
                loc: {
                    start: { line: 1, column: 19 },
                    end: { line: 1, column: 23 }
                }
            },
            body: {
                type: 'ExpressionStatement',
                expression: {
                    type: 'CallExpression',
                    callee: {
                        type: 'Identifier',
                        name: 'process',
                        range: [25, 32],
                        loc: {
                            start: { line: 1, column: 25 },
                            end: { line: 1, column: 32 }
                        }
                    },
                    'arguments': [{
                        type: 'Identifier',
                        name: 'x',
                        range: [33, 34],
                        loc: {
                            start: { line: 1, column: 33 },
                            end: { line: 1, column: 34 }
                        }
                    }],
                    range: [25, 35],
                    loc: {
                        start: { line: 1, column: 25 },
                        end: { line: 1, column: 35 }
                    }
                },
                range: [25, 36],
                loc: {
                    start: { line: 1, column: 25 },
                    end: { line: 1, column: 36 }
                }
            },
            range: [0, 36],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 36 }
            }
        },

        'for (let x of list) process(x);': {
            type: 'ForOfStatement',
            left: {
                type: 'VariableDeclaration',
                declarations: [{
                    type: 'VariableDeclarator',
                    id: {
                        type: 'Identifier',
                        name: 'x',
                        range: [9, 10],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 10 }
                        }
                    },
                    init: null,
                    range: [9, 10],
                    loc: {
                        start: { line: 1, column: 9 },
                        end: { line: 1, column: 10 }
                    }
                }],
                kind: 'let',
                range: [5, 10],
                loc: {
                    start: { line: 1, column: 5 },
                    end: { line: 1, column: 10 }
                }
            },
            right: {
                type: 'Identifier',
                name: 'list',
                range: [14, 18],
                loc: {
                    start: { line: 1, column: 14 },
                    end: { line: 1, column: 18 }
                }
            },
            body: {
                type: 'ExpressionStatement',
                expression: {
                    type: 'CallExpression',
                    callee: {
                        type: 'Identifier',
                        name: 'process',
                        range: [20, 27],
                        loc: {
                            start: { line: 1, column: 20 },
                            end: { line: 1, column: 27 }
                        }
                    },
                    'arguments': [{
                        type: 'Identifier',
                        name: 'x',
                        range: [28, 29],
                        loc: {
                            start: { line: 1, column: 28 },
                            end: { line: 1, column: 29 }
                        }
                    }],
                    range: [20, 30],
                    loc: {
                        start: { line: 1, column: 20 },
                        end: { line: 1, column: 30 }
                    }
                },
                range: [20, 31],
                loc: {
                    start: { line: 1, column: 20 },
                    end: { line: 1, column: 31 }
                }
            },
            range: [0, 31],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 31 }
            }
        }

    },


    // http://wiki.ecmascript.org/doku.php?id=strawman:maximally_minimal_classes

    'Harmony: Class (strawman)': {

        'var A = class extends B {}': {
            type: "VariableDeclaration",
            declarations: [
                {
                    type: "VariableDeclarator",
                    id: {
                        type: "Identifier",
                        name: "A",
                        range: [4, 5],
                        loc: {
                            start: { line: 1, column: 4 },
                            end: { line: 1, column: 5 }
                        }
                    },
                    init: {
                        type: "ClassExpression",
                        body: {
                            type: "ClassBody",
                            body: [],
                            range: [24, 26],
                            loc: {
                                start: { line: 1, column: 24 },
                                end: { line: 1, column: 26 }
                            }
                        },
                        superClass: {
                            type: "Identifier",
                            name: "B",
                            range: [22, 23],
                            loc: {
                                start: { line: 1, column: 22 },
                                end: { line: 1, column: 23 }
                            }
                        },
                        range: [8, 26],
                        loc: {
                            start: { line: 1, column: 8 },
                            end: { line: 1, column: 26 }
                        }
                    },
                    range: [4, 26],
                    loc: {
                        start: { line: 1, column: 4 },
                        end: { line: 1, column: 26 }
                    }
                }
            ],
            kind: "var",
            range: [0, 26],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 26 }
            }
        },

        'class A extends class B extends C {} {}': {
            id: {
                type: "Identifier",
                name: "A",
                range: [6, 7],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 7 }
                }
            },
            type: "ClassDeclaration",
            body: {
                type: "ClassBody",
                body: [],
                range: [37, 39],
                loc: {
                    start: { line: 1, column: 37 },
                    end: { line: 1, column: 39 }
                }
            },
            superClass: {
                id: {
                    type: "Identifier",
                    name: "B",
                    range: [22, 23],
                    loc: {
                        start: { line: 1, column: 22 },
                        end: { line: 1, column: 23 }
                    }
                },
                type: "ClassExpression",
                body: {
                    type: "ClassBody",
                    body: [],
                    range: [34, 36],
                    loc: {
                        start: { line: 1, column: 34 },
                        end: { line: 1, column: 36 }
                    }
                },
                superClass: {
                    type: "Identifier",
                    name: "C",
                    range: [32, 33],
                    loc: {
                        start: { line: 1, column: 32 },
                        end: { line: 1, column: 33 }
                    }
                },
                range: [16, 36],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 36 }
                }
            },
            range: [0, 39],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 39 }
            }
        },

        'class A {get() {}}': {
            id: {
                type: "Identifier",
                name: "A",
                range: [6, 7],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 7 }
                }
            },
            type: "ClassDeclaration",
            body: {
                type: "ClassBody",
                body: [
                    {
                        type: "MethodDefinition",
                        key: {
                            type: "Identifier",
                            name: "get",
                            range: [9, 12],
                            loc: {
                                start: { line: 1, column: 9 },
                                end: { line: 1, column: 12 }
                            }
                        },
                        value: {
                            type: "FunctionExpression",
                            id: null,
                            params: [],
                            defaults: [],
                            body: {
                                type: "BlockStatement",
                                body: [],
                                range: [15, 17],
                                loc: {
                                    start: { line: 1, column: 15 },
                                    end: { line: 1, column: 17 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: false,
                            range: [15, 17],
                            loc: {
                                start: { line: 1, column: 15 },
                                end: { line: 1, column: 17 }
                            }
                        },
                        kind: "",
                        range: [9, 17],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 17 }
                        }
                    }
                ],
                range: [8, 18],
                loc: {
                    start: { line: 1, column: 8 },
                    end: { line: 1, column: 18 }
                }
            },
            range: [0, 18],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 18 }
            }
        },

        'class A extends B {get foo() {}}': {
            id: {
                type: "Identifier",
                name: "A",
                range: [6, 7],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 7 }
                }
            },
            type: "ClassDeclaration",
            body: {
                type: "ClassBody",
                body: [{
                    type: "MethodDefinition",
                    key: {
                        type: "Identifier",
                        name: "foo",
                        range: [23, 26],
                        loc: {
                            start: { line: 1, column: 23 },
                            end: { line: 1, column: 26 }
                        }
                    },
                    value: {
                        type: "FunctionExpression",
                        id: null,
                        params: [],
                        defaults: [],
                        body: {
                            type: "BlockStatement",
                            body: [],
                            range: [29, 31],
                            loc: {
                                start: { line: 1, column: 29 },
                                end: { line: 1, column: 31 }
                            }
                        },
                        rest: null,
                        generator: false,
                        expression: false,
                        range: [29, 31],
                        loc: {
                            start: { line: 1, column: 29 },
                            end: { line: 1, column: 31 }
                        }
                    },
                    kind: "get",
                    range: [19, 31],
                    loc: {
                        start: { line: 1, column: 19 },
                        end: { line: 1, column: 31 }
                    }
                }],
                range: [18, 32],
                loc: {
                    start: { line: 1, column: 18 },
                    end: { line: 1, column: 32 }
                }
            },
            superClass: {
                type: "Identifier",
                name: "B",
                range: [16, 17],
                loc: {
                    start: { line: 1, column: 16 },
                    end: { line: 1, column: 17 }
                }
            },
            range: [0, 32],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 32 }
            }
        },

        'class A {set a(v) {}}': {
            id: {
                type: "Identifier",
                name: "A",
                range: [6, 7],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 7 }
                }
            },
            type: "ClassDeclaration",
            body: {
                type: "ClassBody",
                body: [
                    {
                        type: "MethodDefinition",
                        key: {
                            type: 'Identifier',
                            name: 'a',
                            range: [13, 14],
                            loc: {
                                start: { line: 1, column: 13 },
                                end: { line: 1, column: 14 }
                            }
                        },
                        value: {
                            type: "FunctionExpression",
                            id: null,
                            params: [{
                                type: 'Identifier',
                                name: 'v',
                                range: [15, 16],
                                loc: {
                                    start: { line: 1, column: 15 },
                                    end: { line: 1, column: 16 }
                                }
                            }],
                            defaults: [],
                            body: {
                                type: "BlockStatement",
                                body: [],
                                range: [18, 20],
                                loc: {
                                    start: { line: 1, column: 18 },
                                    end: { line: 1, column: 20 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: false,
                            range: [18, 20],
                            loc: {
                                start: { line: 1, column: 18 },
                                end: { line: 1, column: 20 }
                            }
                        },
                        kind: "set",
                        range: [9, 20],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 20 }
                        }
                    }
                ],
                range: [8, 21],
                loc: {
                    start: { line: 1, column: 8 },
                    end: { line: 1, column: 21 }
                }
            },
            range: [0, 21],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 21 }
            }
        },

        'class A {set(v) {};}': {
            id: {
                type: "Identifier",
                name: "A",
                range: [6, 7],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 7 }
                }
            },
            type: "ClassDeclaration",
            body: {
                type: "ClassBody",
                body: [
                    {
                        type: "MethodDefinition",
                        key: {
                            type: "Identifier",
                            name: "set",
                            range: [9, 12],
                            loc: {
                                start: { line: 1, column: 9 },
                                end: { line: 1, column: 12 }
                            }
                        },
                        value: {
                            type: "FunctionExpression",
                            id: null,
                            params: [{
                                type: 'Identifier',
                                name: 'v',
                                range: [13, 14],
                                loc: {
                                    start: { line: 1, column: 13 },
                                    end: { line: 1, column: 14 }
                                }
                            }],
                            defaults: [],
                            body: {
                                type: "BlockStatement",
                                body: [],
                                range: [16, 18],
                                loc: {
                                    start: { line: 1, column: 16 },
                                    end: { line: 1, column: 18 }
                                }
                            },
                            rest: null,
                            generator: false,
                            expression: false,
                            range: [16, 18],
                            loc: {
                                start: { line: 1, column: 16 },
                                end: { line: 1, column: 18 }
                            }
                        },
                        kind: "",
                        range: [9, 18],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 18 }
                        }
                    }
                ],
                range: [8, 20],
                loc: {
                    start: { line: 1, column: 8 },
                    end: { line: 1, column: 20 }
                }
            },
            range: [0, 20],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 20 }
            }
        },

        'class A {*gen(v) { yield v; }}': {
            id: {
                type: "Identifier",
                name: "A",
                range: [6, 7],
                loc: {
                    start: { line: 1, column: 6 },
                    end: { line: 1, column: 7 }
                }
            },
            type: "ClassDeclaration",
            body: {
                type: "ClassBody",
                body: [
                    {
                        type: "MethodDefinition",
                        key: {
                            type: "Identifier",
                            name: "gen",
                            range: [10, 13],
                            loc: {
                                start: { line: 1, column: 10 },
                                end: { line: 1, column: 13 }
                            }
                        },
                        value: {
                            type: "FunctionExpression",
                            id: null,
                            params: [{
                                type: 'Identifier',
                                name: 'v',
                                range: [14, 15],
                                loc: {
                                    start: { line: 1, column: 14 },
                                    end: { line: 1, column: 15 }
                                }
                            }],
                            defaults: [],
                            body: {
                                type: "BlockStatement",
                                body: [{
                                    type: 'ExpressionStatement',
                                    expression: {
                                        type: 'YieldExpression',
                                        argument: {
                                            type: 'Identifier',
                                            name: 'v',
                                            range: [25, 26],
                                            loc: {
                                                start: { line: 1, column: 25 },
                                                end: { line: 1, column: 26 }
                                            }
                                        },
                                        delegate: false,
                                        range: [19, 26],
                                        loc: {
                                            start: { line: 1, column: 19 },
                                            end: { line: 1, column: 26 }
                                        }
                                    },
                                    range: [19, 27],
                                    loc: {
                                        start: { line: 1, column: 19 },
                                        end: { line: 1, column: 27 }
                                    }
                                }],
                                range: [17, 29],
                                loc: {
                                    start: { line: 1, column: 17 },
                                    end: { line: 1, column: 29 }
                                }
                            },
                            rest: null,
                            generator: true,
                            expression: false,
                            range: [17, 29],
                            loc: {
                                start: { line: 1, column: 17 },
                                end: { line: 1, column: 29 }
                            }
                        },
                        kind: "",
                        range: [9, 29],
                        loc: {
                            start: { line: 1, column: 9 },
                            end: { line: 1, column: 29 }
                        }
                    }
                ],
                range: [8, 30],
                loc: {
                    start: { line: 1, column: 8 },
                    end: { line: 1, column: 30 }
                }
            },
            range: [0, 30],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 30 }
            }
        },

        '"use strict"; (class A {constructor() { super() }})': {
            type: "Program",
            body: [
                {
                    type: "ExpressionStatement",
                    expression: {
                        type: "Literal",
                        value: "use strict",
                        raw: "\"use strict\"",
                        range: [0, 12],
                        loc: {
                            start: { line: 1, column: 0 },
                            end: { line: 1, column: 12 }
                        }
                    },
                    range: [0, 13],
                    loc: {
                        start: { line: 1, column: 0 },
                        end: { line: 1, column: 13 }
                    }
                },
                {
                    type: "ExpressionStatement",
                    expression: {
                        id: {
                            type: "Identifier",
                            name: "A",
                            range: [21, 22],
                            loc: {
                                start: { line: 1, column: 21 },
                                end: { line: 1, column: 22 }
                            }
                        },
                        type: "ClassExpression",
                        body: {
                            type: "ClassBody",
                            body: [
                                {
                                    type: "MethodDefinition",
                                    key: {
                                        type: "Identifier",
                                        name: "constructor",
                                        range: [24, 35],
                                        loc: {
                                            start: { line: 1, column: 24 },
                                            end: { line: 1, column: 35 }
                                        }
                                    },
                                    value: {
                                        type: "FunctionExpression",
                                        id: null,
                                        params: [],
                                        defaults: [],
                                        body: {
                                            type: "BlockStatement",
                                            body: [
                                                {
                                                    type: "ExpressionStatement",
                                                    expression: {
                                                        type: "CallExpression",
                                                        callee: {
                                                            type: "Identifier",
                                                            name: "super",
                                                            range: [40, 45],
                                                            loc: {
                                                                start: { line: 1, column: 40 },
                                                                end: { line: 1, column: 45 }
                                                            }
                                                        },
                                                        'arguments': [],
                                                        range: [40, 47],
                                                        loc: {
                                                            start: { line: 1, column: 40 },
                                                            end: { line: 1, column: 47 }
                                                        }
                                                    },
                                                    range: [40, 48],
                                                    loc: {
                                                        start: { line: 1, column: 40 },
                                                        end: { line: 1, column: 48 }
                                                    }
                                                }
                                            ],
                                            range: [38, 49],
                                            loc: {
                                                start: { line: 1, column: 38 },
                                                end: { line: 1, column: 49 }
                                            }
                                        },
                                        rest: null,
                                        generator: false,
                                        expression: false,
                                        range: [38, 49],
                                        loc: {
                                            start: { line: 1, column: 38 },
                                            end: { line: 1, column: 49 }
                                        }
                                    },
                                    kind: "",
                                    range: [24, 49],
                                    loc: {
                                        start: { line: 1, column: 24 },
                                        end: { line: 1, column: 49 }
                                    }
                                }
                            ],
                            range: [23, 50],
                            loc: {
                                start: { line: 1, column: 23 },
                                end: { line: 1, column: 50 }
                            }
                        },
                        range: [15, 50],
                        loc: {
                            start: { line: 1, column: 15 },
                            end: { line: 1, column: 50 }
                        }
                    },
                    range: [14, 51],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 51 }
                    }
                }
            ],
            range: [0, 51],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 51 }
            },
            comments: []
        }

    },

    'Expression Closures': {

        'function a() 1' : {
            type: 'FunctionDeclaration',
            id: {
                type: 'Identifier',
                name: 'a',
                range: [
                    9,
                    10
                ],
                loc: {
                    start: {
                        line: 1,
                        column: 9
                    },
                    end: {
                        line: 1,
                        column: 10
                    }
                }
            },
            params: [],
            defaults: [],
            body: {
                type: 'Literal',
                value: 1,
                raw: '1',
                range: [
                    13,
                    14
                ],
                loc: {
                    start: {
                        line: 1,
                        column: 13
                    },
                    end: {
                        line: 1,
                        column: 14
                    }
                }
            },
            rest: null,
            generator: false,
            expression: true,
            range: [
                0,
                14
            ],
            loc: {
                start: {
                    line: 1,
                    column: 0
                },
                end: {
                    line: 1,
                    column: 14
                }
            }
        },

        'function a() {} // not an expression': {
            type: 'FunctionDeclaration',
            id: {
                type: 'Identifier',
                name: 'a',
                range: [
                    9,
                    10
                ],
                loc: {
                    start: {
                        line: 1,
                        column: 9
                    },
                    end: {
                        line: 1,
                        column: 10
                    }
                }
            },
            params: [],
            defaults: [],
            body: {
                type: 'BlockStatement',
                body: [],
                range: [
                    13,
                    15
                ],
                loc: {
                    start: {
                        line: 1,
                        column: 13
                    },
                    end: {
                        line: 1,
                        column: 15
                    }
                }
            },
            rest: null,
            generator: false,
            expression: false,
            range: [
                0,
                15
            ],
            loc: {
                start: {
                    line: 1,
                    column: 0
                },
                end: {
                    line: 1,
                    column: 15
                }
            }
        }
    },

    // ECMAScript 6th Syntax, 13 - Rest parameters
    // http://wiki.ecmascript.org/doku.php?id=harmony:rest_parameters
    'ES6: Rest parameters': {
        'function f(a, ...b) {}': {
            type: "FunctionDeclaration",
            id: {
                type: "Identifier",
                name: "f",
                range: [ 9, 10 ],
                loc: {
                    start: { line: 1, column: 9 },
                    end: { line: 1, column: 10 }
                }
            },
            params: [
                {
                    type: "Identifier",
                    name: "a",
                    range: [ 11, 12 ],
                    loc: {
                        start: { line: 1, column: 11 },
                        end: { line: 1, column: 12 }
                    }
                },
                {
                    type: "RestParameter",
                    value: {
                        type: "Identifier",
                        name: "b",
                        range: [ 17, 18 ],
                        loc: {
                            start: { line: 1, column: 17 },
                            end: { line: 1, column: 18 }
                        }
                    },
                    range: [ 14, 18 ],
                    loc: {
                        start: { line: 1, column: 14 },
                        end: { line: 1, column: 18 }
                    }
                }
            ],
            defaults: [],
            body: {
                type: "BlockStatement",
                body: [],
                range: [ 20, 22 ],
                loc: {
                    start: { line: 1, column: 20 },
                    end: { line: 1, column: 22 }
                }
            },
            rest: null,
            generator: false,
            expression: false,
            range: [ 0, 22 ],
            loc: {
                start: { line: 1, column: 0 },
                end: { line: 1, column: 22 }
            }
        },
        'function f(a, ...b, c)': {
            index: 18,
            lineNumber: 1,
            column: 19,
            message: 'Error: Line 1: Rest parameter must be final parameter of an argument list.'
        }
    },

    'Harmony Invalid syntax': {

        '0o': {
            index: 2,
            lineNumber: 1,
            column: 3,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0o1a': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0o9': {
            index: 2,
            lineNumber: 1,
            column: 3,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0o18': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0O': {
            index: 2,
            lineNumber: 1,
            column: 3,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0O1a': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0O9': {
            index: 2,
            lineNumber: 1,
            column: 3,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0O18': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0b': {
            index: 2,
            lineNumber: 1,
            column: 3,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0b1a': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0b9': {
            index: 2,
            lineNumber: 1,
            column: 3,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0b18': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0b12': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0B': {
            index: 2,
            lineNumber: 1,
            column: 3,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0B1a': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0B9': {
            index: 2,
            lineNumber: 1,
            column: 3,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0B18': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '0B12': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '"\\u{110000}"': {
            index: 11,
            lineNumber: 1,
            column: 12,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '"\\u{}"': {
            index: 4,
            lineNumber: 1,
            column: 5,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '"\\u{FFFF"': {
            index: 9,
            lineNumber: 1,
            column: 10,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '"\\u{FFZ}"': {
            index: 7,
            lineNumber: 1,
            column: 8,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '[v] += ary': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Invalid left-hand side in assignment'
        },

        '[2] = 42': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Invalid left-hand side in assignment'
        },

        '({ obj:20 }) = 42': {
            index: 12,
            lineNumber: 1,
            column: 13,
            message: 'Error: Line 1: Invalid left-hand side in assignment'
        },

        '( { get x() {} } ) = 0': {
            index: 18,
            lineNumber: 1,
            column: 19,
            message: 'Error: Line 1: Invalid left-hand side in assignment'
        },

        'x \n is y': {
            index: 7,
            lineNumber: 2,
            column: 5,
            message: 'Error: Line 2: Unexpected identifier'
        },

        'x \n isnt y': {
            index: 9,
            lineNumber: 2,
            column: 7,
            message: 'Error: Line 2: Unexpected identifier'
        },

        'function hello() {\'use strict\'; ({ i: 10, s(eval) { } }); }': {
            index: 44,
            lineNumber: 1,
            column: 45,
            message: 'Error: Line 1: Parameter name eval or arguments is not allowed in strict mode'
        },

        'function a() { "use strict"; ({ b(t, t) { } }); }': {
            index: 37,
            lineNumber: 1,
            column: 38,
            message: 'Error: Line 1: Strict mode function may not have duplicate parameter names'
        },

        'var super': {
            index: 4,
            lineNumber: 1,
            column: 5,
            message: 'Error: Line 1: Unexpected reserved word'
        },

        '({ v: eval }) = obj': {
            index: 13,
            lineNumber: 1,
            column: 14,
            message: 'Error: Line 1: Invalid left-hand side in assignment'
        },

        '({ v: arguments }) = obj': {
            index: 18,
            lineNumber: 1,
            column: 19,
            message: 'Error: Line 1: Invalid left-hand side in assignment'
        },
        'for (var i = function() { return 10 in [] } in list) process(x);': {
            index: 44,
            lineNumber: 1,
            column: 45,
            message: 'Error: Line 1: Unexpected token in'
        },

        'for (let x = 42 in list) process(x);': {
            index: 16,
            lineNumber: 1,
            column: 17,
            message: 'Error: Line 1: Unexpected token in'
        },

        'for (let x = 42 of list) process(x);': {
            index: 16,
            lineNumber: 1,
            column: 17,
            message: 'Error: Line 1: Unexpected identifier'
        },

        'module X 0': {
            index: 9,
            lineNumber: 1,
            column: 10,
            message: 'Error: Line 1: Unexpected number'
        },

        'module X at Y': {
            index: 9,
            lineNumber: 1,
            column: 10,
            message: 'Error: Line 1: Unexpected identifier'
        },

        'export for': {
            index: 7,
            lineNumber: 1,
            column: 8,
            message: 'Error: Line 1: Unexpected token for'
        },

        'import foo': {
            index: 10,
            lineNumber: 1,
            column: 11,
            message: 'Error: Line 1: Missing from after import'
        },

        '((a)) => 42': {
            index: 6,
            lineNumber: 1,
            column: 7,
            message: 'Error: Line 1: Unexpected token =>'
        },

        '(a, (b)) => 42': {
            index: 9,
            lineNumber: 1,
            column: 10,
            message: 'Error: Line 1: Unexpected token =>'
        },

        'eval => 42': {
            index: 4,
            lineNumber: 1,
            column: 5,
            message: 'Error: Line 1: Parameter name eval or arguments is not allowed in strict mode'
        },

        'arguments => 42': {
            index: 9,
            lineNumber: 1,
            column: 10,
            message: 'Error: Line 1: Parameter name eval or arguments is not allowed in strict mode'
        },

        '(eval, a) => 42': {
            index: 9,
            lineNumber: 1,
            column: 10,
            message: 'Error: Line 1: Parameter name eval or arguments is not allowed in strict mode'
        },

        '(arguments, a) => 42': {
            index: 14,
            lineNumber: 1,
            column: 15,
            message: 'Error: Line 1: Parameter name eval or arguments is not allowed in strict mode'
        },

        '(a, a) => 42': {
            index: 6,
            lineNumber: 1,
            column: 7,
            message: 'Error: Line 1: Strict mode function may not have duplicate parameter names'
        },

        '(a) => 00': {
            index: 7,
            lineNumber: 1,
            column: 8,
            message: 'Error: Line 1: Octal literals are not allowed in strict mode.'
        },

        '() <= 42': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Unexpected token <='
        },

        '(10) => 00': {
            index: 5,
            lineNumber: 1,
            column: 6,
            message: 'Error: Line 1: Unexpected token =>'
        },

        '(10, 20) => 00': {
            index: 9,
            lineNumber: 1,
            column: 10,
            message: 'Error: Line 1: Unexpected token =>'
        },

        'yield v': {
            index: 5,
            lineNumber: 1,
            column: 6,
            message: 'Error: Line 1: Illegal yield expression'
        },

        'yield 10': {
            index: 5,
            lineNumber: 1,
            column: 6,
            message: 'Error: Line 1: Illegal yield expression'
        },

        'yield* 10': {
            index: 5,
            lineNumber: 1,
            column: 6,
            message: 'Error: Line 1: Illegal yield expression'
        },

        'e => yield* 10': {
            index: 10,
            lineNumber: 1,
            column: 11,
            message: 'Error: Line 1: Illegal yield expression'
        },

        '(function () { yield 10 })': {
            index: 20,
            lineNumber: 1,
            column: 21,
            message: 'Error: Line 1: Illegal yield expression'
        },

        '(function () { yield* 10 })': {
            index: 20,
            lineNumber: 1,
            column: 21,
            message: 'Error: Line 1: Illegal yield expression'
        },

        '(function* () { yield yield 10 })': {
            index: 27,
            lineNumber: 1,
            column: 28,
            message: 'Error: Line 1: Illegal yield expression'
        },

        '(function* () { })': {
            index: 17,
            lineNumber: 1,
            column: 18,
            message: 'Error: Line 1: Missing yield in generator'
        },

        'function* test () { }': {
            index: 21,
            lineNumber: 1,
            column: 22,
            message: 'Error: Line 1: Missing yield in generator'
        },

        'var obj = { *test() { } }': {
            index: 23,
            lineNumber: 1,
            column: 24,
            message: 'Error: Line 1: Missing yield in generator'
        },

        'var obj = { *test** }': {
            index: 17,
            lineNumber: 1,
            column: 18,
            message: 'Error: Line 1: Unexpected token *'
        },

        'class A extends yield B { }': {
            index: 21,
            lineNumber: 1,
            column: 22,
            message: 'Error: Line 1: Illegal yield expression'
        },

        '`test': {
            index: 5,
            lineNumber: 1,
            column: 6,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        'switch `test`': {
            index: 7,
            lineNumber: 1,
            column: 8,
            message: 'Error: Line 1: Unexpected quasi test'
        },

        '`hello ${10 `test`': {
            index: 18,
            lineNumber: 1,
            column: 19,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        '`hello ${10;test`': {
            index: 11,
            lineNumber: 1,
            column: 12,
            message: 'Error: Line 1: Unexpected token ILLEGAL'
        },

        'function () 1; // even with expression closures, must be named': {
            index: 9,
            lineNumber: 1,
            column: 10,
            message:  'Error: Line 1: Unexpected token ('
        },

        '[a,b if (a)] // (a,b)': {
            index: 4,
            lineNumber: 1,
            column: 5,
            message: 'Error: Line 1: Comprehension Error'
        },

        'for each (let x in {}) {};': {
            index: 3,
            lineNumber: 1,
            column: 4,
            message: 'Error: Line 1: Each is not supported'
        },

        '[x for (let x in [])]': {
            index: 20,
            lineNumber: 1,
            column: 21,
            message: 'Error: Line 1: Comprehension Error'
        },

        '[x for (const x in [])]': {
            index: 22,
            lineNumber: 1,
            column: 23,
            message: 'Error: Line 1: Comprehension Error'
        },

        '[x for (var x in [])]': {
            index: 20,
            lineNumber: 1,
            column: 21,
            message: 'Error: Line 1: Comprehension Error'
        },

        '[a,b for (a in [])] // (a,b) ': {
            index: 4,
            lineNumber: 1,
            column: 5,
            message: 'Error: Line 1: Comprehension Error'
        },

        '[x if (x)]  // block required': {
            index: 10,
            lineNumber: 1,
            column: 11,
            message: 'Error: Line 1: Comprehension must have at least one block'
        },

        'var a = [x if (x)]': {
            index: 18,
            lineNumber: 1,
            column: 19,
            message: 'Error: Line 1: Comprehension must have at least one block'
        },

        '[for (x in [])]  // no espression': {
            index: 15,
            lineNumber: 1,
            column: 16,
            message: 'Error: Line 1: Comprehension Error'
        },

        '({ "chance" }) = obj': {
            index: 12,
            lineNumber: 1,
            column: 13,
            message: 'Error: Line 1: Unexpected token }'
        },

        '({ 42 }) = obj': {
            index: 6,
            lineNumber: 1,
            column: 7,
            message: 'Error: Line 1: Unexpected token }'
        }
    }

};

// Merge both test fixtures.

(function () {

    'use strict';

    var i, fixtures;

    for (i in harmonyTestFixture) {
        if (harmonyTestFixture.hasOwnProperty(i)) {
            fixtures = harmonyTestFixture[i];
            if (i !== 'Syntax' && testFixture.hasOwnProperty(i)) {
                throw new Error('Harmony test should not replace existing test for ' + i);
            }
            testFixture[i] = fixtures;
        }
    }

}());

