/* -*- Mode: javascript; indent-tabs-mode: nil; c-basic-offset: 4; -*-
 *
 * Copyright (C) 2015 Opera Software ASA. All rights reserved.
 *
 * This file is an original work developed by Opera Software ASA
 */

cr.define('cobrand', function() {
  'use strict';

  // List of predefined transcoder clusters
  const kTranscoderClusters = [
    'campaign',
    'cust-demo',
    'dev-odp',
    'developer',
    'global-4',
    'global-next',
    'test-bifrost',
    'test-glb',
    'test-global',
    'test-mini4',
    'test-mxp',
    'test-odp',
    'test-oem',
    'test-qa'
  ];

  /**
   * Interface for a simple notification box appearing at the bottom of the
   * viewport. Exposes two methods:
   *  show(text)
   *  hide()
   */
  var notificationBox = new (function() {
    var mNotificationTimer;

    this.hide = function() {
      var messageBox = queryRequiredElement('.message-box');
      messageBox.classList.add('invisible');
      mNotificationTimer = null;
    };

    this.show = function(text) {
      var messageBox = queryRequiredElement('.message-box');
      var content = queryRequiredElement('.content', messageBox);

      if (mNotificationTimer) {
        clearTimeout(mNotificationTimer);
        mNotificationTimer = null;
      }

      content.innerHTML = text;
      messageBox.classList.remove('invisible');
      mNotificationTimer = setTimeout(this.hide, 3000);
    };
  })();

  /**
   * Builds a string suitable for GET query URIs.
   * @param {Object} params Dictionary containing key value pairs.
   * @return {string} The constructed query string.
   */
  function createQueryString(params) {
    function* SeparatorGenerator() {
      yield '?';
      while (true) {
        yield '&';
      }
    }

    var query = '';
    var separators = SeparatorGenerator();

    for (var p in params) {
      query += separators.next().value;
      query += encodeURIComponent(p) + '=' + encodeURIComponent(params[p]);
    }

    return query;
  }

  /**
   * Creates a key-value-pair dictionary based on the provided form element as
   * an OBML browser would create it.
   * @param {HTMLFormElement} form The form to use
   * @return {Object} Dictionary containing key value pairs.
   */
  function buildFormMap(form) {
    var map = {};

    for (var i = 0; i < form.elements.length; i++) {
      var element = form.elements[i];
      if (element.type == 'text') {
        map[element.name] = element.value;
      } else if (element.type == 'checkbox') {
        map[element.name] = element.checked ? 'yes' : 'no';
      } else if (element.type == 'select') {
        map[element.name] = element.selectedOptions[0].value;
      }
    }

    return map;
  }

  /**
   * Sends a message to make a request.
   * Shows a notification.
   * @param {string} url The request url.
   */
  function makeRequest(url) {
    chrome.send('makeRequest', [url]);
    notificationBox.show(url);
  }

  /**
   * Submits a form.
   * @param {HTMLFormElement} form The form to submit.
   */
  function submitForm(form) {
    makeRequest(form.action + createQueryString(buildFormMap(form)));
  }

  /**
   * Switches the active transcoder cluster.
   * @param {string} cluster The cluster to switch to.
   * @param {string} password The password to use.
   */
  function switchCluster(cluster, password) {
    var params = {
      cluster: cluster
    };

    if (password) {
      params['password'] = password;
    }

    makeRequest('server:cs' + createQueryString(params));
  }

  /**
   * Resets the active cluster.
   */
  function resetCluster(password) {
    var params = {
      reset: '1',
      password: password
    };

    makeRequest('server:cs' + createQueryString(params));
  }

  /**
   * Switches the operator.
   * @param {string} operator The Operator to switch to.
   */
  function switchOperator(operator) {
      makeRequest('operator:/' + operator);
  }

  /**
   * Resets the overridden operator to the default.
   */
  function clearOperator() {
      switchOperator('clear');
  }

  /**
   * Potentially sends a request based on the location string. For instance,
   * chrome://cobrand-setting/apa:bepa?cepa would issue a request for
   * "apa:bepa?cepa".
   */
  function maybeRequestFromLocation() {
    if (window.location.pathname != '/') {
      // +1 for '/' appending the origin
      var url = window.location.href.slice(window.location.origin.length + 1);
      makeRequest(url);
    }
  }

  function populateForm() {
    var clusterSelector = document.querySelector(
        'form[name=clusterSwitchForm] select');

    var add = function(str) {
      var option = document.createElement('option');
      option.text = str;
      clusterSelector.add(option);
    };

    kTranscoderClusters.map(add);

    add('Custom..');
  }

  function installEventListeners() {
    var setFormHandler = function(form, handler) {
        form.addEventListener('submit', function(event) {
            event.preventDefault();

            handler.apply(handler, arguments);

            // Hide the IME
            document.activeElement.blur();
            return false;
        });
    };

    var toggleSection = function(section, event) {
      section.classList.toggle('collapsed');
    };

    var sectionHeads = document.querySelectorAll('body > section > h1');
    for (var i = 0; i < sectionHeads.length; i++) {
      var header = sectionHeads[i];
      var section = header.parentElement;
      header.addEventListener('click', toggleSection.bind(window, section));
    }

    var clusterForm = queryRequiredElement('form[name=clusterSwitchForm]');
    setFormHandler(clusterForm, function(event) {
      var cluster = event.target.querySelector('input[type=text]');
      var passwordElement = event.target.querySelector('input[type=password]');

      if (!cluster.value) {
        cluster = event.target.querySelector('select').selectedOptions[0];
      }

      switchCluster(cluster.value, passwordElement.value);
    });

    var clusterSelector = queryRequiredElement('select', clusterForm);
    clusterSelector.addEventListener('change', function(event) {
      var customLabel =
        event.target.form.querySelector('input[type=text]').parentElement;

      // The custom option always comes last
      if (event.target.selectedIndex == event.target.options.length - 1) {
        customLabel.classList.remove('hidden');
      } else {
        customLabel.classList.add('hidden');
      }
    });

    // The reset button sits outside the form
    var clusterFormParent = clusterForm.parentElement;
    var clusterResetButton = queryRequiredElement('button', clusterFormParent);
    clusterResetButton.addEventListener('click', function() {
      var password = queryRequiredElement('input[type=password]', clusterForm);
      resetCluster(password.value);
    });

    var operatorForm = queryRequiredElement('form[name=operatorForm]');
    setFormHandler(operatorForm, function(event) {
        var operator = queryRequiredElement('input[type=text]', event.target);
        switchOperator(operator.value);
    });

    var operatorResetButton = queryRequiredElement(
            'button', operatorForm.parentElement);
    operatorResetButton.addEventListener('click', function(event) {
        clearOperator();
    });

    var autoForms = document.querySelectorAll('form[data-auto-form]');
    for (var i = 0; i < autoForms.length; i++) {
        setFormHandler(autoForms[i], function(event) {
            submitForm(event.target);
        });
    }

    var requestForm = queryRequiredElement('form[name=genericRequestForm]');
    setFormHandler(requestForm, function(event) {
      var inputField = event.target.querySelector('input');
      makeRequest(inputField.value);
    });

    // Dismiss the message box by clicking on it
    var messageBox = queryRequiredElement('.message-box');
    messageBox.addEventListener('click', function(event) {
      notificationBox.hide();
    });
  }

  function contentLoaded() {
    installEventListeners();
    populateForm();
    maybeRequestFromLocation();
  }

  return {
    contentLoaded: contentLoaded
  };
});

document.addEventListener('DOMContentLoaded', cobrand.contentLoaded);
