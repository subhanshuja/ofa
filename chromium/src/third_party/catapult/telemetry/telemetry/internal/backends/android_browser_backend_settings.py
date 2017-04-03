# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from telemetry.core import exceptions


class AndroidBrowserBackendSettings(object):

  def __init__(self, activity, cmdline_file, package, pseudo_exec_name,
               supports_tab_control):
    self._activity = activity
    self._cmdline_file = cmdline_file
    self._package = package
    self._action = None
    self._extras = None
    self._pseudo_exec_name = pseudo_exec_name
    self._supports_tab_control = supports_tab_control

  @property
  def activity(self):
    return self._activity

  @property
  def package(self):
    return self._package

  @property
  def pseudo_exec_name(self):
    return self._pseudo_exec_name

  @property
  def supports_tab_control(self):
    return self._supports_tab_control

  def GetCommandLineFile(self, is_user_debug_build):
    del is_user_debug_build  # unused
    return self._cmdline_file

  def GetBrowserSpecificStartupArgs(self, browser_options):
      return []

  def GetDevtoolsRemotePort(self, device):
    raise NotImplementedError()

  @property
  def profile_ignore_list(self):
    # Don't delete lib, since it is created by the installer.
    return ['lib']

class ChromeBackendSettings(AndroidBrowserBackendSettings):
  # Stores a default Preferences file, re-used to speed up "--page-repeat".
  _default_preferences_file = None

  def GetCommandLineFile(self, is_user_debug_build):
    if is_user_debug_build:
      return '/data/local/tmp/chrome-command-line'
    else:
      return '/data/local/chrome-command-line'

  def __init__(self, package):
    super(ChromeBackendSettings, self).__init__(
        activity='com.google.android.apps.chrome.Main',
        cmdline_file=None,
        package=package,
        pseudo_exec_name='chrome',
        supports_tab_control=True)

  def GetDevtoolsRemotePort(self, device):
    return 'localabstract:chrome_devtools_remote'

class OperaBackendSettings(ChromeBackendSettings):

  @staticmethod
  def _GetCommandLineFile(adb):
    if adb.IsUserBuild():
      return '/data/local/tmp/opera-browser-command-line'
    else:
      return '/data/local/opera-browser-command-line'

  def __init__(self, adb, package):
    super(OperaBackendSettings, self).__init__(adb=adb, package=package)
    self.activity='com.opera.Opera'
    self.cmdline_file=OperaBackendSettings._GetCommandLineFile(adb)
    self.pseudo_exec_name='opera'
    self.supports_tab_control=True
    # Dismiss the intro dialog on startup
    self._action = 'com.opera.android.action.PROTECTED_INTENT'
    self._extras = {'cmd': 'DISMISS_INTRO', 'id': 'OppiumAutoBot'}

  def GetDevtoolsRemotePort(self):
    return 'localabstract:opera_devtools_remote'

  def GetBrowserSpecificStartupArgs(self, browser_options):
    args = []
    user_agent = browser_options.browser_user_agent_type
    if user_agent:
      args.append('--set-user-agent=%s' % user_agent)
    return args

class ContentShellBackendSettings(AndroidBrowserBackendSettings):
  def __init__(self, package):
    super(ContentShellBackendSettings, self).__init__(
        activity='org.chromium.content_shell_apk.ContentShellActivity',
        cmdline_file='/data/local/tmp/content-shell-command-line',
        package=package,
        pseudo_exec_name='content_shell',
        supports_tab_control=False)

  def GetDevtoolsRemotePort(self, device):
    return 'localabstract:content_shell_devtools_remote'


class WebviewBackendSettings(AndroidBrowserBackendSettings):
  def __init__(self,
               package,
               activity='org.chromium.webview_shell.TelemetryActivity',
               cmdline_file='/data/local/tmp/webview-command-line'):
    super(WebviewBackendSettings, self).__init__(
        activity=activity,
        cmdline_file=cmdline_file,
        package=package,
        pseudo_exec_name='webview',
        supports_tab_control=False)

  def GetDevtoolsRemotePort(self, device):
    # The DevTools socket name for WebView depends on the activity PID's.
    retries = 0
    timeout = 1
    pid = None
    while True:
      pids = device.GetPids(self.package)
      if not pids or self.package not in pids:
        time.sleep(timeout)
        retries += 1
        timeout *= 2
        if retries == 4:
          logging.critical('android_browser_backend: Timeout while waiting for '
                           'activity %s:%s to come up',
                           self.package,
                           self.activity)
          raise exceptions.BrowserGoneException(self.browser,
                                                'Timeout waiting for PID.')
      if len(pids[self.package]) > 1:
        raise Exception(
            'At most one instance of process %s expected but found pids: '
            '%s' % (self.package, pids))
      pid = pids[self.package][0]
      break
    return 'localabstract:webview_devtools_remote_%s' % str(pid)


class WebviewShellBackendSettings(WebviewBackendSettings):
  def __init__(self, package):
    super(WebviewShellBackendSettings, self).__init__(
        activity='org.chromium.android_webview.shell.AwShellActivity',
        cmdline_file='/data/local/tmp/android-webview-command-line',
        package=package)
