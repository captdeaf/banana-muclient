function addCommand(name, fun) {
  FugueCommands[name] = {
    name: name,
    callback: fun
  }
}

addCommand('key_ctrl_P', function() { recallPrev(); });
addCommand('key_ctrl_N', function() { recallNext(); });
addCommand('key_ctrl_up', function() { recallPrev(); });
addCommand('key_ctrl_down', function() { recallNext(); });

addCommand('limit', function(args) {
  try {
    setLimit(curWorld, new RegExp(args));
  } catch (err) {
    API.alert("Unable to limit: '" + err + "'");
  }
});

addCommand('unlimit', function(args) {
  try {
    setLimit(curWorld, undefined);
  } catch (err) {
    API.alert("Unable to unlimit: '" + err.description + "'");
  }
});

addCommand('fg', function(args) {
  if (allWorlds[args]) {
    showWorld(args);
  }
});
addCommand('open', function(args) {
  API.world.open(args);
});
addCommand('connect', function(args) {
  var m = args.split(/\s+/,2);
  API.world.connect(curWorld, m[0], m[1]);
});
addCommand('disconnect', function(args) {
  API.world.disconnect(curWorld);
});
addCommand('close', function(args) {
  API.world.close(curWorld);
});
addCommand('wob', function(args) {
  $.stylesheetSwitch('white-on-black');
});
addCommand('bow', function(args) {
  $.stylesheetSwitch('black-on-white');
});
addCommand('help', function(args) {
  var hlp = [
    "-- WebFugue help --",
    "As of right now, the only commands that are supported are:",
    "",
    "/wob - White-on-black style",
    "/bow - Black-on-white style",
    "",
    "/open &lt;worldname&gt;",
    "/connect &lt;host&gt; &lt;port&gt;",
    "/disconnect",
    "/close",
    "",
    "/fg &lt;worldname&gt;",
    "",
    "ctrl+p or ctrl+up recalls previous command",
    "ctrl+n or ctrl+down recalls next command",
    "--"
  ];

  for (var i = 0; i < hlp.length; i++) {
    appendToWorld(curWorld, hlp[i]);
  }
});
