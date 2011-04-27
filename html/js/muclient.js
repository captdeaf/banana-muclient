var API = {
  apibase: '', /* It connects to apibase + "/action/..." So if you're
                * connecting from a remote site, use API.apibase = 
                * 'http://client.pennmush.org' or similar. */
  updateCount: -1,
  loginname: '', /* This is set on the first update */
  username: '', /* This is set on the first update */
  setNames: function(loginname, username) {
    API.loginname = loginname;
    API.username = username;
  },
  triggerEvent: function(params) {
    /* Only do this once per update count. We are guaranteed
     * to receive it in order from the server. */
    if ((API.updateCount <= params.updateCount) || (params.updateCount == -1)) {
      try {
        API.onEvent(params);
      } catch (err) {
        API.alert("Client error: '" + err + "'");
      }
    }
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
  onFirstSuccess: function(myData, textStatus, xhr) {
    API.onSuccess(myData, textStatus, xhr);
    API.onReady();
  },
  onSuccess: function(myData, textStatus, xhr) {
    API.errorCount = 0;
    try {
      var ucount = xhr.getResponseHeader('updateCount');
      API.fetchUpdates(ucount);
      eval(myData);
      API.updateCount = ucount;
    } catch (err) {
      API.alert("API error on fetchUpdates/success: '" + err + "'");
    }
    API.updateCount = ucount;
    API.flush();
  },
  errorCount: 0,
  onError: function(jqxhr, textStatus) {
    API.alert("AJAX Error?");
    API.errorCount += 1;
    if (API.errorCount > 20) {
      setTimeout(API.fetchUpdates, 5000);
    } else if (API.errorCount > 2) {
      setTimeout(API.fetchUpdates, 1000);
    } else {
      API.fetchUpdates(API.updateCount);
    }
  },
  fetchUpdates: function(ucount) {
    if (!ucount) {
      ucount = API.updateCount;
    }
    API.updateObject = $.ajax({
      url: API.apibase + '/action/updates',
      async: true,
      data: {updateCount: ucount},
      dataType: 'text',
      type: 'get',
      success: API.onSuccess,
      error: API.onError,
    });
  },
  fetchFirstUpdate: function(ucount) {
    API.updateObject = $.ajax({
      url: API.apibase + '/action/updates',
      async: true,
      data: {updateCount: -1},
      dataType: 'text',
      type: 'get',
      success: API.onFirstSuccess,
      error: API.onError,
    });
  },
  callQueue: $(function() {}), // I don't know why, but jQuery needs this?
  callQueueComplete: function() {
    API.callQueue.dequeue();
  },
  callAction: function(uri, args) {
    API.callQueue.queue(function() {
      $.ajax({
        url: API.apibase + '/action/' + uri,
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
  file: {
    write: function(filename, contents) {
      API.callAction('file.write', {file: filename, contents: contents});
    },
    read: function(filename, callback) {
      API.callQueue.queue(function() {
        $.ajax({
          url: API.apibase + '/user/' + API.loginname + '/files/' + filename,
          async: true,
          data: args,
          type: 'get',
          success: function(myData, textStatus, xhr) {
            callback(myData);
          },
          error: function() {
            if (API.file.onReadFail) {
              API.file.onReadFail({file: filename, cause: "File not found."});
            }
          }
        });
      });
    }
  },
  user: {
  },
  alert: function(msg) {
    /* OVERRIDE ME */
    alert(msg);
  },
  flush: function() {
    /* OVERRIDE ME */
  },
  onReady: function() {
    /* OVERRIDE ME */
  },
};

$(window).ready(function() {
  // Timeout is needed to prevent Chrome from thinking we're still
  // loading the document with fetchUpdates.
  setTimeout(API.fetchUpdates, 500);
});
