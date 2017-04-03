#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This is a python sync server used for testing Chrome Sync.

By default, it listens on an ephemeral port and xmpp_port and sends the port
numbers back to the originating process over a pipe. The originating process can
specify an explicit port and xmpp_port if necessary.
"""

import asyncore
import errno
import gzip
import os
import select
import StringIO
import socket
import sys
import urlparse

import chromiumsync
import testserver_base
import xmppserver
import opera_auth
import opera_intercept_auth
import opera_real_auth
import opera_true_auth
import opera_oauth2_auth
import random
import json
import urllib
import threading
import mimetypes
from functools import partial
from opera_http import OperaHTTP


class OperaFlags:

  def __init__(self):
    self.flags = {}

    # Set the default value for each new flag

    # auth_verification: Enables the controlled auth backend used for
    # sync token browser tests. Hardly usable outside of a sync token
    # browser test.
    self.Set('auth_verification', False)
    self.Set('expectations_debug', False)
    self.Set('account_access_token_delay', False)
    self.Set('client_request_delay', False)

    # real_auth: Enables another auth backend that aims at simulation of
    # the auth.opera.com service (fixed user list, password verification,
    # token verification, ...)
    self.Set('real_auth', False)

    # intercept_auth: This is a bit tricky mode that will allow controlling
    # the auth when the test server is used with a OfA client with the
    # sync and auth URLs overriden.
    self.Set('intercept_auth', False)

    # true_auth: The most complete auth backend, requires a valid user account
    # with a valid password to login, allows setting arbitrary authorization
    # errors on demand, allows viewing event log.
    self.Set('true_auth', False)

    # oauth2_auth: Allows basic response control for OAuth2.
    self.Set('oauth2_auth', False)

  def Set(self, name, value):
    self.flags[name] = value

  def Get(self, name):
    return self.flags[name]


class OperaSyncHandlerLoggingDelegate:

  def __init__(self, sync_http_server):
    self._sync_http_server = sync_http_server

  def on_sync_command(self, command_info_dict):
    if self._sync_http_server.GetOperaFlags().Get('true_auth') or \
       self._sync_http_server.GetOperaFlags().Get('oauth2_auth'):
      self._sync_http_server.GetOperaAuthServer().on_sync_command(
          command_info_dict)


# mzajaczkowski: Inheriting from testserver_base.ClientRestrictingServerMixIn
# causes the server to ignore requests made to an address different than
# the address that the server was binding to.
# This forbids using "0.0.0.0" for binding to all interfaces.
class SyncHTTPServer(  # testserver_base.ClientRestrictingServerMixIn,
    testserver_base.BrokenPipeHandlerMixIn,
    testserver_base.StoppableHTTPServer):
  """An HTTP server that handles sync commands."""

  def __init__(self, server_address, xmpp_port, request_handler_class):
    testserver_base.StoppableHTTPServer.__init__(self, server_address,
                                                 request_handler_class)
    self._opera_flags = OperaFlags()
    self._sync_handler = chromiumsync.TestServer()
    self._xmpp_socket_map = {}
    self._xmpp_server = xmppserver.XmppServer(self._xmpp_socket_map,
                                              (server_address[0], xmpp_port),
                                              self._opera_flags)
    self._current_auth_type = None
    self.configure_auth('token_test_auth')

    self._xmpp_server.SetOperaAuthServer(self._opera_auth)

    self._sync_handler.SetXMPPServer(self._xmpp_server)
    self._sync_handler.SetOperaAuthServer(self._opera_auth)
    self._sync_handler.SetOperaFlags(self._opera_flags)

    self.xmpp_port = self._xmpp_server.getsockname()[1]
    self.authenticated = True

    self.do_close_socket_ = True

    self._opera_cmdline = "commandline not available when run from opauto"

    self._current_command_delay = 'no_delay'

    self._opera_sync_logging_delegate = OperaSyncHandlerLoggingDelegate(self)
    self._sync_handler.SetLoggingDelegate(self._opera_sync_logging_delegate)

  def set_opera_cmdline(self, opera_cmdline):
    self._opera_cmdline = opera_cmdline

  def configure_auth(self, auth_type):
    if auth_type == self._current_auth_type:
      print "SYNC: WARNING: Auth reconfiguration requested for same type, ignored (%s)" % (
          auth_type)
      return

    self._current_auth_type = auth_type

    if auth_type == 'none':
      print "SYNC: Disabling auth backend"
      self._opera_auth = None
    elif auth_type == 'token_test_auth':
      print "SYNC: Enabling the sync token test auth backend"
      self._opera_auth = opera_auth.OperaAuth(self._opera_flags)
    elif auth_type == 'real_auth':
      print "SYNC: Enabling the 'real' auth backend"
      send_trick_invalidation = partial(
          self._sync_handler.SendTrickInvalidation)
      self._opera_auth = opera_real_auth.OperaRealAuth(send_trick_invalidation)
    elif auth_type == 'intercept_auth':
      print "SYNC: Enabling the 'intercept_auth' backend"
      self._opera_auth = opera_intercept_auth.OperaInterceptAuth()
    elif auth_type == 'true_auth':
      print "SYNC: Enabling the 'true_auth' backend"
      self._opera_auth = opera_true_auth.OperaTrueAuth()
    elif auth_type == 'oauth2_auth':
      print "SYNC: Enabling the 'oauth2' backend"
      self._opera_auth = opera_oauth2_auth.OperaOAuth2Auth()
    else:
      raise RuntimeError("SYNC: Unknown auth backend type requested: '%s'" %
                         auth_type)
    self._xmpp_server.SetOperaAuthServer(self._opera_auth)

  def set_do_close_socket(self, do_close):
    self.do_close_socket_ = do_close

  def GetXmppServer(self):
    return self._xmpp_server

  def GetOperaAuthServer(self):
    return self._opera_auth

  def GetOperaFlags(self):
    return self._opera_flags

  def HandleEvent(self, query, raw_request):
    return self._sync_handler.HandleEvent(query, raw_request)

  def HandleCommand(self, query, raw_request):
    return self._sync_handler.HandleCommand(query, raw_request)

  def HandleRequestNoBlock(self):
    """Handles a single request.

    Copied from SocketServer._handle_request_noblock().
    """

    try:
      request, client_address = self.get_request()
    except socket.error:
      return
    if self.verify_request(request, client_address):
      try:
        self.process_request(request, client_address)
      except Exception:
        self.handle_error(request, client_address)
        self.close_request(request)

  def process_request(self, request, client_address):
    """Call finish_request.

      Overridden by ForkingMixIn and ThreadingMixIn.

      """
    # A call to the actual handler
    self.finish_request(request, client_address)
    # A call to socket.shutdown() + socket.close()
    # This is what we want to avoid in certain cases.
    if self.do_close_socket_:
      self.shutdown_request(request)
    else:
      self.set_do_close_socket(True)

  def SetAuthenticated(self, auth_valid):
    self.authenticated = auth_valid

  def GetAuthenticated(self):
    return self.authenticated

  def handle_request(self):
    """Adaptation of asyncore.loop"""

    def HandleXmppSocket(fd, socket_map, handler):
      """Runs the handler for the xmpp connection for fd.

      Adapted from asyncore.read() et al.
      """

      xmpp_connection = socket_map.get(fd)
      # This could happen if a previous handler call caused fd to get
      # removed from socket_map.
      if xmpp_connection is None:
        return
      try:
        handler(xmpp_connection)
      except (asyncore.ExitNow, KeyboardInterrupt, SystemExit):
        raise
      except:
        xmpp_connection.handle_error()

    read_fds = [self.fileno()]
    write_fds = []
    exceptional_fds = []

    for fd, xmpp_connection in self._xmpp_socket_map.items():
      is_r = xmpp_connection.readable()
      is_w = xmpp_connection.writable()
      if is_r:
        read_fds.append(fd)
      if is_w:
        write_fds.append(fd)
      if is_r or is_w:
        exceptional_fds.append(fd)

    try:
      read_fds, write_fds, exceptional_fds = (
          select.select(read_fds, write_fds, exceptional_fds))
    except select.error, err:
      if err.args[0] != errno.EINTR:
        raise
      else:
        return

    for fd in read_fds:
      if fd == self.fileno():
        self.HandleRequestNoBlock()
        return
      HandleXmppSocket(fd, self._xmpp_socket_map,
                       asyncore.dispatcher.handle_read_event)

    for fd in write_fds:
      HandleXmppSocket(fd, self._xmpp_socket_map,
                       asyncore.dispatcher.handle_write_event)

    for fd in exceptional_fds:
      HandleXmppSocket(fd, self._xmpp_socket_map,
                       asyncore.dispatcher.handle_expt_event)


class SyncPageHandler(testserver_base.BasePageHandler):
  """Handler for the main HTTP sync server."""

  def __init__(self, request, client_address, sync_http_server):
    get_handlers = [
        self.ChromiumSyncTimeHandler, self.ChromiumSyncMigrationOpHandler,
        self.ChromiumSyncCredHandler, self.ChromiumSyncXmppCredHandler,
        self.ChromiumSyncDisableNotificationsOpHandler,
        self.ChromiumSyncEnableNotificationsOpHandler,
        self.ChromiumSyncSendNotificationOpHandler,
        self.ChromiumSyncBirthdayErrorOpHandler,
        self.ChromiumSyncTransientErrorOpHandler,
        self.ChromiumSyncErrorOpHandler,
        self.ChromiumSyncSyncTabFaviconsOpHandler,
        self.ChromiumSyncCreateSyncedBookmarksOpHandler,
        self.ChromiumSyncEnableKeystoreEncryptionOpHandler,
        self.ChromiumSyncRotateKeystoreKeysOpHandler,
        self.ChromiumSyncEnableManagedUserAcknowledgementHandler,
        self.ChromiumSyncEnablePreCommitGetUpdateAvoidanceHandler,
        self.GaiaOAuth2TokenHandler, self.GaiaSetOAuth2TokenResponseHandler,
        self.OperaSyncSetXMPPAuthError, self.OperaSyncEnableOperaInvalidations,
        self.OperaControlHandler, self.ShowDataPageHandler,
        self.ChromiumSyncShowDataHandler, self.CustomizeClientCommandHandler,
        self.OperaAuthPrefixHandler, self.OperaResetDataHandler,
        self.OperaGetDataHandler, self.OperaEnableDelayedInvalidationsHandler,
        self.OperaAccountLoginPageHandler,
        self.OperaAccountLoginSuccessPageHandler,
        self.OperaAccountSignupPageHandler, self.OperaRealAuthHandler,
        self.OperaInterceptAuthHandler, self.OperaTrueAuthHandler,
        self.OperaPanelHandler, self.OperaXMPPSessionsHandler,
        self.OperaXMPPSessionCloseHandler, self.SetCommandDelayHandler,
        self.OperaOAuth2Handler
    ]

    post_handlers = [
        self.ChromiumSyncCommandHandler, self.ChromiumSyncTimeHandler,
        self.GaiaOAuth2TokenHandler, self.GaiaSetOAuth2TokenResponseHandler,
        self.OperaSyncCommandHandler, self.OperaSyncEventHandler,
        self.OperaAuthPrefixHandler, self.OperaPutDataHandler,
        self.OperaAccountAccessTokenPageHandler,
        self.OperaAccountLoginSuccessPageHandler,
        self.OperaServiceOauthSimpleAccessTokenPageHandler,
        self.OperaRealAuthHandler, self.OperaInterceptAuthHandler,
        self.OperaTrueAuthHandler, self.OperaOAuth2Handler
    ]

    self.do_close_files_ = True
    self.sync_http_server_ = sync_http_server

    self.client_address_ = client_address

    self.timeout = 1

    testserver_base.BasePageHandler.__init__(self, request, client_address,
                                             sync_http_server, [], get_handlers,
                                             [], post_handlers, [])

  def finish(self):
    # Copied from StreamRequestHandler.finish()
    if not self.wfile.closed:
      try:
        self.wfile.flush()
      except socket.error:
        # An final socket error may have occurred here, such as
        # the local error ECONNABORTED.
        pass
    if self.do_close_files_:
      self.wfile.close()
      self.rfile.close()

  def ChromiumSyncTimeHandler(self):
    """Handle Chromium sync .../time requests.

    The syncer sometimes checks server reachability by examining /time.
    """

    test_name = "/chromiumsync/time"
    if not self._ShouldHandleRequest(test_name):
      return False

    # Chrome hates it if we send a response before reading the request.
    if self.headers.getheader('content-length'):
      length = int(self.headers.getheader('content-length'))
      _raw_request = self.rfile.read(length)

    self.send_response(200)
    self.send_header('Content-Type', 'text/plain')
    self.end_headers()
    self.wfile.write('0123456789')
    return True

  def ChromiumSyncCommandHandler(self):
    """Handle a chromiumsync command arriving via http.

    This covers all sync protocol commands: authentication, getupdates, and
    commit.
    """

    test_name = "/chromiumsync/command"
    if not self._ShouldHandleRequest(test_name):
      return False

    length = int(self.headers.getheader('content-length'))
    raw_request = self.rfile.read(length)
    if self.headers.getheader('Content-Encoding'):
      encode = self.headers.getheader('Content-Encoding')
      if encode == "gzip":
        raw_request = gzip.GzipFile(
            fileobj=StringIO.StringIO(raw_request)).read()

    http_response = 200
    raw_reply = None
    if not self.server.GetAuthenticated():
      http_response = 401
      challenge = 'GoogleLogin realm="http://%s", service="chromiumsync"' % (
          self.server.server_address[0])
    else:
      http_response, raw_reply = self.server.HandleCommand(self.path,
                                                           raw_request)

    ### Now send the response to the client. ###
    self.send_response(http_response)
    if http_response == 401:
      self.send_header('www-Authenticate', challenge)
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncMigrationOpHandler(self):
    test_name = "/chromiumsync/migrate"
    if not self._ShouldHandleRequest(test_name):
      return False

    http_response, raw_reply = self.server._sync_handler.HandleMigrate(
        self.path)
    self.send_response(http_response)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncCredHandler(self):
    test_name = "/chromiumsync/cred"
    if not self._ShouldHandleRequest(test_name):
      return False
    try:
      query = urlparse.urlparse(self.path)[4]
      cred_valid = urlparse.parse_qs(query)['valid']
      if cred_valid[0] == 'True':
        self.server.SetAuthenticated(True)
      else:
        self.server.SetAuthenticated(False)
    except Exception:
      self.server.SetAuthenticated(False)

    http_response = 200
    raw_reply = 'Authenticated: %s ' % self.server.GetAuthenticated()
    self.send_response(http_response)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncXmppCredHandler(self):
    test_name = "/chromiumsync/xmppcred"
    if not self._ShouldHandleRequest(test_name):
      return False
    xmpp_server = self.server.GetXmppServer()
    try:
      query = urlparse.urlparse(self.path)[4]
      cred_valid = urlparse.parse_qs(query)['valid']
      if cred_valid[0] == 'True':
        xmpp_server.SetAuthenticated(True)
      else:
        xmpp_server.SetAuthenticated(False)
    except:
      xmpp_server.SetAuthenticated(False)

    http_response = 200
    raw_reply = 'XMPP Authenticated: %s ' % xmpp_server.GetAuthenticated()
    self.send_response(http_response)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncDisableNotificationsOpHandler(self):
    test_name = "/chromiumsync/disablenotifications"
    if not self._ShouldHandleRequest(test_name):
      return False
    self.server.GetXmppServer().DisableNotifications()
    result = 200
    raw_reply = ('<html><title>Notifications disabled</title>'
                 '<H1>Notifications disabled</H1></html>')
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def OperaSyncSetXMPPAuthError(self):
    test_name = "/chromiumsync/setxmppautherror"
    if not self._ShouldHandleRequest(test_name):
      return False
    query = urlparse.urlparse(self.path)[4]
    query_params = urlparse.parse_qs(query)
    auth_error = opera_auth.AUTH_ERROR_OK
    if 'auth_error' in query_params:
      auth_error = int(query_params['auth_error'][0])
    self.server.GetXmppServer().SetAuthError(auth_error)
    result = 200
    raw_reply = ('<html><title>Opera XMPP auth error changed.</title>'
                 '<H1>Opera XMPP auth error changed to %s</H1></html>') % (
                     auth_error)

    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncEnableNotificationsOpHandler(self):
    test_name = "/chromiumsync/enablenotifications"
    if not self._ShouldHandleRequest(test_name):
      return False
    self.server.GetXmppServer().EnableNotifications()
    result = 200
    raw_reply = ('<html><title>Notifications enabled</title>'
                 '<H1>Notifications enabled</H1></html>')
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def OperaSyncEnableOperaInvalidations(self):
    test_name = "/chromiumsync/enableoperainvalidations"
    if not self._ShouldHandleRequest(test_name):
      return False
    self.server.GetXmppServer().EnableOperaInvalidations()
    result = 200
    raw_reply = ('<html><title>Opera invalidations enabled.</title>'
                 '<H1>Opera invalidations enabled</H1></html>')
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def OperaEnableDelayedInvalidationsHandler(self):
    test_name = "/opera/enabledelayedinvalidations"
    if not self._ShouldHandleRequest(test_name):
      return False
    self.server.GetXmppServer().EnableOperaDelayedInvalidations()
    result = 200
    raw_reply = ('<html><title>Delayed Opera invalidations enabled.</title>'
                 '<H1>Delayed Opera invalidations enabled</H1></html>')
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def DoSendOperaAccountAccessToken(self, username):
    """ We need to send some content so that it is possible to save the
        credentials in the password manager in case the /account/login
        page is openend in a tab instead of a popup.
        This is not affecting the popup flow at the time this is written.
    """
    html = "<html><head><title>ALL OK</title></head><body>Logged in OK! You" \
           " can now save the credentials in password manager.</body></html>"

    try:
      user_name = ""
      user_email = ""
      username_split = username.split('@')
      if len(username_split) == 1:
        user_name = username
        user_email = "%s@operapythontestserver.mz" % username
      elif len(username_split) == 2:
        user_name = username_split[0]
        user_email = username
      else:
        raise Exception("User sent most weird login: '%s'" % username)

      TOKEN_LEN = 10
      TOKEN_SECRET_LEN = 20

      generated_token = chromiumsync.GenerateOperaAuthToken(TOKEN_LEN)
      generated_secret = chromiumsync.GenerateOperaAuthToken(TOKEN_SECRET_LEN)

      self.send_response(200)
      self.send_header('Content-Type', 'text/html')
      self.send_header('Content-Length', len(html))

      self.send_header('X-Opera-Auth-AccessToken', generated_token)
      self.send_header('X-Opera-Auth-AccessTokenSecret', generated_secret)
      self.send_header('X-Opera-Auth-UserName', user_name)
      self.send_header('X-Opera-Auth-UserID', '123456789')
      self.send_header('X-Opera-Auth-UserEmail', user_email)
      self.send_header('X-Opera-Auth-EmailVerified', '1')

      print "SYNC: Access token '%s' issued for user_name='%s' and user_email='%s'" % (
          generated_token, user_name, user_email)

      self.end_headers()
      self.wfile.write(html)

      self._FinalizeAsyncRequest()
    except Exception as e:
      print "SYNC: Could not send access token response, did client disconnect? (%s)" % (
          str(e))

  def OperaAccountAccessTokenPageHandler(self):
    test_name = "/account/access-token"
    if not self._ShouldHandleRequest(test_name):
      return False

    if self.server.GetOperaFlags().Get('true_auth'):
      response = self.server.GetOperaAuthServer().handle_request(
          self._get_request_vars())
      return self._send_response(response)

    if self.server.GetOperaFlags().Get('real_auth'):
      response = self.server.GetOperaAuthServer().do_handle(
          self._get_request_vars())
      return self._send_response(response)

    postvars = None
    content_type = self.headers.getheader('content-type')
    if content_type == "application/x-www-form-urlencoded":
      length = int(self.headers.getheader('content-length'))
      postvars = urlparse.parse_qs(self.rfile.read(length), keep_blank_values=1)

    username = ""
    password = ""

    if postvars is not None:
      username = postvars.get('email', '')
      password = postvars.get('password', '')

    if isinstance(username, list):
      username = username[0]
    if isinstance(password, list):
      password = password[0]

    if not username:
      print "SYNC: ERROR: Access token cannot be issued for empty username!"
      self.send_response(500)
      self.end_headers()
      return True

    if not password:
      print "SYNC: WARNING: Client sent empty password, ignoring."
      # TODO(mzajaczkowski): Should be fatal probably.

    if self.server.GetOperaFlags().Get('account_access_token_delay'):
      print "SYNC: Access token request is being delayed due to client_request_delay..."
      self._MarkThisRequestAsync()
      handler = partial(self.DoSendOperaAccountAccessToken, username)
      threading.Timer(5, handler).start()
    else:
      self.DoSendOperaAccountAccessToken(username)

    return True

  def OperaServiceOauthSimpleAccessTokenPageHandler(self):
    test_name = "/service/oauth/simple/access_token"
    if not self._ShouldHandleRequest(test_name):
      return False

    if self.server.GetOperaFlags().Get('true_auth'):
      response = self.server.GetOperaAuthServer().handle_request(
          self._get_request_vars())
      return self._send_response(response)

    if self.server.GetOperaFlags().Get('real_auth'):
      response = self.server.GetOperaAuthServer().do_handle(
          self._get_request_vars())
      return self._send_response(response)

    postvars = None
    content_type = self.headers.getheader('content-type')
    if content_type == "application/x-www-form-urlencoded":
      length = int(self.headers.getheader('content-length'))
      postvars = urlparse.parse_qs(self.rfile.read(length), keep_blank_values=1)

    # {'x_username': ['a'], 'x_password': ['b'], 'x_consumer_key': ['LDKgAObtMIV1eVp1Jb2b0ZlqK1TUDkk3']}
    username = postvars.get('x_username', '')
    password = postvars.get('x_password', '')

    if isinstance(username, list):
      username = username[0]
    if isinstance(password, list):
      password = password[0]

    result = {}

    if not username:
      print "SYNC: ERROR: Can't login without username"
      result['error'] = "No username given!"
    elif not password:
      print "SYNC: ERROR: Can't login without password"
      result['error'] = "No password given!"
    else:
      # TODO(mzajaczkowski): We should probably allow simulation of even more exteme
      # error conditions - that the auth service always returns same token. Client
      # may misbehave.
      result['oauth_token'] = chromiumsync.GenerateOperaAuthToken(10)
      result['oauth_token_secret'] = chromiumsync.GenerateOperaAuthToken(20)

      print "SYNC: Issued access token '%s' for login '%s' and password '%s' using the simple flow." % (
          result['oauth_token'], username, password)

    result_string = urllib.urlencode(result)
    self.send_response(200)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(result_string))
    self.end_headers()
    self.wfile.write(result_string)
    return True

  def OperaAccountLoginPageHandler(self):
    test_name = "/account/login"
    if not self._ShouldHandleRequest(test_name):
      return False

    if self.server.GetOperaFlags().Get('oauth2_auth'):
      return False

    req_vars = self._get_request_vars()

    if self.server.GetOperaFlags().Get('true_auth'):
      response = self.server.GetOperaAuthServer().handle_request(
          req_vars)
      return self._send_response(response)

    # In case the 'continue' GET parameter was not given with the request, use
    # default of /account/login_success - this is what will happen when the
    # /account/login page is opened from within a browser tab rather than from
    # within the sync popup.
    file_name = OperaHTTP.AsDataDirPath('login.html')

    html = \
      open(file_name, 'r').\
      read()

    error_string = req_vars['get'].get('error', '')
    continue_url = req_vars['get'].get('continue', '')
    if not continue_url:
      continue_url = '/account/login_success'

    x_desktop_client = 0
    x_desktop_client_header = self.headers.getheader('x-desktop-client')
    if x_desktop_client_header:
      x_desktop_client = int(x_desktop_client_header)

    html = html.replace('[SYNC_SERVER_ERROR_STRING]', error_string)
    html = html.replace('[SYNC_SERVER_CONTINUE_URL]', continue_url)
    print "SYNC: Sync login page request with continue_url = '%s' " \
          " (x-desktop-client = '%s')" % (continue_url, x_desktop_client)

    self.send_response(200)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(html))
    self.end_headers()
    self.wfile.write(html)
    return True

  def OperaAccountLoginSuccessPageHandler(self):
    test_name = '/account/login_success'
    if not self._ShouldHandleRequest(test_name):
      return False

    print "SYNC: Login success with normal page."

    html = "<html><head><title>ALL OK</title></head><body>Logged in OK! You can" \
        " now save the credentials in password manager.</body></html>"
    self.send_response(200)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(html))
    self.end_headers()
    self.wfile.write(html)
    return True

  def OperaAccountSignupPageHandler(self):
    test_name = "/account/signup"
    if not self._ShouldHandleRequest(test_name):
      return False

    print "SYNC: Sync register page request"
    html = "You cannot register an account with the testing server."
    self.send_response(200)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(html))
    self.end_headers()
    self.wfile.write(html)
    return True

  def ChromiumSyncSendNotificationOpHandler(self):
    test_name = "/chromiumsync/sendnotification"
    if not self._ShouldHandleRequest(test_name):
      return False
    query = urlparse.urlparse(self.path)[4]
    query_params = urlparse.parse_qs(query)
    channel = ''
    data = ''
    if 'channel' in query_params:
      channel = query_params['channel'][0]
    if 'data' in query_params:
      data = query_params['data'][0]
    self.server.GetXmppServer().SendNotification(channel, data)
    result = 200
    raw_reply = ('<html><title>Notification sent</title>'
                 '<H1>Notification sent with channel "%s" '
                 'and data "%s"</H1></html>' % (channel, data))
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncBirthdayErrorOpHandler(self):
    test_name = "/chromiumsync/birthdayerror"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleCreateBirthdayError()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncTransientErrorOpHandler(self):
    test_name = "/chromiumsync/transienterror"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleSetTransientError()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncErrorOpHandler(self):
    test_name = "/chromiumsync/error"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleSetInducedError(
        self.path)
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncSyncTabFaviconsOpHandler(self):
    test_name = "/chromiumsync/synctabfavicons"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleSetSyncTabFavicons()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncCreateSyncedBookmarksOpHandler(self):
    test_name = "/chromiumsync/createsyncedbookmarks"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleCreateSyncedBookmarks()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncEnableKeystoreEncryptionOpHandler(self):
    test_name = "/chromiumsync/enablekeystoreencryption"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = (
        self.server._sync_handler.HandleEnableKeystoreEncryption())
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncRotateKeystoreKeysOpHandler(self):
    test_name = "/chromiumsync/rotatekeystorekeys"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = (self.server._sync_handler.HandleRotateKeystoreKeys())
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncEnableManagedUserAcknowledgementHandler(self):
    test_name = "/chromiumsync/enablemanageduseracknowledgement"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = (
        self.server._sync_handler.HandleEnableManagedUserAcknowledgement())
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def ChromiumSyncEnablePreCommitGetUpdateAvoidanceHandler(self):
    test_name = "/chromiumsync/enableprecommitgetupdateavoidance"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = (
        self.server._sync_handler.HandleEnablePreCommitGetUpdateAvoidance())
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def GaiaOAuth2TokenHandler(self):
    test_name = "/o/oauth2/token"
    if not self._ShouldHandleRequest(test_name):
      return False
    if self.headers.getheader('content-length'):
      length = int(self.headers.getheader('content-length'))
      _raw_request = self.rfile.read(length)
    result, raw_reply = (self.server._sync_handler.HandleGetOauth2Token())
    self.send_response(result)
    self.send_header('Content-Type', 'application/json')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def GaiaSetOAuth2TokenResponseHandler(self):
    test_name = "/setfakeoauth2token"
    if not self._ShouldHandleRequest(test_name):
      return False

    # The index of 'query' is 4.
    # See http://docs.python.org/2/library/urlparse.html
    query = urlparse.urlparse(self.path)[4]
    query_params = urlparse.parse_qs(query)

    response_code = 0
    request_token = ''
    access_token = ''
    expires_in = 0
    token_type = ''

    if 'response_code' in query_params:
      response_code = query_params['response_code'][0]
    if 'request_token' in query_params:
      request_token = query_params['request_token'][0]
    if 'access_token' in query_params:
      access_token = query_params['access_token'][0]
    if 'expires_in' in query_params:
      expires_in = query_params['expires_in'][0]
    if 'token_type' in query_params:
      token_type = query_params['token_type'][0]

    result, raw_reply = (self.server._sync_handler.HandleSetOauth2Token(
        response_code, request_token, access_token, expires_in, token_type))
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def CustomizeClientCommandHandler(self):
    test_name = "/customizeclientcommand"
    if not self._ShouldHandleRequest(test_name):
      return False

    query = urlparse.urlparse(self.path)[4]
    query_params = urlparse.parse_qs(query)

    if 'sessions_commit_delay_seconds' in query_params:
      sessions_commit_delay = query_params['sessions_commit_delay_seconds'][0]
      try:
        command_string = self.server._sync_handler.CustomizeClientCommand(int(
            sessions_commit_delay))
        response_code = 200
        reply = "The ClientCommand was customized:\n\n"
        reply += "<code>{}</code>.".format(command_string)
      except ValueError:
        response_code = 400
        reply = "sessions_commit_delay_seconds was not an int"
    else:
      response_code = 400
      reply = "sessions_commit_delay_seconds is required"

    self.send_response(response_code)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(reply))
    self.end_headers()
    self.wfile.write(reply)
    return True

  def ShowDataPageHandler(self):
    test_name = "/show_data"
    if not self._ShouldHandleRequest(test_name):
      return False

    file_name = OperaHTTP.AsDataDirPath('show_data.html')

    html = \
      open(file_name, 'r').\
      read()

    self.send_response(200)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(html))
    self.end_headers()
    self.wfile.write(html)
    return True

  def ChromiumSyncShowDataHandler(self):
    test_name = "/chromiumsync/showdata"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleShowData()
    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def OperaResetDataHandler(self):
    test_name = "/opera/reset_data"
    if not self._ShouldHandleRequest(test_name):
      return False
    result, raw_reply = self.server._sync_handler.HandleResetData()

    if self.server.GetOperaFlags().Get('true_auth'):
      self.server.GetOperaAuthServer().on_account_reset()

    self.send_response(result)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def OperaGetDataHandler(self):
    test_name = "/opera/get_data"
    if not self._ShouldHandleRequest(test_name):
      return False

    query = urlparse.urlparse(self.path)[4]
    query_params = urlparse.parse_qs(query)
    requested_format = query_params.get('format', ['protobuf'])[0]
    as_file = query_params.get('as_file', None)
    if isinstance(as_file, list):
      as_file = as_file[0]

    result, raw_reply = \
      self.server._sync_handler.HandleGetData(requested_format)

    content_type = 'text/html'
    if requested_format == 'json':
      content_type = 'text/json'
    elif requested_format == 'protobuf':
      content_type = 'application/octet-stream'

    self.send_response(result)
    self.send_header('Content-Type', content_type)
    self.send_header('Content-Length', len(raw_reply))
    if as_file:
      self.send_header('Content-Disposition', 'attachment; filename="%s.%s"' % \
        (as_file, requested_format))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def OperaPutDataHandler(self):
    test_name = "/opera/put_data"
    if not self._ShouldHandleRequest(test_name):
      return False

    length = int(self.headers.getheader('content-length'))
    raw_request = self.rfile.read(length)

    result, raw_reply = \
      self.server._sync_handler.HandlePutData('protobuf', raw_request)

    self.send_response(result)
    self.send_header('Content-Type', 'text/json')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    return True

  def OperaControlHandler(self):
    # TODO(mzajaczkowski): Change this to operaconf
    test_name = "/operacontrol/.*"
    if not self._ShouldHandleRequest(test_name):
      return False

    handled = True

    if self.path.endswith('enable_auth_verification'):
      self.server.GetOperaFlags().Set('auth_verification', True)
      self.server.configure_auth('token_test_auth')
    elif self.path.endswith('enable_expectations_debug'):
      self.server.GetOperaFlags().Set('expectations_debug', True)
    elif self.path.endswith('enable_account_access_token_delay'):
      self.server.GetOperaFlags().Set('account_access_token_delay', True)
    elif self.path.endswith('disable_account_access_token_delay'):
      self.server.GetOperaFlags().Set('account_access_token_delay', False)
    elif self.path.endswith('enable_client_request_delay'):
      self.server.GetOperaFlags().Set('client_request_delay', True)
    elif self.path.endswith('disable_client_request_delay'):
      self.server.GetOperaFlags().Set('client_request_delay', False)
    elif self.path.endswith('enable_real_auth'):
      self.server.GetOperaFlags().Set('real_auth', True)
      self.server.GetOperaFlags().Set('intercept_auth', False)
      self.server.GetOperaFlags().Set('true_auth', False)
      self.server.GetOperaFlags().Set('oauth2_auth', False)
      self.server.configure_auth('real_auth')
    elif self.path.endswith('disable_real_auth'):
      self.server.GetOperaFlags().Set('real_auth', False)
      self.server.configure_auth(None)
    elif self.path.endswith('enable_intercept_auth'):
      self.server.GetOperaFlags().Set('real_auth', False)
      self.server.GetOperaFlags().Set('intercept_auth', True)
      self.server.GetOperaFlags().Set('true_auth', False)
      self.server.GetOperaFlags().Set('oauth2_auth', False)
      self.server.configure_auth('intercept_auth')
    elif self.path.endswith('enable_true_auth'):
      self.server.GetOperaFlags().Set('real_auth', False)
      self.server.GetOperaFlags().Set('intercept_auth', False)
      self.server.GetOperaFlags().Set('true_auth', True)
      self.server.GetOperaFlags().Set('oauth2_auth', False)
      self.server.configure_auth('true_auth')
    elif self.path.endswith('enable_oauth2_auth'):
      self.server.GetOperaFlags().Set('real_auth', False)
      self.server.GetOperaFlags().Set('intercept_auth', False)
      self.server.GetOperaFlags().Set('true_auth', False)
      self.server.GetOperaFlags().Set('oauth2_auth', True)
      self.server.configure_auth('oauth2_auth')
    else:
      handled = False

    if handled:
      print "CONF: Handled '%s'" % self.path
      raw_reply = "<title>OK</title>OK"
    else:
      print "CONF: Could not handle '%s'" % self.path
      raw_reply = "<title>Not handled!</title>Not handled!"

    self.send_response(200)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)

    return True

  def SendOperaTimeSkewUpdateResponse(self, result):
    # NOTE: https://ssl.opera.com:8008/developerwiki/Auth.opera.com/ValidationAPI#Client.27s_clock_time_validation
    # does not define the 500 code! We're using this here to make it something very explicit.
    # Sample response configured via C++:
    # {u'status': u'success', u'code': 200, u'diff': u'NaN', u'message': u'', u'network_error': 0, u'http_code': 200}

    # Proper auth responses:
    #   Example response when clock is within 24 hours from the clock on auth.opera.com:
    #     {"status":"success","diff":"NaN","message":"","code":200}
    #
    #   Example responses when clock is more than 24 hours off from the clock on auth.opera.com:
    #     {"status":"error","diff":-100080,"message":"Clock differs too much","code":401}
    #     {"status":"error","diff":199890,"message":"Clock differs too much","code":401}
    #
    #   Example of server response on bad request:
    #     {"status":"error","diff":"NaN","message":"Bad request","code":400}

    network_error = result.get('network_error', 0)

    if network_error:
      # TODO(mzajaczkowski): Sufficient?
      self._FinalizeAsyncRequest()
      return

    http_code = result.get('http_code', 200)

    if http_code == 200:
      json_dict = {
          'status': result.get('status'),
          'diff': result.get('diff'),
          'message': result.get('message'),
          'code': result.get('code')
      }
      raw_reply = json.dumps(json_dict)
    else:
      raw_reply = ""

    print "Responding with HTTP %d, '%s'" % (http_code, raw_reply)

    self.send_response(http_code)
    # TODO(mzajaczkowski): text/json? What does auth return? Do we care?
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)
    self._FinalizeAsyncRequest()

  def SendOperaAccessTokenRenewalResponse(self, result):
    # https://ssl.opera.com:8008/developerwiki/Auth.opera.com/WebUI/AccessToken#Access_token_renewal_algorithm
    # Sample response configured via C++:
    # {u'status': u'success', u'code': 200, u'diff': u'NaN', u'message': u'', u'network_error': 0, u'http_code': 200}

    # Proper auth responses:
    #   An example of response structure in case of successful token renewal:
    #     {
    #       "auth_token": "ATxnCeCm3fh12d79baDaYprFKsn7GAq9",
    #       "auth_token_secret": "6JhxuBc97phBFthg83PZNZ25RwTUhxEy",
    #       "userID": "212784215",
    #       "userName": null,
    #       "userEmail": "authtestuser@gmail.com"
    #     }
    #
    #   An example of response structure in case of error:
    #     {"err_code":425,"err_msg":"Invalid Opera access token"}
    #
    # Error codes:
    # 420 (NOT_AUTHORIZED_REQUEST) - the request is not authorized, as the
    #     service is not allowed to request Opera access token;
    # 421 (BAD_REQUEST) - in case wrong request signature for example;
    # 422 (OPERA_USER_NOT_FOUND) - the owner of access token, Opera user is
    #     not found;
    # 424 (OPERA_TOKEN_NOT_FOUND) - access token not found;
    # 425 (INVALID_OPERA_TOKEN) - this Opera token has been exchanged for a
    #     new one already or has been invalidated by user;
    # 426 (COULD_NOT_GENERATE_OPERA_TOKEN) - more an internal error related
    #     to inability to issue a new Opera access token.
    # 428 (OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED) - the error indicates an attempt
    #     to exchange a valid active (not yet expired) token for a new one.

    network_error = result.get('network_error', 0)

    if network_error:
      # TODO(mzajaczkowski): Sufficient?
      self._FinalizeAsyncRequest()
      return

    http_code = result.get('http_code', 200)

    if http_code == 200:
      err_code = result.get('err_code')
      if err_code is None:
        json_dict = {
            'auth_token': result.get('auth_token'),
            'auth_token_secret': result.get('auth_token_secret'),
            'userID': result.get('userID', 0),
            'userName': result.get('userName', 'NO_USER_NAME'),
            'userEmail': result.get('userEmail', 'NO@EMAIL.COM')
        }
      else:
        json_dict = {'err_code': err_code, 'err_msg': result.get('err_msg')}
      raw_reply = json.dumps(json_dict)
    else:
      raw_reply = ""

    print "Responding with HTTP %d, '%s'" % (http_code, raw_reply)

    self.send_response(http_code)
    # TODO(mzajaczkowski): text/json? What does auth return? Do we care?
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)

    self._FinalizeAsyncRequest()

  def SendNotifyScriptFinishedResponse(self, response):
    self._send_response(response)
    self._FinalizeAsyncRequest()

  # Sends back a HTTP/HTML response to a synchronous request served by
  # the OperaAuthPrefixHandler() handler.
  # @params:
  # @response: A dictionary.
  #     Keys:
  #       http_code: The HTTP code to send back.
  #       title: The HTML title to send back. Note that SyncTest requires
  #         the title to be set in order to verify whether a request
  #         succeeded.
  #       json_dict: A dictionary that will be JSON-ized and sent back.
  #
  # Note that a response cannot have both title and json_dict, if it has
  # a warning is thrown and json_dict takes precendece.

  def SendOperaAuthPrefixResponse(self, response):
    http_code = 200
    if 'http_code' in response:
      http_code = response['http_code']
    else:
      print "WARNING: No http_code in response!"

    raw_reply = ""
    if 'json_dict' in response:
      raw_reply = json.dumps(response['json_dict'])
    elif 'title' in response:
      raw_reply = response['title']
    else:
      print "WARINING: No json_dict AND no title in response."

    self.send_response(http_code)
    self.send_header('Content-Type', 'text/html')
    self.send_header('Content-Length', len(raw_reply))
    self.end_headers()
    self.wfile.write(raw_reply)

  def OperaAuthPrefixHandler(self):

    def ShouldHandlePath(path):
      handled_urls = [opera_auth.SERVICE_VALIDATION_CLOCK,
                      opera_auth.ACCOUNT_ACCESSTOKEN_RENEWAL]
      if path.startswith(opera_auth.AUTH_PREFIX):
        return True
      else:
        for u in handled_urls:
          if path.endswith(u):
            return True

      return False

    parsed = urlparse.urlparse(self.path)
    if not ShouldHandlePath(parsed.path):
      return False

    if self.server.GetOperaFlags().Get('true_auth'):
      response = self.server.GetOperaAuthServer().handle_request(
          self._get_request_vars())
      return self._send_response(response)

    if self.server.GetOperaFlags().Get(
        'real_auth') or self.server.GetOperaFlags().Get('intercept_auth'):
      response = self.server.GetOperaAuthServer().do_handle(
          self._get_request_vars())
      return self._send_response(response)

    # TODO(mzajaczkowski): Fix the SyncTokenTest class.
    # if not self.server.GetOperaFlags().Get('auth_verification'):
    #   print "SYNC: Ignoring opera auth sync token test request as the backend has not been enabled."
    # return False

    synchronous_paths = [
        opera_auth.ADD_EXPECTATION, opera_auth.CHECK_STATE,
        opera_auth.POST_STEP, opera_auth.CLIENT_TRIGGER_FIRED,
        opera_auth.POST_ALLOW, opera_auth.POST_DISALLOW, opera_auth.LIST_STEPS
    ]

    async_paths = {
        opera_auth.SERVICE_VALIDATION_CLOCK:
            self.SendOperaTimeSkewUpdateResponse,
        opera_auth.ACCOUNT_ACCESSTOKEN_RENEWAL:
            self.SendOperaAccessTokenRenewalResponse,
        opera_auth.NOTIFY_SCRIPT_FINISHED:
            self.SendNotifyScriptFinishedResponse
    }

    postvars = None
    content_type = self.headers.getheader('content-type')
    if content_type == "application/x-www-form-urlencoded":
      length = int(self.headers.getheader('content-length'))
      postvars = urlparse.parse_qs(self.rfile.read(length), keep_blank_values=1)

    for path in synchronous_paths:
      if parsed.path.endswith(path):
        response = self.server.GetOperaAuthServer().HandleURL(parsed, postvars)
        self.SendOperaAuthPrefixResponse(response)
        return True

    for path in async_paths.keys():
      if parsed.path.endswith(path):
        self._MarkThisRequestAsync()
        self.server.GetOperaAuthServer().HandleURL(parsed, postvars,
                                                   async_paths[path])
        return True

    self.SendOperaAuthPrefixResponse(
        {'http_code': 404,
         'title': "No handler for '%s', check your request." % parsed.path})

  # Compare OnAuthHeaderVerificationResponseAvailable() in xmppserver.py for
  # information about the response contents.
  def SendBackOperaSyncCommandResponse(self, response):
    if self.server._current_command_delay is not 'no_delay':
      delay_string = self.server._current_command_delay
      delay_seconds = 0
      if delay_string == 'random_delay':
        delay_seconds = random.randint(1, 5)
      elif delay_string == 'no_delay':
        delay_seconds = 0
      else:
        delay_seconds = int(delay_string)
      print "SYNC: Client request is being delayed by '{}' ({} s.)".format(
          delay_string, delay_seconds)
      self._MarkThisRequestAsync()
      handler = partial(self.SendBackOperaSyncCommandResponseImpl, response)
      threading.Timer(delay_seconds, handler).start()
    elif self.server.GetOperaFlags().Get('client_request_delay'):
      print "SYNC: Client request is being delayed due to client_request_delay..."
      self._MarkThisRequestAsync()
      handler = partial(self.SendBackOperaSyncCommandResponseImpl, response)
      threading.Timer(5, handler).start()
    else:
      self.SendBackOperaSyncCommandResponseImpl(response)

  def SendBackOperaSyncCommandResponseImpl(self, response):
    if int(response.get('network_error', 0)) == opera_auth.NETWORK_ERROR_HARD:
      print "SYNC: Hard network error sent back to client!"
      self._FinalizeAsyncRequest()
      return True
    elif int(response.get('network_error', 0)) == opera_auth.NETWORK_ERROR_SOFT:
      print "SYNC: Transient auth error sent to client!"
      length = int(self.headers.getheader('content-length'))
      raw_request = self.rfile.read(length)
      self.server._sync_handler.SetOneTransietError()
      http_response, raw_reply = self.server.HandleCommand(self.path,
                                                           raw_request)
      self.send_response(http_response)
      self.end_headers()
      self.wfile.write(raw_reply)
      self._FinalizeAsyncRequest()
      return True
    elif response.get('http_error', None) is not None:
      self.send_response(int(response.get('http_error')))
      self.end_headers()
      self.wfile.write("")
      self._FinalizeAsyncRequest()
    else:
      auth_code = response.get('verification_error', opera_auth.VERIFICATION_ERROR_NO_ERROR)

      if auth_code == opera_auth.VERIFICATION_ERROR_NO_ERROR:
        length = int(self.headers.getheader('content-length'))
        raw_request = self.rfile.read(length)

        http_response, raw_reply = self.server.HandleCommand(self.path,
                                                             raw_request)

        try:
          self.send_response(http_response)
          self.end_headers()
          self.wfile.write(raw_reply)
          self._FinalizeAsyncRequest()
        except Exception as e:
          print "SYNC: Cannot send sync command response, did client disconnect? (%s)" % str(
              e)

        return True
      else:  # auth_code != opera_auth.VERIFICATION_ERROR_NO_ERROR
        print "SYNC: Request served with HTTP 401 and auth code %s" % (
            auth_code)
        try:
          self.send_response(401)
          if auth_code != opera_oauth2_auth.VERIFICATION_ERROR_OAUTH2:
            self.send_header('X-Opera-Auth-Reason', auth_code)
          self.end_headers()
          self.wfile.write("")
          print "SYNC: Client response sent now."
          self._FinalizeAsyncRequest()
        except Exception as e:
          print "SYNC: Cannot send sync command response, did client disconnect? (%s)" % str(
              e)

        return True

  def _MarkThisRequestAsync(self):
    self.sync_http_server_.set_do_close_socket(False)
    self.do_close_files_ = False

  def _FinalizeAsyncRequest(self):
    if self.do_close_files_:
      # _FinalizeAsyncRequest was called for a synchronous request.
      # We ignore this.
      return

    if self.wfile is None or self.rfile is None:
      print "One of fileobjects does not exist! wfile = %s, rfile = %s" % (
          str(self.wfile), str(self.rfile))
      return

    sock = self.wfile._sock

    self.wfile.close()
    self.rfile.close()

    if sock is None:
      print "Fileobject had no socket to shutdown!"
      return

    sock.shutdown(socket.SHUT_WR)
    sock.close()

  def OperaSyncCommandHandler(self):
    """Handle an opera sync command arriving via http.

    This is not much different from ChromiumSyncCommandHandler,
    the authorization is rewritten
    """

    test_name = "/operasync/command"
    if not self._ShouldHandleRequest(test_name):
      return False

    auth_header = self.headers.getheader('Authorization')

    final = {}
    final['origin'] = 'sync'

    client_uses_oauth2 = False
    if auth_header.startswith('Bearer'):
      client_uses_oauth2 = True
      oauth2_token = auth_header.split(' ')[1]
      final['oauth2_token'] = oauth2_token
    else:
      header = auth_header[5:]
      parts = header.split(',')
      for part in parts:
        part = part.strip()
        split_part = part.split('=')
        final[split_part[0]] = split_part[1][1:-1]

    OAUTH2_EXPECTION_MESSAGE = "SYNC: Client is using OAuth2 auth but the "\
                               "local auth server isn't either in the OAuth2 "\
                               " or auth_verification mode."

    if self.server.GetOperaFlags().Get('auth_verification'):
      self._MarkThisRequestAsync()
      handler = partial(self.SendBackOperaSyncCommandResponse)
      self.server.GetOperaAuthServer().HandleAuthHeaderVerificationAsync(
          final, handler)
    elif self.server.GetOperaFlags().Get('real_auth'):
      if client_uses_oauth2:
        raise Exception(OAUTH2_EXPECTION_MESSAGE)

      (username, auth_code) = self.server.GetOperaAuthServer().verify_token(
          final['oauth_token'])
      self.SendBackOperaSyncCommandResponse({'network_error': 0,
                                             'verification_error': auth_code})
    elif self.server.GetOperaFlags().Get('intercept_auth'):
      if client_uses_oauth2:
        raise Exception(OAUTH2_EXPECTION_MESSAGE)

      (username, auth_code) = \
        self.server.GetOperaAuthServer().verify_token(final['oauth_token'], self.client_address_)
      self.SendBackOperaSyncCommandResponse({'network_error': 0,
                                             'verification_error': auth_code})
    elif self.server.GetOperaFlags().Get('true_auth'):
      if client_uses_oauth2:
        raise Exception(OAUTH2_EXPECTION_MESSAGE)

      verification_result = self.server.GetOperaAuthServer().verify_token(
          final, self.client_address_)
      self.SendBackOperaSyncCommandResponse(verification_result)
    elif self.server.GetOperaFlags().Get('oauth2_auth'):
      verification_result = self.server.GetOperaAuthServer().verify_token(
          final, self.client_address_)
      self.SendBackOperaSyncCommandResponse(verification_result)
    else:
      self.SendBackOperaSyncCommandResponse({'network_error': 0,
                                             'verification_error': 0})

  def OnOperaSyncEventAuthorizedOK(self, response):
    # We only care if the event was signed correctly or not.
    if response.get('verification_error', 0) == 0:
      length = int(self.headers.getheader('content-length'))
      raw_request = self.rfile.read(length)
      self.server.HandleEvent(self.path, raw_request)

    self._FinalizeAsyncRequest()

  def OperaSyncEventHandler(self):
    """Handle an opera sync event arriving via http.
    """

    test_name = "/operasync/event"
    if not self._ShouldHandleRequest(test_name):
      return False

    auth_header = self.headers.getheader('Authorization')

    final = {}

    if auth_header.startswith('Bearer'):
      oauth2_token = auth_header.split(' ')[1]
      final['oauth2_token'] = oauth2_token
    else:
      header = auth_header[5:]
      parts = header.split(',')
      for part in parts:
        part = part.strip()
        split_part = part.split('=')
        final[split_part[0]] = split_part[1][1:-1]

    final['origin'] = 'sync'

    if self.server.GetOperaFlags().Get('auth_verification'):
      self._MarkThisRequestAsync()
      handler = partial(self.OnOperaSyncEventAuthorizedOK)
      self.server.GetOperaAuthServer().HandleAuthHeaderVerificationAsync(
          final, handler)

  def OperaRealAuthHandler(self):
    """This handler is responsible for forwarding requests to the opera_real_auth
    class."""
    if not self.server.GetOperaFlags().Get('real_auth'):
      return False

    response = self.server.GetOperaAuthServer().do_handle(
        self._get_request_vars())
    return self._send_response(response)

  def OperaInterceptAuthHandler(self):
    if not self.server.GetOperaFlags().Get('intercept_auth'):
      return False

    response = self.server.GetOperaAuthServer().do_handle(
        self._get_request_vars())
    return self._send_response(response)

  def OperaTrueAuthHandler(self):
    if not self.server.GetOperaFlags().Get('true_auth'):
      return False

    response = self.server.GetOperaAuthServer().handle_request(
        self._get_request_vars())
    return self._send_response(response)

  def SetCommandDelayHandler(self):
    test_name = '/opera/set_command_delay'
    if not self._ShouldHandleRequest(test_name):
      return False

    req_vars = self._get_request_vars()
    new_command_delay = req_vars['get'].get('new_command_delay', None)

    print "MNG: Changing command delay to '{}'".format(new_command_delay)

    if new_command_delay is None:
      response = OperaHTTP.response(
          code=400,
          content='Expected new_command_delay in GET vars.')
    else:
      self.server._current_command_delay = new_command_delay
      response = OperaHTTP.response(code=200)

    return self._send_response(response)

  def OperaOAuth2Handler(self):
    def PathAllowed(path):

      print "OperaOAuth2Handler::PathAllowed " + path

      if path.endswith('/'):
        path = path[:-1]

      allowed_paths = []
      allowed_paths.append('/oauth2/v1/authorize')
      allowed_paths.append('/oauth2/v1/token')
      allowed_paths.append('/oauth2/v1/revoketoken')
      allowed_paths.append('/account/login')
      allowed_paths.append('/oauth2_auth/response_control')
      allowed_paths.append('/oauth2_auth/event_log')
      allowed_paths.append('/oauth2_auth/change_response')
      allowed_paths.append('/oauth2_auth/fetch_event_log')
      allowed_paths.append('/oauth2_auth/reset_event_log')
      allowed_paths.append('/account/access-token/renewal')

      if path in allowed_paths:
        return True
      return False

    if not self.server.GetOperaFlags().Get('oauth2_auth'):
      return False

    req_vars = self._get_request_vars()
    if not PathAllowed(req_vars['path']):
      return False

    response = self.server.GetOperaAuthServer().handle_request(req_vars)
    return self._send_response(response)

  def OperaPanelHandler(self):
    """
    Handles the /opera/panel endpoint.
    Serves the operapanel.html page after substituting the formatters
    found inside.
    """
    test_name = "/opera/panel"
    if not self._ShouldHandleRequest(test_name):
      return False

    current_command_delay = self.server._current_command_delay

    content = open(OperaHTTP.AsDataDirPath('operapanel.html')).read()

    content = content.replace('[OPERA_CMDLINE_PARAMS]',
                              self.server._opera_cmdline)
    content = content.replace('[CURRENT_COMMAND_DELAY]', current_command_delay)

    response = OperaHTTP.response(content=content)
    return self._send_response(response)

  def OperaXMPPSessionsHandler(self):
    """
    Handles the /opera/xmppsessions endpoint.
    Serves the operaxmppsessions.html page after substituting the formatters
    found inside.
    """
    test_name = "/opera/xmppsessions"
    if not self._ShouldHandleRequest(test_name):
      return False

    row_string = "%s <button onclick=\"close_session('%s')\">Close</button><br>"

    row_strings = ""
    row_string_subs = ""

    for conn in self.server._xmpp_server._connections:
      row_string_subs = row_string % (conn.get_desc(), conn.get_id())
      row_strings += row_string_subs

    content = open(OperaHTTP.AsDataDirPath('operaxmppsessions.html'), 'r').read()
    content = content.replace('[XMPP_SESSION_ROWS]', row_strings)

    response = OperaHTTP.response(content=content)
    return self._send_response(response)

  def OperaXMPPSessionCloseHandler(self):
    """
    Handles the /opera/xmppsessionclose endpoint.
    Closes the session basing on the session_id parameter given in
    the GET variables.
    """
    test_name = '/opera/xmppsessionclose'
    if not self._ShouldHandleRequest(test_name):
      return False

    req_vars = self._get_request_vars()
    session_id = req_vars['get']['session_id']

    response_dict = OperaHTTP.response(code=401,
                                       content='XMPP session not found')

    to_close = None

    for conn in self.server._xmpp_server._connections:
      if conn.get_id() == session_id:
        to_close = conn
        response_dict = OperaHTTP.response(
            code=200,
            content="XMPP session '{}' closed OK".format(session_id))
        break

    if to_close is not None:
      print "MNG: Closing session '%s'" % session_id
      to_close.close()

    return self._send_response(response_dict)

  def _send_response(self, response):
    if not response.get('handled', False):
      return False

    http_code = response['code']
    headers = response.get('headers', {})
    content_type = response.get('type', 'text/html')
    content = response.get('content', '')
    content_len = len(content)
    network_error = response.get('network_error', False)

    if network_error:
      self._FinalizeAsyncRequest()
      return

    self.send_response(http_code)
    self.send_header('Content-Type', content_type)
    self.send_header('Content-Length', content_len)
    for name in headers:
      value = headers[name]
      if (type(value) is list):
        for element in value:
          self.send_header(name, element)
      else:
        self.send_header(name, value)
    self.end_headers()
    self.wfile.write(content)

    return True

  def _get_request_vars(self):
    """Assumptions:
    1. We can only read the POST data once per request;
    2. A single self is serving a single request."""

    def deduplicate(raw):
      ready = {}
      for k in raw:
        v = raw[k]
        if isinstance(v, list):
          v = v[0]
        ready[k] = v
      return ready

    if getattr(self, '_cached_request_vars', None) is not None:
      return self._cached_request_vars

    parsed_path = urlparse.urlparse(self.path)

    request_vars = {}
    # GET vars:
    get_vars = {}
    query = parsed_path.query
    query_params = urlparse.parse_qs(query)
    # Make it simple, skip any repeated GET values
    get_vars = deduplicate(query_params)

    # POST_VARS
    post_vars = {}
    if self.headers.getheader(
        'content-type') == "application/x-www-form-urlencoded":
      length = int(self.headers.getheader('content-length'))
      post_vars = urlparse.parse_qs(
          self.rfile.read(length),
          keep_blank_values=1)

    # Make it simple, skip any repeated POST values
    post_vars = deduplicate(post_vars)

    # HEADERS
    request_headers = {}
    for h in self.headers:
      request_headers[h] = self.headers[h]

    request_vars['get'] = get_vars
    request_vars['post'] = post_vars
    request_vars['headers'] = request_headers
    request_vars['path'] = parsed_path.path

    self._cached_request_vars = request_vars
    return request_vars


class SyncServerRunner(testserver_base.TestServerRunner):
  """TestServerRunner for the net test servers."""

  def __init__(self):
    super(SyncServerRunner, self).__init__()

  def create_server(self, server_data):
    port = self.options.port
    host = self.options.host
    xmpp_port = self.options.xmpp_port
    server = SyncHTTPServer((host, port), xmpp_port, SyncPageHandler)

    guessed_ip = host
    # The original implementation of this server did only work for 127.0.0.1.
    # Since we have introduced the --bind-all parameter, we don't want to show
    # loopback address however, since that has no meaning to the user.
    if host == "0.0.0.0":
      try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('8.8.8.8', 80))
        guessed_ip = s.getsockname()[0]
        s.close()
      except:
        print "Could not guess your IP"

    print('Sync HTTP server started at %s:%d/operasync...' %
          (host, server.server_port))
    print('Fake Opera OAuth token server started at %s:%d...' %
          (host, server.server_port))
    print('Sync XMPP server started at %s:%d...' % (host, server.xmpp_port))

    sync_url = "%s:%d/operasync" % (guessed_ip, server.server_port)
    auth_url = "%s:%d" % (guessed_ip, server.server_port)
    accounts_url = "%s:%d" % (guessed_ip, server.server_port)
    xmpp_addr = "%s:%d" % (guessed_ip, server.xmpp_port)

    full_cmdline = '--sync-url=http://%s ' \
                   '--sync-notification-host-port=%s ' \
                   '--sync-allow-insecure-xmpp-connection ' \
                   '--sync-auth-url=http://%s ' \
                   '--sync-accounts-url=http://%s ' \
                   '--sync-allow-insecure-accounts-connection ' \
                   '--sync-allow-insecure-auth-connection' % \
                   (sync_url, xmpp_addr, auth_url, accounts_url)
    print('Use the following client command line for full test server support:')
    print(full_cmdline)
    server.set_opera_cmdline(full_cmdline)

    server_data['port'] = server.server_port
    server_data['xmpp_port'] = server.xmpp_port
    return server

  def run_server(self):
    testserver_base.TestServerRunner.run_server(self)

  def add_options(self):
    testserver_base.TestServerRunner.add_options(self)
    self.option_parser.add_option('--xmpp-port',
                                  default='0',
                                  type='int',
                                  help='Port used by the XMPP server. If '
                                  'unspecified, the XMPP server will listen on '
                                  'an ephemeral port.')
    # Override the default logfile name used in testserver.py.
    self.option_parser.set_defaults(log_file='sync_testserver.log')


if __name__ == '__main__':
  sys.exit(SyncServerRunner().main())
