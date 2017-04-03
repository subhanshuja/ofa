import json
import urllib

import chromiumsync

REAL_AUTH_PREFIX = '/opera/real_auth'

LIST_ACCOUNTS_SUFFIX = '/list_accounts'
ADD_ACCOUNT_SUFFIX = '/add_account'
REMOVE_ACCOUNT_SUFFIX = '/remove_account'
CHANGE_PASSWORD_SUFFIX = '/change_password'
ACCOUNT_INFO_SUFFIX = '/account_info'

SIMPLE_ACCESS_TOKEN_PATH = '/service/oauth/simple/access_token'
ACCOUNT_ACCESS_TOKEN_PATH = '/account/access-token'
SERVICE_VALIDATION_CLOCK_PATH = '/service/validation/clock'
ACCOUNT_ACCESS_TOKEN_RENEWAL_PATH = '/account/access-token/renewal'


class OperaAuthAccount:
  """Represents a single auth account, wraps any functionality
  related to it."""
  last_id = 0

  def __init__(self, username=None, password=None, email=None):
    OperaAuthAccount.last_id += 1
    account_id = OperaAuthAccount.last_id
    self._data = {'username': username,
                  'password': password,
                  'id': account_id,
                  'email': email}
    self._stats = {}

  def is_valid(self):
    if self._data.get('username', None) is None:
      return False
    if self._data.get('password', None) is None:
      return False
    return True

  def __str__(self):
    return json.dumps(self._data)

  def pop_stat(self, name):
    cur_val = self._stats.get(name, 0)
    cur_val += 1
    self._stats[name] = cur_val

  def get(self, key):
    return self._data.get(key, None)

  def data(self):
    return self._data

  def stats(self):
    return self._stats

  def change_password(self, new_password):
    self.pop_stat('password_change_requests')
    self._data['password'] = new_password
    # Invalidate token once we issue real tokens.

  def _token_data(self):
    data = {}
    mixed = '%s_%s' % (self.get('username'), self.get('password'))
    data['token'] = 'token_%s' % mixed
    data['secret'] = 'secret_%s' % mixed
    return data

  def is_token_valid(self, token):
    self.pop_stat('token_validation_requests')
    data = self._token_data()
    if data['token'] == token:
      self.pop_stat('successful_token_validation_requests')
      return True
    self.pop_stat('failed_token_validation_requests')
    return False

  def issue_token(self):
    # Should issue a real tracked token here.
    self.pop_stat('issue_token_requests')
    return self._token_data()


