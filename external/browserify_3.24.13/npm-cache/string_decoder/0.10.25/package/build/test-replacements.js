module.exports.all = [
    [
        /require\(['"]string_decoder['"]\)/g
      , 'require(\'../../\')'
    ]

]

module.exports['common.js'] = [
    [
        /^                      setImmediate,$/m
      , '                      typeof setImmediate == \'undefined\' ? null : setImmediate,'
    ]

  , [
        /^                      clearImmediate,$/m
      , '                      typeof clearImmediate == \'undefined\' ? null : clearImmediate,'
    ]

  , [
        /^                      global];$/m
      , '                      global].filter(Boolean);'
    ]
]
