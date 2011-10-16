var CONFIG = {
  mudhost: "mush.pennmush.org",
  mudport: "4240",
  mudname: "Walker's Dev MUSH",

};

API.onStart = function() {
  API.file.readJSON("config.json", function(newconf) {
    CONFIG = newconf;
    API.world.open('world');
  });
  API.file.read("header.html", function(content) {
    $('#header').html(content);
  });
  API.file.read("footer.html", function(content) {
    $('#footer').html(content);
  });
};

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

var allLines = [];
var toadd = [];
var curWidth = 80;

function redrawAllLines() {
  curWidth = getCharWidthOf($('#outbox'));
  var l = wrapLines(allLines, curWidth);
  $('#outbox').append(l.join("<br />\n"));
  $("#outbox").attr({ scrollTop: $("#outbox").attr("scrollHeight") });
}

API.flush = function() {
  if (toadd.length > 0) {
    allLines = allLines.concat(toadd).slice(-1000);
    $('#outbox').append("<br />");
    var l = wrapLines(toadd, curWidth);
    $('#outbox').append(l.join("<br />\n"));
    $("#outbox").attr({ scrollTop: $("#outbox").attr("scrollHeight") });
    toadd = [];
  }
}

$('#promptbox').hide();
function setPrompt(text) {
  $('#promptbox').html(text);
  if (text && text != "") {
    $('#promptbox').show();
  } else {
    $('#promptbox').hide();
  }
}

API.world.onOverflow = function(p) {
  setPrompt("Too much text was received at once. You lost " + p.count + " lines.");
};

function append(msg) {
  toadd.push(msg);
}

API.onSystemMessage = function(p) {
  append('API error: ' + p.message + ' -- ');
}

function setInfo(text) {
  $('#namebox').html(text);
}

function handleTriggers(line) {
  line = striphtml(line);
  var m = line.match(/^Welcome, (.*)!$/);
  if (m) {
    setInfo("Connected to " + CONFIG.mudname + ". You are currently logged in as '" + m[1] + "'");
  }
}

API.world.onReceive = function(p) {
  append(p.text || '');
  try {
    handleTriggers(p.text || '');
  } catch (e) {
    setPrompt("Error with trigger: " + e.description);
  }
};
API.world.onPrompt = function(p) { setPrompt(p.text); };

API.world.onConnectFail = function(p) { append(' -- CONNFAIL: ' + p.cause + ' -- '); }
API.world.onDisconnectFail = function(p) { append(' -- DISCONNFAIL: ' + p.cause + ' -- '); }
API.world.onError = function(p) { append(' -- ERROR: ' + p.cause + ' -- '); }

setInfo("Connecting to Banana MU* Gateway");
API.world.onOpen = function(p) {
  if (p.seen == 1) {
    setInfo("Connecting to " + CONFIG.mudname + " . . .");
    API.world.connect('world', CONFIG.mudhost, CONFIG.mudport);
  }
  append(' -- Opened -- ');
}
API.world.onConnect = function(p) {
  if (p.seen == 1) {
    setInfo("Connected to " + CONFIG.mudname);
    append(' -- Connected to ' + CONFIG.mudname + ' --');
    API.file.read("autosend.txt", function(content) {
      API.world.send('world', content);
    });
    // API.world.send('world', '+newbie Hello from a web guest!');
  }
}

API.world.onDisconnect = function(p) {
  setInfo("You have been disconnected. Redirecting. . .");
  append(' -- You have been disconnected -- ');
  setTimeout(function() {
    window.location.replace(API.apibase + "/action/logout");
  }, 3000);
}
API.world.onClose = function(p) { append(' -- Closed -- '); }

API.alert = function(msg) {
  setPrompt(msg);
}

$(document).ready(function() {
  redrawAllLines();
  $('#inbox').focus();
  $('#inbox').keypress(function(e) {
    if (e.keyCode == 13) {
      var val = $('#inbox').val();
      API.world.send('world', val);
      $('#inbox').val('');
      e.keyCode = 505;
      e.returnValue = false;
      e.cancelBubble = true;
      if (e.stopPropagation) {
        e.stopPropagation();
        e.preventDefault();
      }
      return false;
    }
    return true;
  });
});
