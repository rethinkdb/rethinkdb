module.exports = function RGBToHex(data) {
  return {
    process: function() {
      return data.replace(/rgb\((\d+),(\d+),(\d+)\)/g, function(match, red, green, blue) {
        var redAsHex = parseInt(red, 10).toString(16);
        var greenAsHex = parseInt(green, 10).toString(16);
        var blueAsHex = parseInt(blue, 10).toString(16);

        return '#' +
          ((redAsHex.length == 1 ? '0' : '') + redAsHex) +
          ((greenAsHex.length == 1 ? '0' : '') + greenAsHex) +
          ((blueAsHex.length == 1 ? '0' : '') + blueAsHex);
      });
    }
  };
};
