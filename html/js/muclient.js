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
      type: 'get',
      success: API.onSuccess,
      error: API.onError,
    });
  },
  callQueue: $(function() {}), // I don't know why, but jQuery needs this?
  callQueueComplete: function() {
    API.callQueue.dequeue();
  },
  queueAction: function(fun) {
    API.callQueue.queue(fun);
  },
  callAction: function(uri, args) {
    API.queueAction(function() {
      $.ajax({
        url: '/action/' + uri,
        async: true,
        data: args,
        type: 'post',
        complete: API.callQueueComplete
      });
    });
  },
  world: {
    open: function(world) {
      API.callAction('world.open', {world: world});
    },
    connect: function(world, host, port) {
      API.callAction('world.connect', {world: world, host: host, port: port});
    },
    sendQueues: {},
    send: function(world, text) {
      // send is special: We don't fire immediately, in case of queued actions.
      if (API.world.sendQueues[world]) {
        API.world.sendQueues[world].push(text);
      } else {
        API.world.sendQueues[world] = [text];
        API.queueAction(function() {
          var allLines = API.world.sendQueues[world].join("\n");
          delete API.world.sendQueues[world];
          $.ajax({
            url: '/action/world.send',
            async: true,
            data: {world: world, text: allLines},
            type: 'post',
            complete: API.callQueueComplete
          });
        });
      }
    },
    echoQueues: {},
    echo: function(world, text) {
      // echo is special: We don't fire immediately, in case of queued actions.
      if (API.world.echoQueues[world]) {
        API.world.echoQueues[world].push(text);
      } else {
        API.world.echoQueues[world] = [text];
        API.queueAction(function() {
          var allLines = API.world.echoQueues[world].join("\n");
          delete API.world.echoQueues[world];
          $.ajax({
            url: '/action/world.echo',
            async: true,
            data: {world: world, text: allLines},
            type: 'post',
            complete: API.callQueueComplete
          });
        });
      }
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
