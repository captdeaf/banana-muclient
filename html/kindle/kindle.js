API.onReady = function() {
  $('#namebox').html("You are logged in as " + API.username);
  $('#filelink').attr({href: API.apibase + "user/" + API.loginname + "/files/"});
  $('#filelink').show();
  if (!API.username.match(/guest/)) {
    $('#loglink').attr({href: API.apibase + "user/" + API.loginname + "/logs/"});
    $('#loglink').show();
  }
};

var outbox;
var inbox;

$(document).ready(function() {
  outbox = $('#outbox');
  inbox = $('#inbox');
  var newheight = $(window).height() - 250;
  var newwidth = $(window).width() - 80;
  outbox.css({height: newheight, width: newwidth});
  inbox.css({width: newwidth - 80});
});

var repls = {
  '&amp;': '&',
  '&lt;': '<',
  '&gt;': '>',
  '&nbsp;': ' ',
};

function striphtml(line) {
  line = line.replace(/(<.*?>)+/g,'');
  return line.replace(/&.*?;/g, function(i) { return repls[i]; });
}

var toadd = {};
var allWorlds = {};
var curWidth = 80;

function redrawWorld(world) {
  var w = allWorlds[world];
  if (w) {
    var l = wrapLines(w.lines, curWidth);
    w.outdiv.html(l.join("<br />\n"));
    w.lineCount = w.lines.length;
    if (world == curWorld) {
      outbox.attr({ scrollTop: outbox.attr("scrollHeight") });
    }
  }
}

function redrawAllWorlds() {
  curWidth = getCharWidthOf(outbox);
  for (var world in allWorlds) {
    var w = allWorlds[world];
    var l = wrapLines(w.lines, curWidth);
    w.lineCount = w.lines.length;
    w.outdiv.html(l.join("<br />\n"));
    if (world == curWorld) {
      outbox.attr({ scrollTop: outbox.attr("scrollHeight") });
    }
  }
}

API.flush = function() {
  for (var world in toadd) {
    var w = allWorlds[world];
    if (w) {
      w.lineCount += toadd[world].length;
      // If linecount > w.limit + 200, then redraw.
      if (w.lineCount > (w.limit + 200)) {
        redrawWorld(world);
        w.lineCount = w.lines.length;
      } else {
        var l = wrapLines(toadd[world], curWidth);
        w.outdiv.append("<br />");
        w.outdiv.append(l.join("<br />\n"));
        w.lineCount += toadd[world].length;
      }
      if (world == curWorld) {
        outbox.attr({ scrollTop: outbox.attr("scrollHeight") });
      }
    } else {
      setPrompt("Odd errors? " + toadd[world].join('<br>'));
    }
  }
  toadd = {};
}

function handleCommand(cmd, args) {
  if (cmd == 'open') {
    API.world.open(args);
  } else if (cmd == 'connect') {
    var m = args.split(/\s+/,2);
    API.world.connect(curWorld, m[0], m[1]);
  } else if (cmd == 'disconnect') {
    API.world.disconnect(curWorld);
  } else if (cmd == 'close') {
    API.world.close(curWorld);
  } else {
    API.alert('Invalid command "' + cmd + '"');
  }
}

var inprev = [];
var innext = [];
function handleInput(text) {
  var m;
  if (text.match(/^\/\/(.*)/)) {
    API.world.send(curWorld, text.slice(1));
  } else if (m = text.match(/^\/(\S*)\s*(.*)$/)) {
    handleCommand(m[1],m[2]);
  } else {
    inprev.push(text);
    API.world.send(curWorld, text);
  }
}
$(document).ready(function() {
  $('#prev').click(function() {
    if (inprev.length > 0) {
      var val = inbox.val();
      if (val && val != '') {
        innext.push(val);
      }
      inbox.val(inprev.pop());
    }
    inbox.focus();
  });
  $('#next').click(function() {
    var val = inbox.val() || '';
    if (innext.length > 0 || val != '') {
      if (val != '') {
        inprev.push(val);
      }
      if (innext.length > 0) {
        inbox.val(innext.pop());
      } else {
        inbox.val('');
      }
    }
    inbox.focus();
  });
});

