var wrapPatterns = {};

function makeWrapRx(width) {
  var withspace = '((?:(?:<.*?>)?(?:[^<>&]|&.*?;)(?:<.*?>)?){0,' + width + '})(?:&nbsp;|\\s+)';
  var nospace = '((?:(?:<.*?>)?(?:[^<>&]|&.*?;)(?:<.*?>)?){' + width + '})';
  var end = '((?:(?:<.*?>)?(?:[^<>&]|&.*?;)(?:<.*?>)?){1,' + width + '})\s*$';
  var rx = end + '|' + withspace + '|' + nospace;
  return new RegExp(rx,'g');
}

function wrapLines(lines,width) {
  var newlines = [];
  var rx;
  var nlines = lines.length;
  rx = wrapPatterns[width];
  if (rx == undefined) {
    rx = makeWrapRx(width);
    wrapPatterns[width] = rx;
  }
  for (var i = 0; i < nlines; i += 1) {
    var line = lines[i];
    var newline = '' + line;
    newline = newline.replace(rx,function(str,end,withspace,nospace) {
      if (withspace) {
        newlines.push(withspace);
      } else if (nospace) {
        newlines.push(nospace);
      } else if (end) {
        newlines.push(end);
      } else {
        newlines.push(str);
      }
    });
  };
  return newlines;
}
