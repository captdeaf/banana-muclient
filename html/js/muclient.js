var API = {
  alert: function(msg) {
    alert(msg);
  },
  updateCount: -1,
  triggerEvent: function(params) {
    /* Only do this once per update count. We are guaranteed
     * to receive it in order from the server. */
    if ((API.updateCount <= params.updateCount) || (params.updateCount == -1)) {
      API.updateCount = params.updateCount + 1;
      try {
        API.onEvent(params);
      } catch (err) {
        alert("Client error: '" + err + "'");
      }
    }
  },
  flush: function() {
  },
  onEvent: function(params) {
    var m = params.eventname.match(/^(\w+)\.(\w+)$/);
    if (m) {
      if (API[m[1]] && API[m[1]][m[2]]) {
        API[m[1]][m[2]](params);
      }
    } else if (API[params.eventname]) {
      API[params.eventname](params);
    }
  },
  onSuccess: function(myData, textStatus, xhr) {
    try {
      var ucount = xhr.getResponseHeader('updateCount');
      API.fetchUpdates(ucount);
      eval(myData);
    } catch (err) {
      API.alert("API error on fetchUpdates/success: '" + err + "'");
    }
    API.updateCount = ucount;
    API.flush();
  },
  onError: function(jqxhr, textStatus) {
    setTimeout(API.fetchupdates, 1000);
  },
  fetchUpdates: function(ucount) {
    if (!ucount) {
      ucount = API.updateCount;
    }
    API.updateObject = $.ajax({
      url: '/action/updates',
      async: true,
      data: {updateCount: ucount},
      dataType: 'text',
      type: 'POST',
      success: API.onSuccess,
      error: API.onError,
    });
  },
  callAction: function(uri, args) {
    $.ajax({
      url: '/action/' + uri,
      async: true,
      data: args,
      type: 'post'
    });
  },
  world: {
    open: function(world) {
      API.callAction('world.open', {world: world});
    },
    connect: function(world, host, port) {
      API.callAction('world.connect', {world: world, host: host, port: port});
    },
    send: function(world, text) {
      API.callAction('world.send', {world: world, text: text});
    },
    echo: function(world, text) {
      API.callAction('world.echo', {world: world, text: text});
    },
    disconnect: function(world) {
      API.callAction('world.disconnect', {world: world});
    },
    close: function(world) {
      API.callAction('world.close', {world: world});
    }
  },
};

$(window).ready(function() {
  // Timeout is needed to prevent Chrome from thinking we're still
  // loading the document with fetchUpdates.
  setTimeout(API.fetchUpdates, 500);
});