function showWorld(which) {
  curWorld = which;
  for (var world in allWorlds) {
    var w = allWorlds[world];
    if (world == which) {
      w.outdiv.show();
      w.outdiv.attr({ scrollTop: w.outdiv.attr("scrollHeight") });
      w.tab.css({ 'text-decoration': 'underline', 'background-color': '#99F'});
    } else {
      w.outdiv.hide();
      w.tab.css({ 'text-decoration': 'none', 'background-color': '#CCCCCC'});
    }
  }
  outbox.attr({ scrollTop: outbox.attr("scrollHeight") });
  inbox.focus();
}

function setPrompt(text) {
  appendToWorld(curWorld, '-- ' + text + ' --');
  API.flush();
}

API.world.onOverflow = function(p) {
  setPrompt("Too much text was received at once. You lost " + p.count + " lines.");
};

function appendToWorld(world, msg) {
  var w = allWorlds[world];
  if (!toadd[world]) {
    toadd[world] = []
  };
  if (w) {
    if (w.lines.push(msg) > (w.limit + 200)) {
      // Keep the last w.limit lines
      w.lines = w.lines.slice(0 - w.limit);
    }
  }
  toadd[world].push(msg);
}

API.onSystemMessage = function(p) {
  appendToWorld(p.world, 'API error: ' + p.message + ' -- ');
}

function setInfo(text) {
  $('#namebox').html(text);
}

API.world.onReceive = function(p) {
  appendToWorld(p.world, p.text || '');
};
API.world.onPrompt = function(p) {
  appendToWorld(p.world, p.text || '');
};

API.world.onConnectFail = function(p) { appendToWorld(p.world, ' -- CONNFAIL: ' + p.cause + ' -- '); }
API.world.onDisconnectFail = function(p) { appendToWorld(p.world, ' -- DISCONNFAIL: ' + p.cause + ' -- '); }
API.world.onError = function(p) { appendToWorld(p.world, ' -- ERROR: ' + p.cause + ' -- '); }

API.world.onOpen = function(p) {
  var tabname = '#' + p.world + '-output';
  var w = {
    name: p.world,
    tab: $('<div class="tab"><a>' + p.world + '</a></div>'),
    outdiv: $('<div class="world"></div>'),
    lines: [],
    lineCount: 0, /* Not lines.length, but displayed line count. */
    limit: 2000 /* # of lines of recall we keep. */
  };
  w.tab.appendTo('#tabs');
  $(w.tab,'a').click(function() {
    showWorld(w.name);
  });
  w.outdiv.css({display: 'none'});
  w.outdiv.appendTo(outbox);

  allWorlds[p.world] = w;
  appendToWorld(p.world, ' -- Opened -- ');
  showWorld(p.world);
}
API.world.onConnect = function(p) {
  if (p.seen == 1) {
    appendToWorld(p.world, ' -- Connected to ' + p.world + ' --');
  }
}

API.world.onDisconnect = function(p) {
  appendToWorld(p.world, ' -- You have disconnected -- ');
}
API.world.onClose = function(p) {
  appendToWorld(p.world, ' -- Closed -- ');
  API.flush();
  var w = allWorlds[p.world];
  w.tab.remove();
  w.outdiv.remove();
  delete allWorlds[p.world];
  for (var world in allWorlds) {
    showWorld(world);
    break;
  }
}

API.alert = function(msg) {
  setPrompt(msg);
}

$(document).ready(function() {
  redrawAllWorlds();
  outbox = $('#outbox');
  inbox = $('#inbox');
  inbox.focus();
  inbox.keypress(function(e) {
    if (e.keyCode == 13) {
      var val = inbox.val();
      handleInput(val);
      inbox.val('');
      e.keyCode = 505;
      e.returnValue = false;
      e.cancelBubble = true;
      if (e.stopPropagation) {
        e.stopPropagation();
        e.preventDefault();
      }
      return false;
    }
    $('#send').click(function() {
      var val = inbox.val();
      handleInput(val);
      inbox.val('');
    });
    return true;
  });
});
