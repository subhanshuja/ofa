function $(o) {return document.getElementById(o);}

function sendCommand(cmd) {
  window.domAutomationController.setAutomationId(1);
  window.domAutomationController.send(cmd);
}

// Should match SSLBlockingPageCommands in ssl_blocking_page.cc.
var CMD_DONT_PROCEED = 0;
var CMD_PROCEED = 1;
var CMD_RELOAD = 2;

function setupEvents() {
  var overridable = loadTimeData.getBoolean('overridable');

  $('back-button').addEventListener('click', function() {
    sendCommand(CMD_DONT_PROCEED);
  });

  $('proceed-button').addEventListener('click', function(event) {
    if (overridable) {
      sendCommand(CMD_PROCEED);
    } else {
      sendCommand(CMD_RELOAD);
    }
  });

  document.addEventListener('contextmenu', function(e) {
    e.preventDefault();
  });
}

document.addEventListener('DOMContentLoaded', setupEvents);
