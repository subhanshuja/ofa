import json
import opera_auth
import random
import string
import sync_testserver
import time
import urllib
from opera_http import OperaHTTP

USER_EMAIL = 'useremail@testserver.mz'

RESPONSE_VALUES = {
    'SIMPLE_LOGIN_RESPONSE': [
        'SLR_OK',
        'SLR_AUTH',
        'SLR_PARSE',
        'SLR_NETWORK',
        'SLR_HTTP_500',
        'SLR_EMPTY_TOKEN',
        'SLR_EMPTY_SECRET',
        'SLR_EMPTY_BOTH'
    ],

    # Web login response represents the HTTP server
    # response to client requests during sync popup login.
    # There are three stages of popup login:
    # 1. Client requests the /account/login HTML page with a GET request;
    # 2. Client requests the /account/login page with a POST request with
    #    the login data filled in (i.e. form was filled in by the user);
    # 3. Upon a successful login client is redirected to /account/access-token
    #    page that the server returns with HTTP headers representing the login
    #    and auth data (X-Opera-* HTTP headers).
    #
    # Each of the stages can fail either with a network or HTTP error.
    'WEB_LOGIN_RESPONSE': [
        'WLR_OK',
        'WLR_AUTH',
        'WLR_NETWORK_STAGE1',
        'WLR_NETWORK_STAGE2',
        'WLR_NETWORK_STAGE3',
        'WLR_HTTP_500_STAGE1',
        'WLR_HTTP_500_STAGE2',
        'WLR_HTTP_500_STAGE3'
    ],

    # AUTH_ERROR_INVALID_TOKEN: 400,
    # AUTH_ERROR_EXPIRED_TOKEN: 401,
    # AUTH_ERROR_INVALID_CONSUMER_APPLICATION: 402,
    # AUTH_ERROR_TOKEN_KEY_MISMATCH: 403,
    # AUTH_ERROR_BAD_OAUTH_REQUEST: 404,
    # AUTH_ERROR_TIMESTAMP_INCORRECT: 405,
    # AUTH_ERROR_INVALID_TIMESTAMP_NONCE: 406,
    'SYNC_COMMAND_RESPONSE': [
        'SCR_OK',
        'SCR_AUTH_400',
        'SCR_AUTH_401',
        'SCR_AUTH_402',
        'SCR_AUTH_403',
        'SCR_AUTH_404',
        'SCR_AUTH_405',
        'SCR_AUTH_406',
        'SCR_TRANSIENT',
        'SCR_NETWORK',
        'SCR_HTTP_500',
    ],
    'XMPP_CONNECT_RESPONSE': [
        'XCR_OK',
        'XCR_AUTH_400',
        'XCR_AUTH_401',
        'XCR_AUTH_402',
        'XCR_AUTH_403',
        'XCR_AUTH_404',
        'XCR_AUTH_405',
        'XCR_AUTH_406',
        'XCR_TRANSIENT',
        'XCR_NETWORK',
    ],
    'CLOCK_VALIDATION_RESPONSE': [
        'CVR_OK',
        'CVR_BAD_REQUEST',
        'CVR_PLUS_24H',
        'CVR_MINUS_24H',
        'CVR_NETWORK',
        'CVR_HTTP_500',
    ],

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
    #     to exchange a valid active (not yet expired) token for a new one."""
    'TOKEN_RENEWAL_RESPONSE': [
        'TRR_OK',
        'TRR_AUTH_420',
        'TRR_AUTH_421',
        'TRR_AUTH_422',
        'TRR_AUTH_424',
        'TRR_AUTH_425',
        'TRR_AUTH_426',
        'TRR_AUTH_428',
        'TRR_PARSE',
        'TRR_NETWORK',
        'TRR_HTTP_500',
        'TRR_EMPTY_TOKEN',
        'TRR_EMPTY_SECRET',
        'TRR_EMPTY_BOTH'
    ]
}


class Event:

  def __init__(self, event_type, event_details, event_response):
    self.event_type_ = event_type
    self.event_details_ = event_details
    self.event_response_ = event_response
    self.event_timestamp_ = time.strftime('%H:%M:%S')

  def to_string(self):
    return "[%s] %s %s %s" % (self.event_timestamp_, self.event_type_,
                              str(self.event_details_), self.event_response_)


