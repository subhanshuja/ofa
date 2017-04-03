# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A bare-bones and non-compliant XMPP server.

Just enough of the protocol is implemented to get it to work with
Chrome's sync notification system.
"""

import asynchat
import asyncore
import base64
import re
import socket
from xml.dom import minidom

import binascii
import client_gateway_pb2
import client_protocol_pb2
import os
import types_pb2
import time

import opera_auth
from functools import partial

# pychecker complains about the use of fileno(), which is implemented
# by asyncore by forwarding to an internal object via __getattr__.
__pychecker__ = 'no-classattr'


class Error(Exception):
  """Error class for this module."""
  pass


class UnexpectedXml(Error):
  """Raised when an unexpected XML element has been encountered."""

  def __init__(self, xml_element):
    xml_text = xml_element.toxml()
    Error.__init__(self, 'Unexpected XML element', xml_text)


def ParseXml(xml_string):
  """Parses the given string as XML and returns a minidom element
  object.
  """
  dom = minidom.parseString(xml_string)

  # minidom handles xmlns specially, but there's a bug where it sets
  # the attribute value to None, which causes toxml() or toprettyxml()
  # to break.
  def FixMinidomXmlnsBug(xml_element):
    if xml_element.getAttribute('xmlns') is None:
      xml_element.setAttribute('xmlns', '')

  def ApplyToAllDescendantElements(xml_element, fn):
    fn(xml_element)
    for node in xml_element.childNodes:
      if node.nodeType == node.ELEMENT_NODE:
        ApplyToAllDescendantElements(node, fn)

  root = dom.documentElement
  ApplyToAllDescendantElements(root, FixMinidomXmlnsBug)
  return root


def CloneXml(xml):
  """Returns a deep copy of the given XML element.

  Args:
    xml: The XML element, which should be something returned from
         ParseXml() (i.e., a root element).
  """
  return xml.ownerDocument.cloneNode(True).documentElement


class StanzaParser(object):
  """A hacky incremental XML parser.

  StanzaParser consumes data incrementally via FeedString() and feeds
  its delegate complete parsed stanzas (i.e., XML documents) via
  FeedStanza().  Any stanzas passed to FeedStanza() are unlinked after
  the callback is done.

  Use like so:

  class MyClass(object):
    ...
    def __init__(self, ...):
      ...
      self._parser = StanzaParser(self)
      ...

    def SomeFunction(self, ...):
      ...
      self._parser.FeedString(some_data)
      ...

    def FeedStanza(self, stanza):
      ...
      print stanza.toprettyxml()
      ...
  """

  # NOTE(akalin): The following regexps are naive, but necessary since
  # none of the existing Python 2.4/2.5 XML libraries support
  # incremental parsing.  This works well enough for our purposes.
  #
  # The regexps below assume that any present XML element starts at
  # the beginning of the string, but there may be trailing whitespace.

  # Matches an opening stream tag (e.g., '<stream:stream foo="bar">')
  # (assumes that the stream XML namespace is defined in the tag).
  _stream_re = re.compile(r'^(<stream:stream [^>]*>)\s*')

  # Matches an empty element tag (e.g., '<foo bar="baz"/>').
  _empty_element_re = re.compile(r'^(<[^>]*/>)\s*')

  # Matches a non-empty element (e.g., '<foo bar="baz">quux</foo>').
  # Does *not* handle nested elements.
  _non_empty_element_re = re.compile(r'^(<([^ >]*)[^>]*>.*?</\2>)\s*')

  # The closing tag for a stream tag.  We have to insert this
  # ourselves since all XML stanzas are children of the stream tag,
  # which is never closed until the connection is closed.
  _stream_suffix = '</stream:stream>'

  def __init__(self, delegate):
    self._buffer = ''
    self._delegate = delegate

  def FeedString(self, data):
    """Consumes the given string data, possibly feeding one or more
    stanzas to the delegate.
    """
    self._buffer += data
    while (self._ProcessBuffer(self._stream_re, self._stream_suffix) or
           self._ProcessBuffer(self._empty_element_re) or
           self._ProcessBuffer(self._non_empty_element_re)):
      pass

  def _ProcessBuffer(self, regexp, xml_suffix=''):
    """If the buffer matches the given regexp, removes the match from
    the buffer, appends the given suffix, parses it, and feeds it to
    the delegate.

    Returns:
      Whether or not the buffer matched the given regexp.
    """
    results = regexp.match(self._buffer)
    if not results:
      return False
    xml_text = self._buffer[:results.end()] + xml_suffix
    self._buffer = self._buffer[results.end():]
    stanza = ParseXml(xml_text)
    self._delegate.FeedStanza(stanza)
    # Needed because stanza may have cycles.
    stanza.unlink()
    return True


class Jid(object):
  """Simple struct for an XMPP jid (essentially an e-mail address with
  an optional resource string).
  """

  def __init__(self, username, domain, resource=''):
    self.username = username
    self.domain = domain
    self.resource = resource

  def __str__(self):
    jid_str = "%s@%s" % (self.username, self.domain)
    if self.resource:
      jid_str += '/' + self.resource
    return jid_str

  def GetBareJid(self):
    return Jid(self.username, self.domain)


class IdGenerator(object):
  """Simple class to generate unique IDs for XMPP messages."""

  def __init__(self, prefix):
    self._prefix = prefix
    self._id = 0

  def GetNextId(self):
    next_id = "%s.%s" % (self._prefix, self._id)
    self._id += 1
    return next_id


class HandshakeTask(object):
  """Class to handle the initial handshake with a connected XMPP
  client.
  """

  # The handshake states in order.
  (_INITIAL_STREAM_NEEDED, _AUTH_NEEDED, _AUTH_STREAM_NEEDED, _BIND_NEEDED,
   _SESSION_NEEDED, _FINISHED) = range(6)

  # Used when in the _INITIAL_STREAM_NEEDED and _AUTH_STREAM_NEEDED
  # states.  Not an XML object as it's only the opening tag.
  #
  # The from and id attributes are filled in later.
  _STREAM_DATA = (
      '<stream:stream from="%s" id="%s" '
      'version="1.0" xmlns:stream="http://etherx.jabber.org/streams" '
      'xmlns="jabber:client">')

  # Used when in the _INITIAL_STREAM_NEEDED state.
  _AUTH_STANZA = ParseXml(
      '<stream:features xmlns:stream="http://etherx.jabber.org/streams">'
      '  <mechanisms xmlns="urn:ietf:params:xml:ns:xmpp-sasl">'
      '    <mechanism>PLAIN</mechanism>'
      '    <mechanism>X-GOOGLE-TOKEN</mechanism>'
      '    <mechanism>X-OAUTH2</mechanism>'
      '  </mechanisms>'
      '</stream:features>')

  # Used when in the _INITIAL_STREAM_NEEDED state.
  # This is the Opera version.
  _OPERA_AUTH_STANZA = ParseXml(
      '<stream:features xmlns:stream="http://etherx.jabber.org/streams">'
      '  <starttls xmlns="urn:ietf:params:xml:ns:xmpp-tls">'
      '  </starttls>'
      '  <mechanisms xmlns="urn:ietf:params:xml:ns:xmpp-sasl">'
      '    <mechanism>PLAIN</mechanism>'
      '    <mechanism>X-OPERA-TOKEN</mechanism>'
      '    <mechanism>X-OAUTH2</mechanism>'
      '  </mechanisms>'
      '</stream:features>')

  # Used when in the _AUTH_NEEDED state.
  _AUTH_SUCCESS_STANZA = ParseXml(
      '<success xmlns="urn:ietf:params:xml:ns:xmpp-sasl"/>')

  # Used when in the _AUTH_NEEDED state.
  _AUTH_FAILURE_STANZA = ParseXml(
      '<failure xmlns="urn:ietf:params:xml:ns:xmpp-sasl"/>')

  _AUTH_TEMP_FAILURE_STANZA = ParseXml(
      '<failure xmlns="urn:ietf:params:xml:ns:xmpp-sasl">'
      '  <temporary-auth-failure/>'
      '</failure>')

  _AUTH_NOT_AUTHORIZED_STANZA = '<failure xmlns="urn:ietf:params:xml:ns:xmpp-sasl"><not-authorized>%s</not-authorized></failure>'

  # Used when in the _AUTH_STREAM_NEEDED state.
  _BIND_STANZA = ParseXml(
      '<stream:features xmlns:stream="http://etherx.jabber.org/streams">'
      '  <bind xmlns="urn:ietf:params:xml:ns:xmpp-bind"/>'
      '  <session xmlns="urn:ietf:params:xml:ns:xmpp-session"/>'
      '</stream:features>')

  # Used when in the _BIND_NEEDED state.
  #
  # The id and jid attributes are filled in later.
  _BIND_RESULT_STANZA = ParseXml(
      '<iq id="" type="result">'
      '  <bind xmlns="urn:ietf:params:xml:ns:xmpp-bind">'
      '    <jid/>'
      '  </bind>'
      '</iq>')

  # Used when in the _SESSION_NEEDED state.
  #
  # The id attribute is filled in later.
  _IQ_RESPONSE_STANZA = ParseXml('<iq id="" type="result"/>')

  def __init__(self, connection, resource_prefix, authenticated, auth_error,
               opera_auth_server, opera_flags):
    self._connection = connection
    self._id_generator = IdGenerator(resource_prefix)
    self._username = ''
    self._domain = ''
    self._jid = None
    self._authenticated = authenticated
    self._auth_error = auth_error
    self._resource_prefix = resource_prefix
    self._state = self._INITIAL_STREAM_NEEDED
    self._opera_auth_server = opera_auth_server
    self._opera_flags = opera_flags

    # The login task now checks the supplied auth header against the auth
    # server.
    # The FeedStanza() method is called twice for the _AUTH_NEEDED state,
    # once with this flag set to False, then set to True, once the
    # verification response is available.
    self._async_verification_available = False

  # This method is called asynchronously once the opera_auth instance prepares an
  # authorization header verification response.
  #
  # params:
  # @stanza - the original XML stanza that the FeedStanza() method was called
  #           already.
  # @response - a dictionary containing the authorization header verification
  #             response.
  #           Keys:
  #             network_error: 0 (default), 1 or 2. In case this value is set to 1
  #               the XMPP connection is closed without any response. Any other
  #               keys are ignored in such a case. In case the value is 2, the
  #               verification result reported to client is transient error.
  #             verification_error: int value designating the verification problem
  #               code (compare 1.1. At the authorization header verification endpoint)
  #               found in the opera_auth code. Only taken into the account in case
  #               network error is 0.
  #               Note that no error is designated by setting the value assigned to this
  #               key to 0.
  #             jid: the JID returned as a result of auth header verification. Only taken
  #               into the account in case the verification_error is set to 0.
  #
  # It is assumed that the supplied response conforms to the above description.
  def OnAuthHeaderVerificationResponseAvailable(self, stanza, response):
    self._async_verification_available = True
    if int(response.get('network_error', 0)) == 1:
      self._connection.close()
    elif int(response.get('network_error', 0)) == 2:
      self._auth_error = opera_auth.AUTH_ERROR_TEMPORARY
    else:
      self._auth_error = opera_auth.OperaComErrCodeToLocalAuthError(
          response.get('verification_error', 0))
      if self._auth_error == opera_auth.AUTH_ERROR_OK:
        jid = response['jid']
        (self._username, self._domain) = jid.split('@', 2)

      self.FeedStanza(stanza)

  def FeedStanza(self, stanza):
    """Inspects the given stanza and changes the handshake state if needed.

    Called when a stanza is received from the client.  Inspects the
    stanza to make sure it has the expected attributes given the
    current state, advances the state if needed, and sends a reply to
    the client if needed.
    """

    def ExpectStanza(stanza, name):
      if stanza.tagName != name:
        raise UnexpectedXml(stanza)

    def ExpectIq(stanza, type, name):
      ExpectStanza(stanza, 'iq')
      if (stanza.getAttribute('type') != type or
          stanza.firstChild.tagName != name):
        raise UnexpectedXml(stanza)

    def GetStanzaId(stanza):
      return stanza.getAttribute('id')

    def HandleStream(stanza):
      ExpectStanza(stanza, 'stream:stream')
      domain = stanza.getAttribute('to')
      if domain:
        self._domain = domain
      SendStreamData()

    def SendStreamData():
      next_id = self._id_generator.GetNextId()
      stream_data = self._STREAM_DATA % (self._domain, next_id)
      self._connection.SendData(stream_data)

    def GetUserDomain(stanza):
      encoded_username_password = stanza.firstChild.data
      username_password = base64.b64decode(encoded_username_password)
      (_, username_domain, _) = username_password.split('\0')
      # The domain may be omitted.
      #
      # If we were using python 2.5, we'd be able to do:
      #
      #   username, _, domain = username_domain.partition('@')
      #   if not domain:
      #     domain = self._domain
      at_pos = username_domain.find('@')
      if at_pos != -1:
        username = username_domain[:at_pos]
        domain = username_domain[at_pos + 1:]
      else:
        username = username_domain
        domain = self._domain
      return (username, domain)

    def Base64EncodedAuthHeaderToDict(header):
      header = base64.b64decode(header)
      final = {}
      if self._opera_flags.Get('oauth2_auth'):
        final['oauth2_token'] = header
      else:
        header = header[5:]
        parts = header.split(',')
        for part in parts:
          part = part.strip()
          split_part = part.split('=')
          final[split_part[0]] = split_part[1][1:-1]

      final['origin'] = 'xmpp'
      return final

    def GetOperaUserDomainAsync(stanza):
      final = Base64EncodedAuthHeaderToDict(stanza.firstChild.data)

      handler = partial(self.OnAuthHeaderVerificationResponseAvailable, stanza)
      self._opera_auth_server.HandleAuthHeaderVerificationAsync(final, handler)

    def Finish():
      self._state = self._FINISHED
      self._connection.HandshakeDone(self._jid)

    if self._state == self._INITIAL_STREAM_NEEDED:
      HandleStream(stanza)
      if self._connection._delegate._opera_invalidations_enabled and not self._opera_flags.Get('oauth2_auth'):
        self._connection.SendStanza(self._OPERA_AUTH_STANZA, False)
      else:
        self._connection.SendStanza(self._AUTH_STANZA, False)
      self._state = self._AUTH_NEEDED

    elif self._state == self._AUTH_NEEDED:
      ExpectStanza(stanza, 'auth')

      if not self._opera_flags.Get('true_auth'):
        if not self._opera_flags.Get('auth_verification'):
          if not self._opera_flags.Get('oauth2_auth'):
            self._async_verification_available = True
            self._auth_error = opera_auth.AUTH_ERROR_OK

      if not self._async_verification_available:
        if self._opera_flags.Get('true_auth'):
          auth_header_dict = Base64EncodedAuthHeaderToDict(
              stanza.firstChild.data)
          response = self._opera_auth_server.verify_xmpp_connect_attempt(
              auth_header_dict, self._resource_prefix)
          self.OnAuthHeaderVerificationResponseAvailable(stanza, response)
        elif self._opera_flags.Get('oauth2_auth'):
          auth_header_dict = Base64EncodedAuthHeaderToDict(
              stanza.firstChild.data)
          response = self._opera_auth_server.verify_token(
              auth_header_dict, self._resource_prefix)
          self.OnAuthHeaderVerificationResponseAvailable(stanza, response)
        else:
          # auth_verification
          GetOperaUserDomainAsync(stanza)
      else:
        if self._auth_error != opera_auth.AUTH_ERROR_OK:
          if self._auth_error == opera_auth.AUTH_ERROR_TEMPORARY:
            print "XMPP: Refusing login with a temporary error"
            self._connection.SendStanza(self._AUTH_TEMP_FAILURE_STANZA, False)
            Finish()
          else:
            errcode = opera_auth.LocalAuthErrorToAuthOperaComErrCode(
                self._auth_error)
            print "XMPP: Sending back auth error %s" % (errcode)
            stanza = ParseXml(self._AUTH_NOT_AUTHORIZED_STANZA % errcode)
            self._connection.SendStanza(stanza, False)
          Finish()
        else:
          if self._authenticated:
            self._connection.SendStanza(self._AUTH_SUCCESS_STANZA, False)
            self._state = self._AUTH_STREAM_NEEDED
          else:
            self._connection.SendStanza(self._AUTH_FAILURE_STANZA, False)
            Finish()

    elif self._state == self._AUTH_STREAM_NEEDED:
      HandleStream(stanza)
      self._connection.SendStanza(self._BIND_STANZA, False)
      self._state = self._BIND_NEEDED

    elif self._state == self._BIND_NEEDED:
      ExpectIq(stanza, 'set', 'bind')
      stanza_id = GetStanzaId(stanza)
      resource_element = stanza.getElementsByTagName('resource')[0]
      resource = resource_element.firstChild.data
      full_resource = '%s.%s' % (self._resource_prefix, resource)
      response = CloneXml(self._BIND_RESULT_STANZA)
      response.setAttribute('id', stanza_id)
      self._jid = Jid(self._username, self._domain, full_resource)
      jid_text = response.parentNode.createTextNode(str(self._jid))
      response.getElementsByTagName('jid')[0].appendChild(jid_text)
      self._connection.SendStanza(response)
      self._state = self._SESSION_NEEDED

    elif self._state == self._SESSION_NEEDED:
      ExpectIq(stanza, 'set', 'session')
      stanza_id = GetStanzaId(stanza)
      xml = CloneXml(self._IQ_RESPONSE_STANZA)
      xml.setAttribute('id', stanza_id)
      self._connection.SendStanza(xml)
      Finish()


def AddrString(addr):
  return '%s:%d' % addr


class XmppConnection(asynchat.async_chat):
  """A single XMPP client connection.

  This class handles the connection to a single XMPP client (via a
  socket).  It does the XMPP handshake and also implements the (old)
  Google notification protocol.
  """

  # Used for acknowledgements to the client.
  #
  # The from and id attributes are filled in later.
  _IQ_RESPONSE_STANZA = ParseXml('<iq from="" id="" type="result"/>')

  def __init__(self, sock, socket_map, delegate, addr, authenticated,
               auth_error, opera_auth_server, opera_flags, id):
    """Starts up the xmpp connection.

    Args:
      sock: The socket to the client.
      socket_map: A map from sockets to their owning objects.
      delegate: The delegate, which is notified when the XMPP
        handshake is successful, when the connection is closed, and
        when a notification has to be broadcast.
      addr: The host/port of the client.
    """
    # We do this because in versions of python < 2.6,
    # async_chat.__init__ doesn't take a map argument nor pass it to
    # dispatcher.__init__.  We rely on the fact that
    # async_chat.__init__ calls dispatcher.__init__ as the last thing
    # it does, and that calling dispatcher.__init__ with socket=None
    # and map=None is essentially a no-op.
    asynchat.async_chat.__init__(self)
    asyncore.dispatcher.__init__(self, sock, socket_map)

    self.set_terminator(None)

    self._delegate = delegate
    self._parser = StanzaParser(self)
    self._jid = None

    self._token = 'testtoken'

    self._opera_flags = opera_flags
    self._id = id

    self._addr = addr
    addr_str = AddrString(self._addr)
    self._handshake_task = HandshakeTask(self, addr_str, authenticated, \
                                         auth_error, opera_auth_server, opera_flags)
    print 'XMPP: Starting connection to %s' % self

  def __str__(self):
    if self._jid:
      return str(self._jid)
    else:
      return AddrString(self._addr)

  def get_jid(self):
    """
    Returns the Jabber ID for the current session. The Jabber ID resembles an
    e-mail format (i.e. user@server).
    """
    if self._jid:
      return self._jid
    return "JID_UNKNOWN"

  def get_addr(self):
    return AddrString(self._addr)

  def get_id(self):
    return self._id

  def get_desc(self):
    return "[%s] %s %s" % (self.get_id(), self.get_addr(), self.get_jid())

  # async_chat implementation.

  def set_token(self, token):
    self._token = token

  def get_token(self):
    return self._token

  def generate_token(self):
    #new_token = binascii.b2a_hex(os.urandom(15))
    new_token = 'testtoken'
    self.set_token(new_token)
    return new_token

  def collect_incoming_data(self, data):
    self._parser.FeedString(data)

  # This is only here to make pychecker happy.
  def found_terminator(self):
    asynchat.async_chat.found_terminator(self)

  def close(self):
    print "XMPP: Closing connection to %s" % self
    self._delegate.OnXmppConnectionClosed(self)
    asynchat.async_chat.close(self)

  # Called by self._parser.FeedString().
  def FeedStanza(self, stanza):
    if self._handshake_task:
      self._handshake_task.FeedStanza(stanza)
    elif stanza.tagName == 'iq' and stanza.getAttribute('type') == 'result':
      # Ignore all client acks.
      pass
    elif (stanza.firstChild and
          stanza.firstChild.namespaceURI == 'google:push'):
      self._HandlePushCommand(stanza)
    else:
      raise UnexpectedXml(stanza)

  def PostAuthVerification(self, action_data):
    if action_data.get('do_disconnect', False) is True:
      print "XMMP: Performing forced disconnection!"
      self.close()

  # Called by self._handshake_task.
  def HandshakeDone(self, jid):
    if jid:
      self._jid = jid
      self._handshake_task = None
      self._delegate.OnXmppHandshakeDone(self)
      print "XMPP: Handshake done for %s" % self
    else:
      print "XMPP: Handshake failed for %s" % self
      self.close()
    if self._opera_flags.Get('auth_verification'):
      self._delegate._opera_auth_server.HandleXMPPFinished(
          self.PostAuthVerification)

  def _HandlePushCommand(self, stanza):
    if stanza.tagName == 'iq' and stanza.firstChild.tagName == 'subscribe':
      # Subscription request.
      self._SendIqResponseStanza(stanza)
    elif stanza.tagName == 'message' and stanza.firstChild.tagName == 'push':
      # Send notification request.
      if self._delegate._opera_invalidations_enabled:
        self._delegate.HandlePushMessage(self, stanza)
      else:
        self._delegate.ForwardNotification(self, stanza)
    else:
      raise UnexpectedXml(command_xml)

  def _SendIqResponseStanza(self, iq):
    stanza = CloneXml(self._IQ_RESPONSE_STANZA)
    stanza.setAttribute('from', str(self._jid.GetBareJid()))
    stanza.setAttribute('id', iq.getAttribute('id'))
    self.SendStanza(stanza)

  def SendStanza(self, stanza, unlink=True):
    """Sends a stanza to the client.

    Args:
      stanza: The stanza to send.
      unlink: Whether to unlink stanza after sending it. (Pass in
      False if stanza is a constant.)
    """
    self.SendData(stanza.toxml())
    if unlink:
      stanza.unlink()

  def SendData(self, data):
    """Sends raw data to the client.
    """
    # We explicitly encode to ascii as that is what the client expects
    # (some minidom library functions return unicode strings).
    self.push(data.encode('ascii'))

  def ForwardNotification(self, notification_stanza):
    """Forwards a notification to the client."""
    notification_stanza.setAttribute('from', str(self._jid.GetBareJid()))
    notification_stanza.setAttribute('to', str(self._jid))
    self.SendStanza(notification_stanza, False)


class XmppServer(asyncore.dispatcher):
  """The main XMPP server class.

  The XMPP server starts accepting connections on the given address
  and spawns off XmppConnection objects for each one.

  Use like so:

    socket_map = {}
    xmpp_server = xmppserver.XmppServer(socket_map, ('127.0.0.1', 5222))
    asyncore.loop(30.0, False, socket_map)
  """

  # Used when sending a notification.
  _NOTIFICATION_STANZA = ParseXml('<message>'
                                  '  <push xmlns="google:push">'
                                  '    <data/>'
                                  '  </push>'
                                  '</message>')

  def __init__(self, socket_map, addr, opera_flags):
    asyncore.dispatcher.__init__(self, None, socket_map)
    self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
    self.set_reuse_addr()
    self.bind(addr)
    self.listen(5)
    self._socket_map = socket_map
    self._connections = set()
    self._handshake_done_connections = set()
    self._auth_error = opera_auth.AUTH_ERROR_OK
    self._notifications_enabled = True
    self._authenticated = True
    self._opera_invalidations_enabled = True
    self._opera_delayed_invalidations = False
    self._opera_auth_server = None
    self._opera_flags = opera_flags
    self._last_invalidation = None
    self._invalidation_timer = None
    self._connection_id_generator = IdGenerator('xmpp_session')

  def handle_accept(self):
    (sock, addr) = self.accept()
    xmpp_connection = XmppConnection( \
      sock, self._socket_map, self, addr, self._authenticated, self._auth_error, self._opera_auth_server, self._opera_flags, self._connection_id_generator.GetNextId())
    self._connections.add(xmpp_connection)

    print "XMPP: Connect from %s:%s" % (addr[0], addr[1])

    if self._opera_flags.Get('true_auth') or self._opera_flags.Get('oauth2_auth'):
      self._opera_auth_server.on_xmpp_connected(addr)

    # Return the new XmppConnection for testing.
    return xmpp_connection

  def close(self):
    # A copy is necessary since calling close on each connection
    # removes it from self._connections.
    for connection in self._connections.copy():
      connection.close()
    asyncore.dispatcher.close(self)

  def SetOperaAuthServer(self, server):
    self._opera_auth_server = server

  def SetAuthError(self, auth_error):
    self._auth_error = auth_error

  def EnableNotifications(self):
    self._notifications_enabled = True

  def DisableNotifications(self):
    self._notifications_enabled = False

  def EnableOperaInvalidations(self):
    print "XMPP: Opera notifications enabled"
    self._notifications_enabled = False
    self._opera_invalidations_enabled = True

  def EnableOperaDelayedInvalidations(self):
    print "XMPP: Opera delayed invalidations enabled"
    self._opera_delayed_invalidations = True

  def MakeNotification(self, channel, data):
    """Makes a notification from the given channel and encoded data.

    Args:
      channel: The channel on which to send the notification.
      data: The notification payload.
    """
    notification_stanza = CloneXml(self._NOTIFICATION_STANZA)
    push_element = notification_stanza.getElementsByTagName('push')[0]
    push_element.setAttribute('channel', channel)
    data_element = push_element.getElementsByTagName('data')[0]
    encoded_data = base64.b64encode(data)
    data_text = notification_stanza.parentNode.createTextNode(encoded_data)
    data_element.appendChild(data_text)
    return notification_stanza

  def SendNotification(self, channel, data):
    """Sends a notification to all connections.

    Args:
      channel: The channel on which to send the notification.
      data: The notification payload.
    """
    notification_stanza = self.MakeNotification(channel, data)
    self.ForwardNotification(None, notification_stanza)
    notification_stanza.unlink()

  def SetAuthenticated(self, auth_valid):
    self._authenticated = auth_valid

    # We check authentication only when establishing new connections.  We close
    # all existing connections here to make sure previously connected clients
    # pick up on the change.  It's a hack, but it works well enough for our
    # purposes.
    if not self._authenticated:
      for connection in self._handshake_done_connections:
        connection.close()

  def GetAuthenticated(self):
    return self._authenticated

  # XmppConnection delegate methods.
  def OnXmppHandshakeDone(self, xmpp_connection):
    print "XMPP: Handshake done"
    self._handshake_done_connections.add(xmpp_connection)

  def OnXmppConnectionClosed(self, xmpp_connection):
    if self._opera_flags.Get('true_auth') or self._opera_flags.Get('oauth2_auth'):
      self._opera_auth_server.on_xmpp_disconnected(xmpp_connection._addr)

    self._connections.discard(xmpp_connection)
    self._handshake_done_connections.discard(xmpp_connection)

  def HandlePushMessage(self, xmpp_connection, notification_stanza):
    data = notification_stanza.getElementsByTagName('data')[0].childNodes[
        0].data
    decoded_data = base64.b64decode(data)
    gateway_msg = client_gateway_pb2.ClientGatewayMessage()
    gateway_msg.ParseFromString(decoded_data)

    if gateway_msg.is_client_to_server:
      c2s_msg = client_protocol_pb2.ClientToServerMessage()
      c2s_msg.ParseFromString(gateway_msg.network_message)
      #print c2s_msg.HasField('initialize_message'), c2s_msg.HasField('registration_message'), c2s_msg.HasField('registration_sync_message'), c2s_msg.HasField('invalidation_ack_message'), c2s_msg.HasField('info_message')
      if c2s_msg.HasField('initialize_message'):
        self.HandleInitializeMessage(c2s_msg, xmpp_connection)
        return
      if c2s_msg.HasField('registration_message'):
        self.HandleRegistrationMessage(c2s_msg, xmpp_connection)
        return
      if c2s_msg.HasField('info_message'):
        self.HandleInfoMessage(c2s_msg, xmpp_connection)
        return

    self.ForwardNotification(xmpp_connection, notification_stanza)

  def ForwardNotification(self, unused_xmpp_connection, notification_stanza):
    if self._notifications_enabled:
      for connection in self._handshake_done_connections:
        print 'Sending notification to %s' % connection
        connection.ForwardNotification(notification_stanza)
    else:
      print 'Notifications disabled; dropping notification'

  def HandleInitializeMessage(self, c2s_msg, xmpp_connection):
    """Forwards a notification to the client."""
    # Respond with token control message

    # Nonce is used to get token, remember and use as token when sending reponse
    nonce = c2s_msg.initialize_message.nonce

    print "XMPP: initialize_message(%s) <- %s" % (nonce, xmpp_connection)

    response_msg = client_protocol_pb2.ServerToClientMessage()
    self.MakeHeaderWithToken(response_msg, nonce)

    # We should generate and remember a token per connection, use it in
    # SendInvalidation().
    # However, the client does not seem to be able to refresh the token
    # in case the one saved in the persistent state across browser restarts
    # mismatches the one sent by the server.
    # Keeping the token constant across all clients and all connections for
    # the time being.
    # It seems actually, that the client token can be changed only after
    # the new token control message passes the token validation against
    # the token stored in the persistent state.
    new_token = xmpp_connection.generate_token()
    response_msg.token_control_message.new_token = new_token

    gateway_msg = client_gateway_pb2.ClientGatewayMessage()
    gateway_msg.is_client_to_server = False
    gateway_msg.network_message = response_msg.SerializeToString()

    notification_stanza = self.MakeNotification("tango_raw",
                                                gateway_msg.SerializeToString())

    print 'XMPP: token(%s) -> %s' % (new_token, xmpp_connection)
    #    xmpp_connection.set_token(new_token)
    xmpp_connection.ForwardNotification(notification_stanza)

    if self._last_invalidation is not None:
      print "XMPP: Sending initial invalidation"
      (last_type, last_version) = self._last_invalidation
      self.RequestSendInvalidation(last_type, last_version)

    if self._opera_flags.Get('auth_verification'):
      self._opera_auth_server.HandleXMPPInitialized(None)

  def HandleRegistrationMessage(self, c2s_msg, xmpp_connection):
    print 'XMPP: unhandled registration_message <- %s' % (xmpp_connection)

  def HandleInfoMessage(self, c2s_msg, xmpp_connection):
    info_message = c2s_msg.info_message

    if info_message.server_registration_summary_requested:
      # TODO: Send back registration summary in the server header.
      print 'XMPP: unhandled server_registration_summary_requested <- %s' % (
          xmpp_connection)
      return

    print 'XMPP: unhandled info_message <- %s' % (xmpp_connection)

  def RequestSendInvalidation(self, invalidation_type, invalidation_version):
    if self._opera_delayed_invalidations:
      import threading
      if self._invalidation_timer is not None:
        self._invalidation_timer.cancel()

      do_send = partial(self.DoSendInvalidation, invalidation_type,
                        invalidation_version)
      self._invalidation_timer = threading.Timer(1, do_send)
      self._invalidation_timer.start()
    else:
      self.DoSendInvalidation(invalidation_type, invalidation_version)

  def DoSendInvalidation(self, invalidation_type, invalidation_version):
    if not self._opera_invalidations_enabled:
      return

    self._last_invalidation = (invalidation_type, invalidation_version)

    for connection in self._handshake_done_connections:
      s2c_msg = client_protocol_pb2.ServerToClientMessage()
      self.MakeHeaderWithToken(s2c_msg, connection.get_token())

      s2c_msg.invalidation_message.invalidation.add()
      s2c_msg.invalidation_message.invalidation[
          0].object_id.name = invalidation_type.upper()
      s2c_msg.invalidation_message.invalidation[
          0].object_id.source = types_pb2.ClientType.CHROME_SYNC
      s2c_msg.invalidation_message.invalidation[0].is_known_version = True
      s2c_msg.invalidation_message.invalidation[
          0].version = invalidation_version

      gateway_msg = client_gateway_pb2.ClientGatewayMessage()
      gateway_msg.is_client_to_server = False
      gateway_msg.network_message = s2c_msg.SerializeToString()

      notification_stanza = self.MakeNotification(
          "tango_raw", gateway_msg.SerializeToString())

      print 'XMPP: invalidate <%s>@%s -> %s' % (
          invalidation_type, invalidation_version, connection)
      connection.ForwardNotification(notification_stanza)

  def MakeHeaderWithToken(self, msg, token):
    msg.header.protocol_version.version.major_version = 3
    msg.header.protocol_version.version.minor_version = 2
    msg.header.server_time_ms = int(round(time.time() * 1000))
    msg.header.client_token = token
