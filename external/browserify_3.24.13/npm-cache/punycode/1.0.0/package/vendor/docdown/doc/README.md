# Docdown.php API documentation

<!-- div -->


<!-- div -->

## `docdown`
* [`docdown`](#docdown)

<!-- /div -->


<!-- /div -->


<!-- div -->


<!-- div -->

## `docdown`

<!-- div -->

### <a id="docdown" href="https://github.com/jdalton/docdown/blob/master/docdown.php#L34" title="View in source">`docdown([$options=array()])`</a>
Generates Markdown from JSDoc entries in a given file.
[&#9650;][1]

#### Arguments
1. `[$options=array()]` *(Array)*: The options array.

#### Returns
*(String)*: The generated Markdown.

#### Example
~~~ php
// specify a file path
$markdown = docdown(array(
  // path to js file
  'path' => $filepath,
  // url used to reference line numbers in code
  'url' => 'https://github.com/username/project/blob/master/my.js'
));

// or pass raw js
$markdown = docdown(array(
  // raw JavaScript source
  'source' => $rawJS,
  // documentation title
  'title' => 'My API Documentation',
  // url used to reference line numbers in code
  'url' => 'https://github.com/username/project/blob/master/my.js'
));
~~~

<!-- /div -->


<!-- /div -->


<!-- /div -->


  [1]: #readme "Jump back to the TOC."