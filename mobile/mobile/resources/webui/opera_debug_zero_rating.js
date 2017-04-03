cr.define('debug_zero_rating_ui', function() {
  'use strict';

  function fetchRules() {
    chrome.send('fetchRules');
  }

  function toggleDebug() {
    chrome.send('toggleDebug');
  }

  function clearConsole() {
    $('logConsole').innerHTML = "";
    $('clearConsole').disabled = true;
  }

  function mccmncChanged() {
    var mcc = $('mcc').value;
    var mnc = $('mnc').value;

    chrome.send('mccmncChanged', [mcc, mnc]);
  }

  function contentLoaded() {
    chrome.send('contentLoaded');

    $('fetchRules').addEventListener('click', fetchRules);
    $('toggleDebug').addEventListener('click', toggleDebug);
    $('clearConsole').addEventListener('click', clearConsole);
    $('clearConsole').disabled = true;
    setDebugState(false);
  }

  function enableSpoofing() {
    $('mcc').disabled = false;
    $('mcc').addEventListener('change', mccmncChanged);

    $('mnc').disabled = false;
    $('mnc').addEventListener('change', mccmncChanged);
  }

  function logMessage(str) {
    $('logConsole').insertAdjacentHTML('afterbegin', str + '<br />');
    $('clearConsole').disabled = false;
  }

  function logWarning(str) {
    $('logConsole').insertAdjacentHTML(
        'afterbegin', '<font color="red">' + str + '</font><br />');
    $('clearConsole').disabled = false;
  }

  function setDebugState(debugging_on) {
    if (debugging_on) {
      $('toggleDebug').value = 'Stop debugging';
    } else {
      $('toggleDebug').value = 'Start debugging';
    }
  }

  function setVisibleMCCMNC(mcc, mnc) {
    $('mcc').value = mcc;
    $('mnc').value = mnc;
  }

  return {
    contentLoaded: contentLoaded,
    logMessage: logMessage,
    logWarning: logWarning,
    enableSpoofing: enableSpoofing,
    setDebugState: setDebugState,
    setVisibleMCCMNC: setVisibleMCCMNC,
  }
});

document.addEventListener('DOMContentLoaded', debug_zero_rating_ui.contentLoaded);
