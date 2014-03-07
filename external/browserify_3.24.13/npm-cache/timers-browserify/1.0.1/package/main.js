// DOM APIs, for completeness

if (typeof setTimeout !== 'undefined') exports.setTimeout = setTimeout;
if (typeof clearTimeout !== 'undefined') exports.clearTimeout = clearTimeout;
if (typeof setInterval !== 'undefined') exports.setInterval = setInterval;
if (typeof clearInterval !== 'undefined') exports.clearInterval = clearInterval;

// TODO: Change to more effiecient list approach used in Node.js
// For now, we just implement the APIs using the primitives above.

exports.enroll = function(item, delay) {
  item._timeoutID = setTimeout(item._onTimeout, delay);
};

exports.unenroll = function(item) {
  clearTimeout(item._timeoutID);
};

exports.active = function(item) {
  // our naive impl doesn't care (correctness is still preserved)
};

exports.setImmediate = require('process/browser.js').nextTick;
