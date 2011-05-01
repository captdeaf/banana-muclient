function getCharWidthOf(element) {
  if (element.charmeasure == undefined) {
    var x = $('<div>ABCDEFGHIJKLMNOPQRSTUVWXYZ</div>');
    x.css({
      visibility: 'hidden',
      position: 'absolute',
      width: 'auto',
      height: 'auto'
    });
    element.prepend(x);
    element.charmeasure = x;
  }
  // Subtract 30 for the scrollbar.
  return Math.floor((element.width() - 30) /
                    (element.charmeasure.width() / 26));
}

var wrapPatterns = {};

function getWrapRx(width) {
  if (wrapPatterns[width]) return wrapPatterns[width];
  var x = {
    full: new RegExp('^((?:(?:<[^>]*>)*(?:[^<>&]|&[^;]*;)){1,' + width + '}(?:<[^>]*>)*)((?:&nbsp;|\\s)+)?(\\S.+)?$'),
    withspace: new RegExp('^((?:(?:<[^>]+>)*(?:[^<>&]|&.*?;)(?:<[^>]+>)*){1,' + width + '})((?:&nbsp;|\\s))*(\\S[\\s\\S]+)?$'),
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
      var x;
      while (rest && rest.length > 0) {
        if (m = rest.match(rx.full)) {
          var spaces = m[2] || '';
          rest = m[3] || '';
          if (spaces.length > 0 || rest.length == 0) {
            newlines.push(m[1]);
          } else if (x = m[1].match(/^((?:(?:<[^>]*>)*(?:[^<>&]|&[^;]*;))+)(\s|&nbsp;)(\S+)$/)) {
            newlines.push(x[1]);
            rest = x[3] + (m[2] || '') + (m[3] || '');
          } else {
            newlines.push(m[1]);
            rest = (m[2] || '') + (m[3] || '');
          }
        } else {
          newlines.push(rest);
          rest = '';
        }
      }
    }
  };
  return newlines;
}