class OperaRealAuth:
  """Handles all functionality needed to simulate auth.opera.com
  behaviour - authorization, token verification, manipulation of
  user accounts."""

  def __init__(self, send_trick_invalidation_callback):
    self._accounts = {}
    # We'll need this to send invalidation on password change.
    self._send_trick_invalidation_callback = send_trick_invalidation_callback

  def do_handle(self, request_vars):
    path = request_vars['path']
    if path.startswith(REAL_AUTH_PREFIX):
      return self.do_handle_real_auth_prefix(request_vars)

    handlers = {
        SIMPLE_ACCESS_TOKEN_PATH: 'do_handle_simple_access_token',
        ACCOUNT_ACCESS_TOKEN_PATH: 'do_handle_account_access_token',
        SERVICE_VALIDATION_CLOCK_PATH: 'do_handle_service_validation_clock',
        ACCOUNT_ACCESS_TOKEN_RENEWAL_PATH: 'do_handle_access_token_renewal',
    }
    if handlers.get(path, None) is None:
      return self.response(handled=False)
    return getattr(self, handlers[path])(request_vars)

  def do_handle_real_auth_prefix(self, request_vars):
    suffix = request_vars['path']
    suffix = suffix[len(REAL_AUTH_PREFIX):]
    suffix_to_handler = {
        LIST_ACCOUNTS_SUFFIX: 'do_handle_list_accounts',
        ADD_ACCOUNT_SUFFIX: 'do_handle_add_account',
        REMOVE_ACCOUNT_SUFFIX: 'do_handle_remove_account',
        CHANGE_PASSWORD_SUFFIX: 'do_handle_change_password',
        ACCOUNT_INFO_SUFFIX: 'do_handle_account_info',
    }
    if suffix_to_handler.get(suffix, None) is None:
      raise RuntimeError("Unknown suffix '%s'" % suffix)
    return getattr(self, suffix_to_handler[suffix])(request_vars)

  def verify_token(self, token):
    """Verifies the given token.
    Expects a token that was used to sign a sync request
    Returns a (username, result) tuple. If result is non-zero,
    username is None. Result may be one of:

    No error: 0
    AUTH_ERROR_INVALID_TOKEN: 400,
    AUTH_ERROR_EXPIRED_TOKEN: 401,
    AUTH_ERROR_INVALID_CONSUMER_APPLICATION: 402,
    AUTH_ERROR_TOKEN_KEY_MISMATCH: 403,
    AUTH_ERROR_BAD_OAUTH_REQUEST: 404,
    AUTH_ERROR_TIMESTAMP_INCORRECT: 405,
    AUTH_ERROR_INVALID_TIMESTAMP_NONCE: 406,

    NOTE: Since we are not really issuing tokens,
    and a token is a combination of username and password,
    a persistent client session will still be valid across
    server restarts given that a username is recreated on the
    server with same password.
    This should not have any side effects other than the
    NOT_MY_BIRTHDAY errors in case the server was restarted."""
    for username in self._accounts:
      account = self._accounts[username]
      if account.is_token_valid(token):
        return (username, 0)

    return (None, 400)

  def do_handle_access_token_renewal(self, request_vars):
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

    # We always fail to renew a token, we don't implement token expiration.
    json_dict = {
        'err_code': 425,
        'err_msg': 'Didnt expect a token renewal request'
    }
    return self.response(to_json=json_dict)

  def do_handle_service_validation_clock(self, request_vars):
    """Client sends a POST request with the following data: {'localtime': '1438348125'}

      Example response when clock is within 24 hours from the clock on auth.opera.com:
        {"status":"success","diff":"NaN","message":"","code":200}

      Example responses when clock is more than 24 hours off from the clock on auth.opera.com:
        {"status":"error","diff":-100080,"message":"Clock differs too much","code":401}
        {"status":"error","diff":199890,"message":"Clock differs too much","code":401}

      Example of server response on bad request:
        {"status":"error","diff":"NaN","message":"Bad request","code":400}"""

    # Always success
    json_dict = {
        'status': 'success',
        'diff': 'NaN',
        'message': '',
        'code': 200
    }
    return self.response(to_json=json_dict)

  def do_handle_account_access_token(self, request_vars):
    """Given the login data from the login form do the following:
    1. Redirect client back to the login form with an error message
       in case the login data is invalid.
    2. Issue the auth data as expected in case the login data is valid."""
    post = request_vars['post']

    username = post.get('email', None)
    password = post.get('password', None)
    continue_url = post.get('continue_url', None)

    error_string = ""

    if not username or not password:
      error_string = "Empty username or password."
    else:
      account = self._accounts.get(username, None)
      if account is None:
        error_string = "Username '%s' not found." % username
      else:
        valid_password = account.get('password')
        if password != valid_password:
          account.pop_stat('account_login_auth_failed')
          error_string = "Password invalid for username '%s', given '%s', valid '%s'." % (
              username, password, valid_password)
        else:
          account.pop_stat('account_login_auth_ok')

    headers = {}
    if not error_string:
      if not continue_url.endswith('access-token'):
        headers = {}
        headers['Location'] = continue_url
        return self.response(code=303, headers=headers)
      else:
        token_data = account.issue_token()
        email = account.get('email')
        email_string = email
        has_email = '1'
        if not email:
          email_string = ''
          has_email = '0'

        headers['X-Opera-Auth-AccessToken'] = token_data['token']
        headers['X-Opera-Auth-AccessTokenSecret'] = token_data['secret']
        headers['X-Opera-Auth-UserName'] = username
        headers['X-Opera-Auth-UserID'] = account.get('user_id')
        headers['X-Opera-Auth-UserEmail'] = email_string
        headers['X-Opera-Auth-EmailVerified'] = has_email
        return self.response(headers=headers)
    else:
      qs_dict = {}
      qs_dict['error'] = error_string
      qs_dict['continue'] = continue_url
      qs = urllib.urlencode(qs_dict)
      headers['Location'] = '/account/login?%s' % qs
      return self.response(code=303, headers=headers)

  def do_handle_simple_access_token(self, request_vars):
    """Endpoint called by the client with the command line authorization
    and during implicit passphrase recovery after upgrade.

    Expected input:
      Type: POST
      Variables: x_username, x_password and x_consumer_key

    Expected output:
      Success: oauth_token=TOKEN&oauth_token_secret=SECRET
      Error: error=ERROR"""
    result = {}

    post = request_vars['post']
    username = post['x_username']
    password = post['x_password']

    if not username or not password:
      result[
          'error'] = "Both POST vars 'username' and 'password' must exist and be non-empty."
    else:
      account = self._accounts.get(username, None)
      if account is None:
        result['error'] = "Account '%s' not found." % username
      else:
        valid_password = account.get('password')
        if password != valid_password:
          account.pop_stat('simple_token_auth_failed')
          result[
              'error'] = "Password mismatch for user '%s', valid '%s', given '%s'." % (
                  username, valid_password, password)
        else:
          account.pop_stat('simple_token_auth_ok')
          token = account.issue_token()
          result['oauth_token'] = token['token']
          result['oauth_token_secret'] = token['secret']

    return self.response(content=urllib.urlencode(result))

  def do_handle_list_accounts(self, request_vars):
    final_json = {}
    for a in self._accounts:
      final_json[a] = self._accounts[a].data()

    return self.response(to_json=final_json)

  def do_handle_add_account(self, request_vars):
    get_vars = request_vars['get']

    username = get_vars.get('username', None)
    password = get_vars.get('password', None)
    email = get_vars.get('email', '')
    if not username or not password:
      return self.response(
          code=400,
          content=
          "Both 'username' and 'password' GET parameters must be present and non-empty.")

    if self._accounts.get(username):
      return self.response(code=409,
                           content="Username '%s' already exists." % username)

    account = OperaAuthAccount(username=username,
                               password=password,
                               email=email)
    self._accounts[username] = account

    return self.response(
        content="Account '%s' added with password '%s' and email '%s'." % (
            account.get('username'), account.get('password'), account.get(
                'email')))

  def do_handle_remove_account(self, request_vars):
    get_vars = request_vars['get']

    username = get_vars.get('username', None)
    if username is None:
      return self.response(code=400,
                           content="GET parameter 'username' not found")

    if self._accounts.get(username, None) is None:
      return self.response(code=409,
                           content="Username '%s' not found." % username)

    del self._accounts[username]
    return self.response(
        content="Username '%s' removed succesfully." % username)

  def do_handle_change_password(self, request_vars):
    get_vars = request_vars['get']

    username = get_vars.get('username', None)
    password = get_vars.get('password', None)
    if username is None or password is None:
      return self.response(
          code=400,
          content=
          "Both 'username' and 'password' GET parameters must be present and non-empty.")

    if self._accounts.get(username, None) is None:
      return self.response(code=409,
                           content="Account for '%s' not found." % username)

    self._accounts.get(username).change_password(password)
    self._send_trick_invalidation_callback()
    return self.response(content="Password for username='%s' changed to '%s'" %
                         (username, password))

  def do_handle_account_info(self, request_vars):
    get_vars = request_vars['get']

    username = get_vars.get('username', None)
    if username is None:
      return self.response(
          code=400,
          content="The 'username' GET variable is not present.")

    account = self._accounts.get(username, None)
    if account is None:
      return self.response(code=409,
                           content="Username '%s' not found." % username)

    return self.response(to_json=account.stats())

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
