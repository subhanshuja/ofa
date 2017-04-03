import json
import urllib
from datetime import datetime

import chromiumsync

USER_NAME = "intercept_username"

AUTH_ERROR_NO_ERROR = 0
AUTH_ERROR_INVALID_TOKEN = 400
AUTH_ERROR_EXPIRED_TOKEN = 401
AUTH_ERROR_INVALID_CONSUMER_APPLICATION = 402
AUTH_ERROR_TOKEN_KEY_MISMATCH = 403
AUTH_ERROR_BAD_OAUTH_REQUEST = 404
AUTH_ERROR_TIMESTAMP_INCORRECT = 405
AUTH_ERROR_INVALID_TIMESTAMP_NONCE = 406

AUTH_RENEWAL_NO_ERROR = 0
AUTH_RENEWAL_NOT_AUTHORIZED_REQUEST = 420
AUTH_RENEWAL_BAD_REQUEST = 421
AUTH_RENEWAL_OPERA_USER_NOT_FOUND = 422
AUTH_RENEWAL_OPERA_TOKEN_NOT_FOUND = 423
AUTH_RENEWAL_INVALID_OPERA_TOKEN = 425
AUTH_RENEWAL_COULD_NOT_GENERATE_OPERA_TOKEN = 426
AUTH_RENEWAL_OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED = 428


class OperaInterceptTokenInfo:

  def __init__(self, token, client_host, username):
    self._token = token
    self._data = {}
    self._data['next_auth_response'] = AUTH_ERROR_NO_ERROR
    self._data['auth_renewal_response'] = AUTH_RENEWAL_NO_ERROR
    self._data['ctime'] = datetime.now()
    self._data['client_host'] = client_host
    self._data['username'] = username

  def set_next_auth_response(self, next_auth_response):
    self._data['next_auth_response'] = int(next_auth_response)

  def set_auth_renewal_response(self, auth_renewal_response):
    self._data['auth_renewal_response'] = int(auth_renewal_response)

  def next_auth_response(self):
    return int(self._data['next_auth_response'])

  def auth_renewal_response(self):
    return int(self._data['auth_renewal_response'])

  def data(self):
    return str(self._data)

  def next_auth_response_to_string(self, next_auth_response):
    next_auth_response = int(next_auth_response)

    if next_auth_response == AUTH_ERROR_NO_ERROR:
      return "AUTH_ERROR_NO_ERROR"
    elif next_auth_response == AUTH_ERROR_INVALID_TOKEN:
      return "AUTH_ERROR_INVALID_TOKEN"
    elif next_auth_response == AUTH_ERROR_EXPIRED_TOKEN:
      return "AUTH_ERROR_EXPIRED_TOKEN"
    elif next_auth_response == AUTH_ERROR_INVALID_CONSUMER_APPLICATION:
      return "AUTH_ERROR_INVALID_CONSUMER_APPLICATION"
    elif next_auth_response == AUTH_ERROR_TOKEN_KEY_MISMATCH:
      return "AUTH_ERROR_TOKEN_KEY_MISMATCH"
    elif next_auth_response == AUTH_ERROR_BAD_OAUTH_REQUEST:
      return "AUTH_ERROR_BAD_OAUTH_REQUEST"
    elif next_auth_response == AUTH_ERROR_TIMESTAMP_INCORRECT:
      return "AUTH_ERROR_TIMESTAMP_INCORRECT"
    elif next_auth_response == AUTH_ERROR_INVALID_TIMESTAMP_NONCE:
      return "AUTH_ERROR_INVALID_TIMESTAMP_NONCE"

    raise Exception("Not reached %s" % next_auth_response)

  def auth_renewal_response_to_string(self, auth_renewal_response):
    auth_renewal_response = int(auth_renewal_response)

    if auth_renewal_response == AUTH_RENEWAL_NO_ERROR:
      return "AUTH_RENEWAL_NO_ERROR"
    elif auth_renewal_response == AUTH_RENEWAL_NOT_AUTHORIZED_REQUEST:
      return "AUTH_RENEWAL_NOT_AUTHORIZED_REQUEST"
    elif auth_renewal_response == AUTH_RENEWAL_BAD_REQUEST:
      return "AUTH_RENEWAL_BAD_REQUEST"
    elif auth_renewal_response == AUTH_RENEWAL_OPERA_USER_NOT_FOUND:
      return "AUTH_RENEWAL_OPERA_USER_NOT_FOUND"
    elif auth_renewal_response == AUTH_RENEWAL_OPERA_TOKEN_NOT_FOUND:
      return "AUTH_RENEWAL_OPERA_TOKEN_NOT_FOUND"
    elif auth_renewal_response == AUTH_RENEWAL_INVALID_OPERA_TOKEN:
      return "AUTH_RENEWAL_INVALID_OPERA_TOKEN"
    elif auth_renewal_response == AUTH_RENEWAL_COULD_NOT_GENERATE_OPERA_TOKEN:
      return "AUTH_RENEWAL_COULD_NOT_GENERATE_OPERA_TOKEN"
    elif auth_renewal_response == AUTH_RENEWAL_OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED:
      return "AUTH_RENEWAL_OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED"

    raise Exception("Not reached")

  def time_to_string(self, a_time):
    return a_time.strftime("%Y-%m-%d %H:%M:%S")

  def desc(self):
    final = ""
    final = final + self.state_to_string(self._data['state'])
    final = final + " "
    final = final + self.time_to_string(self._data['ctime'])
    final = final + " "
    final = final + self._data['client_host']
    return final

  def client_host(self):
    return self._data['client_host']

  def username(self):
    return self._data['username']

  def next_auth_response_string(self):
    return self.next_auth_response_to_string(self._data['next_auth_response'])

  def auth_renewal_response_tring(self):
    return self.auth_renewal_response_to_string(self._data[
        'auth_renewal_response'])

  def ctime_string(self):
    return self.time_to_string(self._data['ctime'])


