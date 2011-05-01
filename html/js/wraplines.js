var wrapPatterns = {};

function getWrapRx(width) {
  if (wrapPatterns[width]) return wrapPatterns[width];
  var x = {
    withspace: new RegExp('^((?:(?:<[^>]+>)*(?:[^<>&]|&.*?;)(?:<[^>]+>)*){1,' + width + '})(?:&nbsp;|\\s)*([\\s\\S]+)?$'),
    nospace: new RegExp('^((?:(?:<[^>]+>)*(?:[^<>&]|&.*?;)(?:<[^>]+>)*){' + width + '})([\\s\S]+)?$')
  };
  wrapPatterns[width] = x;
  return x;
}

function wrapLines(lines,width) {
  var newlines = [];
  var rx;
  var nlines = lines.length;
  rx = getWrapRx(width);
  for (var i = 0; i < nlines; i += 1) {
    var line = lines[i] || '';
    var newline = '' + line;
    if (newline == '') {
      newlines.push('');
    } else {
      var rest = newline;
      var m;
      while (rest && rest.length > 0) {
        if (m = rest.match(rx.withspace)) {
          newlines.push(m[1]);
          rest = m[2];
        } else if (m = rest.match(rx.nospace)) {
          newlines.push(m[1]);
          rest = m[2];
        } else {
          newlines.push(rest);
          rest = '';
        }
      }
    }
  };
  return newlines;
}
