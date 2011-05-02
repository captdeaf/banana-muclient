API.onReady = function() {
  $('#namebox').html("You are logged in as " + API.username);
  $('#filelink').attr({href:"/user/" + API.loginname + "/files/"});
  $('#filelink').show();
  if (!API.username.match(/guest/)) {
    $('#loglink').attr({href:"/user/" + API.loginname + "/logs/"});
    $('#loglink').show();
  }
};

function text2html(text) {
  return $('<div></div>').text(text).html();
}

var outbox;
var inbox;
var boxframe;

function resizeDivs() {
  outbox.height(boxframe.height() - 170);
  inbox.height(40);
  outbox.width(boxframe.width() - 40);
  inbox.width(boxframe.width() - 40);
  redrawAllWorlds();
}

$(document).ready(function() {
  $.stylesheetInit();
  $('#resizable').resizable({
    handles:'se',
    minHeight: 300,
    minWidth: 500,
    resize: resizeDivs,
  });
  outbox = $('#outbox');
  boxframe = $('#resizable');
  inbox = $('#inbox');

  resizeDivs();
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

// Called after a limit or unlimit.
function redrawWorld(world) {
  var w = allWorlds[world];
  var lim;
  if (w.limit) {
    lim = [];
    for (var i = 0; i < w.lines.length; i++) {
      if (matchLimit(w.lines[i], w.limit)) {
        lim.push(w.lines[i]);
      }
    }
  } else {
    lim = w.lines;
  }
  w.lineCount = lim.length;
  var l = wrapLines(lim, curWidth);
  w.outdiv.html(l.join("<br />\n"));
  if (world == curWorld) {
    outbox.attr({ scrollTop: outbox.attr("scrollHeight") });
  }
}

function redrawAllWorlds() {
  curWidth = getCharWidthOf(outbox);
  for (var world in allWorlds) {
    redrawWorld(world);
  }
}

function matchLimit(text, limit) {
  return striphtml(text).match(limit);
}

function setLimit(world, limit) {
  if (allWorlds[world]) {
    allWorlds[world].limit = limit;
    redrawWorld(world);
  } else {
    API.alert("No such world '" + world + "'");
  }
}

API.flush = function() {
  for (var world in toadd) {
    var w = allWorlds[world];
    if (w) {
      var lim;
      if (w.limit) {
        lim = [];
        for (var i = 0; i < toadd[world].length; i++) {
          if (matchLimit(toadd[world][i], w.limit)) {
            lim.push(toadd[world][i]);
          }
        }
      } else {
        lim = w.lines;
      }
      if (lim.length > 0) {
        w.lineCount += lim.length;
        if (w.lineCount > (w.lineLimit + 200)) {
          // Redraw the world instead of appending to a too-long buffer.
          redrawWorld(world);
        } else {
          var l = wrapLines(lim, curWidth);
          w.outdiv.append("<br />");
          w.outdiv.append(l.join("<br />\n"));
          if (world == curWorld) {
            outbox.attr({ scrollTop: outbox.attr("scrollHeight") });
          }
        }
      }
    } else {
      setPrompt("Odd errors? " + toadd[world].join('<br>'));
    }
  }
  toadd = {};
}

var FugueCommands = {};
function handleCommand(cmd, args) {
  if (FugueCommands[cmd]) {
    FugueCommands[cmd].callback(args);
    API.flush();
  } else {
    API.alert('Invalid command "' + cmd + '"');
    API.flush();
  }
}

function handleSend(text) {
  API.world.send(curWorld, text);
}

var inprev = [];
var innext = [];
function handleInput(text) {
  var m;
  inprev.push(text);
  if (text.match(/^\/\/(.*)/)) {
    handleSend(text.slice(1));
  } else if (m = text.match(/^\/(\S*)\s*(.*)$/)) {
    handleCommand(m[1],m[2]);
  } else {
    handleSend(text);
  }
}

function recallPrev() {
  if (inprev.length > 0) {
    var val = inbox.val();
    if (val && val != '') {
      innext.push(val);
    }
    inbox.val(inprev.pop());
  }
  inbox.focus();
}
function recallNext() {
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
}
$(document).ready(function() {
  $('#prev').click(recallPrev);
  $('#next').click(recallNext);
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
  API.alert(text);
}

API.world.onOverflow = function(p) {
  setPrompt("Too much text was received at once. You lost " + p.count + " lines.");
};

function appendToWorld(world, msg, received) {
  if (!toadd[world]) {
    toadd[world] = []
  };
  var w = allWorlds[world];
  if (w) {
    if (w.lines.push(msg) > (w.lineLimit + 200)) {
      w.lines = w.lines.slice(0 - w.lineLimit);
    }
    if (received) {
      if (w.inlines.push(msg) > (w.lineLimit + 200)) {
        w.inlines = w.inlines.slice(0 - w.lineLimit);
      }
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

var FugueTriggers = {};
function handleReceived(p) {
  // TODO: Figure out a way to handle fugue triggers, with
  // priority, regexp matching, and ability to replace (in html!)
  // stuff.
  appendToWorld(p.world, p.text || '', true);
}

API.world.onReceive = function(p) {
  handleReceived(p);
};
API.world.onPrompt = function(p) {
  handleReceived(p);
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
    inlines: [],
    lineLimit: 3000,
    lineCount: 0
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

var KeyReplacers = {
  17: 'ctrl',
  18: 'alt',
  37: 'left',
  38: 'up',
  39: 'right',
  40: 'down',
  33: 'pgup',
  34: 'pgdn'
};

API.alert = function(msg) {
  appendToWorld(curWorld, '-- ' + msg + ' --');
  API.flush();
}

$(document).ready(function() {
  redrawAllWorlds();
  outbox = $('#outbox');
  inbox = $('#inbox');
  inbox.focus();
  inbox.keydown(function(e) {
    var code = e.charCode ? e.charCode : e.keyCode;
    if (code == 13) {
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
    } else if (e.altKey || e.ctrlKey) {
      var keyp = String.fromCharCode(code);
      if (KeyReplacers[code]) {
        keyp = KeyReplacers[code];
      }
      var keyname = 'key_' + 
        (e.ctrlKey ? 'ctrl_' : '') +
        (e.altKey ? 'alt_' : '') +
        keyp;
      keyname = text2html(keyname);
      if (FugueCommands[keyname]) {
        handleCommand(keyname, 'down');
        e.keyCode = 505;
        e.returnValue = false;
        e.cancelBubble = true;
        if (e.stopPropagation) {
          e.stopPropagation();
          e.preventDefault();
        }
        return false;
      }
    }
    return true;
  });
});