class OperaInterceptAuth:

  def __init__(self):
    # token string -> OperaInterceptToken map
    self._tokens = {}

  def handle_get_request_tokens(self, get_vars):

    def append_option(option_name, option_value):
      option_string = "<option value='%s'>%s</option>" % (option_value,
                                                          option_name)
      return option_string

    text = "Tokens:<br><table>"
    for token in self._tokens:
      token_obj = self._tokens[token]
      next_auth_response_select_string = "<select name='next_auth_response'>"
      next_auth_response_select_string += append_option('AUTH_ERROR_NO_ERROR',
                                                        AUTH_ERROR_NO_ERROR)
      next_auth_response_select_string += append_option(
          'AUTH_ERROR_INVALID_TOKEN', AUTH_ERROR_INVALID_TOKEN)
      next_auth_response_select_string += append_option(
          'AUTH_ERROR_EXPIRED_TOKEN', AUTH_ERROR_EXPIRED_TOKEN)
      next_auth_response_select_string += append_option(
          'AUTH_ERROR_INVALID_CONSUMER_APPLICATION',
          AUTH_ERROR_INVALID_CONSUMER_APPLICATION)
      next_auth_response_select_string += append_option(
          'AUTH_ERROR_TOKEN_KEY_MISMATCH', AUTH_ERROR_TOKEN_KEY_MISMATCH)
      next_auth_response_select_string += append_option(
          'AUTH_ERROR_BAD_OAUTH_REQUEST', AUTH_ERROR_BAD_OAUTH_REQUEST)
      next_auth_response_select_string += append_option(
          'AUTH_ERROR_TIMESTAMP_INCORRECT', AUTH_ERROR_TIMESTAMP_INCORRECT)
      next_auth_response_select_string += append_option(
          'AUTH_ERROR_INVALID_TIMESTAMP_NONCE',
          AUTH_ERROR_INVALID_TIMESTAMP_NONCE)
      next_auth_response_select_string += "</select>"

      auth_renewal_response_select_string = "<select name='auth_renewal_response'>"
      auth_renewal_response_select_string += append_option(
          'AUTH_RENEWAL_NO_ERROR', AUTH_RENEWAL_NO_ERROR)
      auth_renewal_response_select_string += append_option(
          'AUTH_RENEWAL_NOT_AUTHORIZED_REQUEST',
          AUTH_RENEWAL_NOT_AUTHORIZED_REQUEST)
      auth_renewal_response_select_string += append_option(
          'AUTH_RENEWAL_BAD_REQUEST', AUTH_RENEWAL_BAD_REQUEST)
      auth_renewal_response_select_string += append_option(
          'AUTH_RENEWAL_OPERA_USER_NOT_FOUND',
          AUTH_RENEWAL_OPERA_USER_NOT_FOUND)
      auth_renewal_response_select_string += append_option(
          'AUTH_RENEWAL_OPERA_TOKEN_NOT_FOUND',
          AUTH_RENEWAL_OPERA_TOKEN_NOT_FOUND)
      auth_renewal_response_select_string += append_option(
          'AUTH_RENEWAL_INVALID_OPERA_TOKEN', AUTH_RENEWAL_INVALID_OPERA_TOKEN)
      auth_renewal_response_select_string += append_option(
          'AUTH_RENEWAL_COULD_NOT_GENERATE_OPERA_TOKEN',
          AUTH_RENEWAL_COULD_NOT_GENERATE_OPERA_TOKEN)
      auth_renewal_response_select_string += append_option(
          'AUTH_RENEWAL_OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED',
          AUTH_RENEWAL_OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED)
      auth_renewal_response_select_string += "</select>"

      hidden_token_field = "<input type='hidden' name='token' value='%s'>" % (
          token)
      change_state_button = "<input type='submit' value='Change'>"

      row = "<tr>" \
            "<td><b>%s</b></td>" \
            "<td><font color='navy'>%s</font></td>" \
            "<td><font color='red'>%s</font></td>" \
            "<td><font color='green'>%s</font></td>" \
            "<td><font color='blue'>%s</font></td>" \
            "<td><form action='/opera_intercept_auth/change_token' method='POST'>%s%s%s%s</form></td>" \
            "</tr>" % (token, token_obj.next_auth_response_string(), \
                token_obj.auth_renewal_response_tring(), \
                token_obj.ctime_string(), token_obj.client_host(), next_auth_response_select_string, \
                auth_renewal_response_select_string, hidden_token_field, change_state_button)
      text = text + row

    text = text + "</table>"
    return self.response(handled=True, content=text)

  def handle_post_change_token(self, get_vars):
    headers = {}
    headers['Location'] = '/opera_intercept_auth/tokens'
    code = 302

    token = get_vars['post'].get('token', None)
    next_auth_response = get_vars['post'].get('next_auth_response', None)
    auth_renewal_response = get_vars['post'].get('auth_renewal_response', None)
    if token is None:
      raise Exception('Expecting token in POST vars.')

    if next_auth_response is None:
      raise Exception('Expecting next_auth_response in POST vars.')

    if auth_renewal_response is None:
      raise Exception('Expecting auth_renewal_response in POST vars.')

    token_info = self.get_token_info(token)
    if not token_info:
      raise Exception('Token not found.')

    token_info.set_next_auth_response(next_auth_response)
    token_info.set_auth_renewal_response(auth_renewal_response)

    return self.response(handled=True, code=302, headers=headers)

  def handle_service_validation_clock(self, get_vars):
    """Client sends a POST request with the following data: {'localtime': '1438348125'}

      Example response when clock is within 24 hours from the clock on auth.opera.com:
        {"status":"success","diff":"NaN","message":"","code":200}

      Example responses when clock is more than 24 hours off from the clock on auth.opera.com:
        {"status":"error","diff":-100080,"message":"Clock differs too much","code":401}
        {"status":"error","diff":199890,"message":"Clock differs too much","code":401}

      Example of server response on bad request:
        {"status":"error","diff":"NaN","message":"Bad request","code":400}"""

    json_dict = {'status': 'success',
                 'diff': 'NaN',
                 'message': '',
                 'code': 200}

    return self.response(to_json=json_dict)

  def handle_account_access_token_renewal(self, get_vars):
    """Client sends a GET request with the following data:
    {
     'old_token': 'token_dupa1_dupa1pass',
     'consumer_key': 'LDKgAObtMIV1eVp1Jb2b0ZlqK1TUDkk3',
     'service': 'desktop2',
     'signature': '659f9c91c8195455b7b03d5b490bf5ecb15e98ef'
    }
    Proper auth responses:
      An example of response structure in case of successful token renewal:
        {
          "auth_token": "ATxnCeCm3fh12d79baDaYprFKsn7GAq9",
          "auth_token_secret": "6JhxuBc97phBFthg83PZNZ25RwTUhxEy",
          "userID": "212784215",
          "userName": null,
          "userEmail": "authtestuser@gmail.com"
        }

    An example of response structure in case of error:
        {"err_code":425,"err_msg":"Invalid Opera access token"}

    Error codes:
    420 (NOT_AUTHORIZED_REQUEST) - the request is not authorized, as the
        service is not allowed to request Opera access token;
    421 (BAD_REQUEST) - in case wrong request signature for example;
    422 (OPERA_USER_NOT_FOUND) - the owner of access token, Opera user is
        not found;
    424 (OPERA_TOKEN_NOT_FOUND) - access token not found;
    425 (INVALID_OPERA_TOKEN) - this Opera token has been exchanged for a
        new one already or has been invalidated by user;
    426 (COULD_NOT_GENERATE_OPERA_TOKEN) - more an internal error related
        to inability to issue a new Opera access token.
    428 (OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED) - the error indicates an attempt
        to exchange a valid active (not yet expired) token for a new one."""
    old_token = get_vars['get'].get('old_token', None)

    if old_token is None:
      raise Exception("Expected old_token in GET request!")

    token_obj = self.get_token_info(old_token)

    if token_obj is None:
      json_dict = {'err_code': 424, 'err_msg': "424 Opera token not found"}
    elif token_obj.auth_renewal_response() == AUTH_RENEWAL_NO_ERROR:
      client_host = token_obj.client_host()
      new_token = chromiumsync.GenerateOperaAuthToken(10)
      self.add_token(new_token, client_host, token_obj.username())
      json_dict = {
          'auth_token': new_token,
          'auth_token_secret': "%s_secret" % new_token,
          'userID': 9999,
          'userName': token_obj.username(),
          'userEmail': 'user@email.com'
      }
    elif token_obj.auth_renewal_response(
    ) == AUTH_RENEWAL_NOT_AUTHORIZED_REQUEST:
      json_dict = {'err_code': 420, 'err_msg': "420 Request not authorized"}
    elif token_obj.auth_renewal_response() == AUTH_RENEWAL_BAD_REQUEST:
      json_dict = {'err_code': 421, 'err_msg': "421 Bad request"}
    elif token_obj.auth_renewal_response() == AUTH_RENEWAL_OPERA_USER_NOT_FOUND:
      json_dict = {'err_code': 422, 'err_msg': "422 Opera user not found"}
    elif token_obj.auth_renewal_response(
    ) == AUTH_RENEWAL_OPERA_TOKEN_NOT_FOUND:
      json_dict = {'err_code': 424, 'err_msg': "424 Opera token not found"}
    elif token_obj.auth_renewal_response() == AUTH_RENEWAL_INVALID_OPERA_TOKEN:
      json_dict = {'err_code': 425, 'err_msg': "425 Invalid Opera token"}
    elif token_obj.auth_renewal_response(
    ) == AUTH_RENEWAL_COULD_NOT_GENERATE_OPERA_TOKEN:
      json_dict = {
          'err_code': 426,
          'err_msg': "426 Could not generate Opera token"
      }
    elif token_obj.auth_renewal_response(
    ) == AUTH_RENEWAL_OPERA_ACCESS_TOKEN_IS_NOT_EXPIRED:
      json_dict = {
          'err_code': 428,
          'err_msg': "428 Opera access token not expired"
      }
    else:
      raise Exception("Auth renewal response unknown")

    return self.response(to_json=json_dict)

  def try_handle_renewal(self, get_vars):
    renewal_handlers = {
        '/service/validation/clock': 'handle_service_validation_clock',
        '/account/access-token/renewal': 'handle_account_access_token_renewal'
    }
    path = get_vars['path']

    if renewal_handlers.get(path, None):
      response = getattr(self, renewal_handlers[path])(get_vars)
      return (True, response)

    return (False, None)

  def do_handle(self, get_vars):
    (handled, response) = self.try_handle_renewal(get_vars)
    if handled:
      return response

    path_split = get_vars['path'].split('/')
    if len(path_split) < 3:
      return self.response(handled=False)

    if path_split[0]:
      raise Exception("Expecting empty string")

    if path_split[1] == 'opera_intercept_auth':
      command = path_split[2]
      if command == 'tokens':
        return self.handle_get_request_tokens(get_vars)
      elif command == 'change_token':
        return self.handle_post_change_token(get_vars)

    return self.response(handled=False)

  def get_token_info(self, token):
    return self._tokens.get(token, None)

  def add_token(self, token, client_host, username):
    if self.get_token_info(token):
      raise Exception("Should not happen")

    token_info = OperaInterceptTokenInfo(token, client_host, username)
    self._tokens[token] = token_info
    return token_info

  def verify_token(self, token, client_address):
    # No error: 0
    # AUTH_ERROR_INVALID_TOKEN: 400,
    # AUTH_ERROR_EXPIRED_TOKEN: 401,
    # AUTH_ERROR_INVALID_CONSUMER_APPLICATION: 402,
    # AUTH_ERROR_TOKEN_KEY_MISMATCH: 403,
    # AUTH_ERROR_BAD_OAUTH_REQUEST: 404,
    # AUTH_ERROR_TIMESTAMP_INCORRECT: 405,
    # AUTH_ERROR_INVALID_TIMESTAMP_NONCE: 406,

    host, port = client_address

    print "INTERCEPT_AUTH: Signed with token '%s' from '%s'" % (token, host)

    token_info = self.get_token_info(token)
    if token_info is None:
      # Username should come from login popup, tie this with proper
      # handler.
      self.add_token(token, host, USER_NAME)
      return (USER_NAME, 0)
    else:
      return (token_info.username(), token_info.next_auth_response())

  def response(self,
               handled=True,
               code=200,
               headers=[],
               content_type='text/html',
               content='',
               to_json=None):
    if content and to_json is not None:
      raise RuntimeError('Cant send both content and to_json!')

    if to_json is not None:
      content = json.dumps(to_json,
                           sort_keys=True,
                           indent=2,
                           separators=(',', ': '))
      content_type = 'application/json'

    ret_dict = {
        'handled': handled,
        'code': code,
        'headers': headers,
        'type': content_type,
        'content': content
    }
    return ret_dict
