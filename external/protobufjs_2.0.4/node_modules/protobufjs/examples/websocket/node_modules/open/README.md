# open

open a file or url in the user's preferred application

# usage

```javascript
var open = require('open');
open('http://www.google.com');
```

# install

    npm install open

# how it works

- on `win32` uses `start`
- on `darwin` uses `open`
- otherwise uses the `xdg-open` script from [freedesktop.org](http://portland.freedesktop.org/xdg-utils-1.0/xdg-open.html)

# warning

The same care should be taken when calling open as if you were calling child_process.exec directly. If it is an executable it will run in a new shell.