class OperaTrueAuth:

  def __init__(self):
    self.last_token_ = None
    self.response_config_ = {}
    self.setup_initial_responses()
    self.events_ = []
    self.user_name_ = None
    self.user_id_ = 123456

  def append_event(self, event_type, event_details, event_response):
    new_event = Event(event_type, event_details, event_response)
    self.events_.append(new_event)

  def clear_events(self):
    self.events_ = []

  def generate_token(self, from_token):
    if from_token is None:
      token_length = 10
      token = ''.join(random.choice(string.ascii_uppercase)
                      for _ in range(token_length))
      token = token + '_1'
      return token
    else:
      chunks = from_token.split('_')
      if len(chunks) == 1:
        counter = 1
      else:
        counter = int(chunks[1]) + 1
      return "%s_%d" % (chunks[0], counter)

  def setup_initial_responses(self):
    for key in RESPONSE_VALUES:
      response_type = key
      response_values = RESPONSE_VALUES[key]
      ok_response = response_values[0]
      self.set_response(response_type, ok_response)

  def set_response(self, response_type, response_value):
    self.response_config_[response_type] = response_value

  def get_response(self, response_type):
    return self.response_config_[response_type]

  def handle_request(self, req_vars):
    path = req_vars.get('path', None)

    if path == '/account/login':
      return self.handle_account_login(req_vars)
    elif path == '/account/access-token':
      return self.handle_account_access_token(req_vars)
    elif path == '/true_auth/response_control':
      return self.handle_response_control(req_vars)
    elif path == '/true_auth/event_log':
      return self.serve_file_response('true_auth_event_log.html', {})
    elif path == '/true_auth/change_response':
      return self.handle_change_response(req_vars)
    elif path == '/true_auth/fetch_event_log':
      return self.handle_fetch_event_log(req_vars)
    elif path == '/true_auth/reset_event_log':
      return self.handle_reset_event_log(req_vars)
    elif path == '/service/validation/clock':
      return self.handle_service_validation_clock(req_vars)
    elif path == '/account/access-token/renewal':
      return self.handle_access_token_renewal(req_vars)
    elif path == '/service/oauth/simple/access_token':
      return self.handle_simple_access_token(req_vars)

    return self.response(handled=False)

  def on_sync_command(self, command_info_dict):
    self.append_event('SYNC_COMMAND', command_info_dict, 'NONE')

  def verify_token(self, parsed_auth_header, client_addr):
    response = self.get_response('SYNC_COMMAND_RESPONSE')
    self.append_event(
        'SYNC_AUTHORIZE', {
            'client_ip': client_addr[0],
            'oauth_nonce': parsed_auth_header['oauth_nonce'],
            'oauth_timestamp': parsed_auth_header['oauth_timestamp'],
            'oauth_token': parsed_auth_header['oauth_token']
        }, response)

    if response not in RESPONSE_VALUES['SYNC_COMMAND_RESPONSE']:
      raise Exception("Unknown SYNC_COMMAND_RESPONSE: '%s'" % response)

    if response == 'SCR_OK':
      return {'verification_error': opera_auth.VERIFICATION_ERROR_NO_ERROR}
    elif response == 'SCR_NETWORK':
      return {'network_error': opera_auth.NETWORK_ERROR_HARD}
    elif response == 'SCR_TRANSIENT':
      return {'network_error': opera_auth.NETWORK_ERROR_SOFT}
    elif response == 'SCR_HTTP_500':
      return {'http_error': 500}
    elif response == 'SCR_AUTH_400':
      return {'verification_error': 400}
    elif response == 'SCR_AUTH_401':
      return {'verification_error': 401}
    elif response == 'SCR_AUTH_402':
      return {'verification_error': 402}
    elif response == 'SCR_AUTH_403':
      return {'verification_error': 403}
    elif response == 'SCR_AUTH_404':
      return {'verification_error': 404}
    elif response == 'SCR_AUTH_405':
      return {'verification_error': 405}
    elif response == 'SCR_AUTH_406':
      return {'verification_error': 406}

  def on_xmpp_connected(self, client_addr):
    self.append_event('XMPP_CONNECT', {'client_ip': client_addr[0]}, 'NONE')

  def on_xmpp_disconnected(self, client_addr):
    self.append_event('XMPP_DISCONNECT', {'client_ip': client_addr[0]}, 'NONE')

  def on_account_reset(self):
    self.append_event('ACCOUNT_RESET', {}, 'NONE')

  def verify_xmpp_connect_attempt(self, parsed_auth_header, client_addr):
    """
    Called by the XMPP server when a client connects.
    @parsed_auth_header: A dictionary containing the decoded Authorization
       header as sent by the client within the connection attempt.
    @client_addr: The remote address of the connecting client, in a
       "IP:PORT" format.
    """
    client_addr = client_addr.split(':')
    response = self.get_response('XMPP_CONNECT_RESPONSE')
    self.append_event(
        'XMPP_AUTHORIZE', {
            'client_ip': client_addr[0],
            'oauth_nonce': parsed_auth_header['oauth_nonce'],
            'oauth_timestamp': parsed_auth_header['oauth_timestamp'],
            'oauth_token': parsed_auth_header['oauth_token']
        }, response)

    if response not in RESPONSE_VALUES['XMPP_CONNECT_RESPONSE']:
      raise Exception("Unknown XMPP_CONNECT_RESPONSE: '%s'" % response)

    if response == 'XCR_OK':
      return {
          'verification_error': opera_auth.VERIFICATION_ERROR_NO_ERROR,
          'jid': USER_EMAIL
      }
    elif response == 'XCR_NETWORK':
      return {'network_error': opera_auth.NETWORK_ERROR_HARD}
    elif response == 'XCR_TRANSIENT':
      return {'network_error': opera_auth.NETWORK_ERROR_SOFT}
    elif response == 'XCR_AUTH_400':
      return {'verification_error': 400}
    elif response == 'XCR_AUTH_401':
      return {'verification_error': 401}
    elif response == 'XCR_AUTH_402':
      return {'verification_error': 402}
    elif response == 'XCR_AUTH_403':
      return {'verification_error': 403}
    elif response == 'XCR_AUTH_404':
      return {'verification_error': 404}
    elif response == 'XCR_AUTH_405':
      return {'verification_error': 405}
    elif response == 'XCR_AUTH_406':
      return {'verification_error': 406}

  def handle_simple_access_token(self, req_vars):

    def _construct_auth_response(req_vars, token_ok, secret_ok):
      username = req_vars['post'].get('x_username', '')
      assert (username != '')
      self.user_name_ = username
      token = ''
      secret = ''
      if token_ok:
        token = self.generate_token(None)
      if secret_ok:
        secret = token + "_secret"
      json_dict = {'oauth_token': token, 'oauth_token_secret': secret}
      return json_dict

    response = self.get_response('SIMPLE_LOGIN_RESPONSE')
    self.append_event('SIMPLE_LOGIN_REQUEST', {'POST': req_vars['post']},
                      response)
    if response == 'SLR_OK':
      return self.response(
          to_urlencoded=_construct_auth_response(req_vars, True, True))
    elif response == 'SLR_AUTH':
      json_dict = {'error': 'Error induced by SLR_AUTH', 'success': 0}
      return self.response(to_urlencoded=json_dict)
    elif response == 'SLR_NETWORK':
      return self.response(network_error=True)
    elif response == 'SLR_PARSE':
      return self.response(content='garbage')
    elif response == 'SLR_HTTP_500':
      return self.response(code=500)
    elif response == 'SLR_EMPTY_TOKEN':
      return self.response(
          to_urlencoded=_construct_auth_response(req_vars, False, True))
    elif response == 'SLR_EMPTY_SECRET':
      return self.response(
          to_urlencoded=_construct_auth_response(req_vars, True, False))
    elif response == 'SLR_EMPTY_BOTH':
      return self.response(
          to_urlencoded=_construct_auth_response(req_vars, False, False))

  def handle_access_token_renewal(self, req_vars):

    def _generate_renewal_response(req_vars, token_ok, secret_ok):
      old_token = req_vars['get'].get('old_token')
      new_token = ''
      if token_ok:
        new_token = self.generate_token(old_token)
      new_secret = ''
      if secret_ok:
        new_secret = new_token + '_secret'
      json_dict = {
          'auth_token': new_token,
          'auth_token_secret': new_secret,
          'userID': self.user_id_,
          'userName': self.user_name_,
          'userEmail': USER_EMAIL,
      }
      return json_dict

    final_response = None
    desired_response = self.get_response('TOKEN_RENEWAL_RESPONSE')

    if desired_response not in RESPONSE_VALUES['TOKEN_RENEWAL_RESPONSE']:
      raise Exception("Unknown TOKEN_RENEWAL_RESPONSE: '%s'" % desired_response)

    if desired_response == 'TRR_OK':
      final_response = self.response(
          to_json=_generate_renewal_response(req_vars, True, True))
    elif desired_response == 'TRR_AUTH_420':
      json_dict = {'err_code': 420, 'err_msg': 'Request not authorized'}
      final_response = self.response(to_json=json_dict)
    elif desired_response == 'TRR_AUTH_421':
      json_dict = {'err_code': 421, 'err_msg': 'Bad request'}
      final_response = self.response(to_json=json_dict)
    elif desired_response == 'TRR_AUTH_422':
      json_dict = {'err_code': 422, 'err_msg': 'Opera user not found'}
      final_response = self.response(to_json=json_dict)
    elif desired_response == 'TRR_AUTH_424':
      json_dict = {'err_code': 424, 'err_msg': 'Access token not found'}
      final_response = self.response(to_json=json_dict)
    elif desired_response == 'TRR_AUTH_425':
      json_dict = {'err_code': 425, 'err_msg': 'Invalid Opera token'}
      final_response = self.response(to_json=json_dict)
    elif desired_response == 'TRR_AUTH_426':
      json_dict = {'err_code': 426, 'err_msg': 'Could not generate Opera token'}
      final_response = self.response(to_json=json_dict)
    elif desired_response == 'TRR_AUTH_428':
      json_dict = {'err_code': 428, 'err_msg': 'Access token is not expired'}
      final_response = self.response(to_json=json_dict)
    elif desired_response == 'TRR_PARSE':
      final_response = self.response(content='garbage')
    elif desired_response == 'TRR_NETWORK':
      final_response = self.response(network_error=True)
    elif desired_response == 'TRR_HTTP_500':
      final_response = self.response(code=500)
    elif desired_response == 'TRR_EMPTY_TOKEN':
      final_response = self.response(
          to_json=_generate_renewal_response(req_vars, False, True))
    elif desired_response == 'TRR_EMPTY_SECRET':
      final_response = self.response(
          to_json=_generate_renewal_response(req_vars, True, False))
    elif desired_response == 'TRR_EMPTY_BOTH':
      final_response = self.response(
          to_json=_generate_renewal_response(req_vars, False, False))

    self.append_event(
        'ACCESS_TOKEN_RENEWAL_REQUEST',
        {'GET': req_vars['get'],
         'RESPONSE': final_response}, desired_response)

    return final_response

  def handle_service_validation_clock(self, req_vars):
    response = self.get_response('CLOCK_VALIDATION_RESPONSE')
    self.append_event('CLOCK_VALIDATION_REQUEST', {'POST': req_vars['post']},
                      response)
    if response == 'CVR_OK':
      json_dict = {
          'status': 'success',
          'diff': 'NaN',
          'message': '',
          'code': 200
      }
      return self.response(to_json=json_dict)
    elif response == 'CVR_NETWORK':
      return self.response(network_error=True)
    elif response == 'CVR_HTTP_500':
      return self.response(code=500)
    elif response == 'CVR_BAD_REQUEST':
      json_dict = {
          'status': 'error',
          'diff': 'NaN',
          'message': 'Bad request',
          'code': 400
      }
      return self.response(to_json=json_dict)
    elif response == 'CVR_MINUS_24H':
      json_dict = {
          'status': 'error',
          'diff': -172800,
          'message': 'Clock differs too much',
          'code': 401
      }
      return self.response(to_json=json_dict)
    elif response == 'CVR_PLUS_24H':
      json_dict = {
          'status': 'error',
          'diff': 172800,
          'message': 'Clock differs too much',
          'code': 401
      }
      return self.response(to_json=json_dict)

  def handle_fetch_event_log(self, req_vars):
    content = ""
    for e in self.events_:
      content = content + e.to_string() + "\n"

    return self.response(content=content)

  def handle_reset_event_log(self, req_vars):
    self.clear_events()
    return self.response(content="Auth event log cleared OK")

  def handle_change_response(self, req_vars):
    key = req_vars['get'].get('key')
    value = req_vars['get'].get('value')

    self.set_response(key, value)
    return self.serve_redirect_response('/true_auth/response_control')

  def handle_response_control(self, req_vars):
    replacements = {}
    replacements['RESPONSES_JSON'] = json.dumps(RESPONSE_VALUES)
    replacements['CURRENT_RESPONSES'] = self.response_config_
    return self.serve_file_response('true_auth_control.html', replacements)

  def handle_account_login(self, req_vars):
    username = req_vars['post'].get('email', None)
    password = req_vars['post'].get('password', None)
    replacements = {'SYNC_SERVER_ERROR_STRING': ''}
    response = self.get_response('WEB_LOGIN_RESPONSE')
    if username and password:
      self.append_event('WEB_LOGIN_REQUEST_STAGE2', {'POST': req_vars['post']},
                        response)
      if response == 'WLR_AUTH':
        replacements[
            'SYNC_SERVER_ERROR_STRING'
        ] = 'Auth error induced by WLR_AUTH setting.'
        return self.serve_file_response('true_auth_login.html', replacements)
      elif response == 'WLR_NETWORK_STAGE2':
        return self.response(network_error=True)
      elif response == 'WLR_HTTP_500_STAGE2':
        return self.response(code=500)
      else:
        self.user_name_ = username
        return self.serve_redirect_response('/account/access-token')
    elif not (username and password):
      self.append_event('WEB_LOGIN_REQUEST_STAGE1', {'POST': req_vars['post']},
                        response)
      if response == 'WLR_NETWORK_STAGE1':
        return self.response(network_error=True)
      elif response == 'WLR_HTTP_500_STAGE1':
        return self.response(code=500)

      return self.serve_file_response('true_auth_login.html', replacements)
    else:
      print "TRUE AUTH: Incomplete POST data [%s]" % req_vars['post']
      return self.response(code=500, content='Incomplete POST data')

  """This method serves the client call to the /account/access-token endpoint.
     Compare web login flow described by WEB_LOGIN_RESPONSE definition at
     the top of this file."""

  def handle_account_access_token(self, req_vars):
    response = self.get_response('WEB_LOGIN_RESPONSE')
    self.append_event('WEB_LOGIN_REQUEST_STAGE3', {'POST': req_vars['post']},
                      response)
    if response == 'WLR_NETWORK_STAGE3':
      return self.response(network_error=True)
    elif response == 'WLR_HTTP_500_STAGE3':
      return self.response(code=500)

    assert (response == 'WLR_OK')

    headers = {}
    token = self.generate_token(None)
    token_secret = token + '_secret'
    headers['X-Opera-Auth-AccessToken'] = token
    headers['X-Opera-Auth-AccessTokenSecret'] = token_secret
    headers['X-Opera-Auth-UserName'] = self.user_name_
    headers['X-Opera-Auth-UserID'] = str(self.user_id_)
    headers['X-Opera-Auth-UserEmail'] = USER_EMAIL
    headers['X-Opera-Auth-EmailVerified'] = '1'
    content = 'User logged in OK.'
    self.append_event('WEB_ACCESS_TOKEN_REQUEST',
                      {'token': token,
                       'token_secret': token_secret}, "OK")
    return self.response(headers=headers, content=content)

  def serve_redirect_response(self, url):
    headers = {}
    headers['Location'] = url
    return self.response(code=302, headers=headers)

  def serve_file_response(self, filename, replacements={}):
    filepath = OperaHTTP.AsDataDirPath(filename)
    content = open(filepath, 'r').read()
    for key in replacements.keys():
      findstr = '[%s]' % key
      replacestr = str(replacements[key])
      content = content.replace(findstr, replacestr)
    return self.response(content_type='text/html', content=content)

  def response(self,
               handled=True,
               code=200,
               headers=[],
               content_type='text/html',
               content='',
               to_json=None,
               to_urlencoded=None,
               network_error=False):
    if content and to_json is not None:
      raise RuntimeError('Cant send both content and to_json!')

    if to_json is not None:
      content = json.dumps(to_json,
                           sort_keys=True,
                           indent=2,
                           separators=(',', ': '))
      content_type = 'application/json'

    if to_urlencoded is not None:
      content = urllib.urlencode(to_urlencoded)
      content_type = 'application/x-www-form-urlencoded'

    ret_dict = {
        'handled': handled,
        'code': code,
        'headers': headers,
        'type': content_type,
        'content': content,
        'network_error': network_error,
    }
    return ret_dict
