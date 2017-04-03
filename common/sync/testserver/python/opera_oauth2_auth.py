import copy
import json
import opera_auth
import time
import urllib
import urlparse
import uuid
from opera_http import OperaHTTP

VERIFICATION_ERROR_OAUTH2 = -1

USER_EMAIL = 'useremail@testserver.mz'
USER_ID = '1234567'
USER_NAME = 'fake-user-name'

# The encoded scope strings as available in the token request POST data
SYNC_SCOPE = 'https://sync.opera.com'
XMPP_SCOPE = 'https://push.opera.com'
CDN_SCOPE  = 'https://cdnbroker.opera.com'

NETWORK_ERROR_RESPONSE = {}
NETWORK_ERROR_RESPONSE['network_error'] = True
NETWORK_ERROR_RESPONSE['http_status'] = 0
NETWORK_ERROR_RESPONSE['json'] = {}

HTTP_ERROR_RESPONSE = {}
HTTP_ERROR_RESPONSE['network_error'] = False
HTTP_ERROR_RESPONSE['http_status'] = 205
HTTP_ERROR_RESPONSE['json'] = {}

PARSE_ERROR_RESPONSE = {}
PARSE_ERROR_RESPONSE['network_error'] = False
PARSE_ERROR_RESPONSE['http_status'] = 400
PARSE_ERROR_RESPONSE['json'] = {}
PARSE_ERROR_RESPONSE['json']['error'] = 'THIS IS GARBAGE'
PARSE_ERROR_RESPONSE['json']['error_description'] = 'N______ot parseable!!'

REDIRECT_RESPONSE = {}
REDIRECT_RESPONSE['network_error'] = False
REDIRECT_RESPONSE['http_status'] = 303
REDIRECT_RESPONSE['json'] = {}
REDIRECT_RESPONSE['headers'] = {'Location': 'http://opera.com/'}

RETRY_AFTER_RESPONSE = {}
RETRY_AFTER_RESPONSE = {}
RETRY_AFTER_RESPONSE['network_error'] = False
RETRY_AFTER_RESPONSE['http_status'] = 429
RETRY_AFTER_RESPONSE['json'] = {}
RETRY_AFTER_RESPONSE['headers'] = {'Retry-After': 61}

ERROR_RESPONSE_INVALID_REQUEST = {}
ERROR_RESPONSE_INVALID_REQUEST['network_error'] = False
ERROR_RESPONSE_INVALID_REQUEST['http_status'] = 400
ERROR_RESPONSE_INVALID_REQUEST['json'] = {}
ERROR_RESPONSE_INVALID_REQUEST['json']['error'] = 'invalid_request'
ERROR_RESPONSE_INVALID_REQUEST['json']['error_description'] = 'Invalid request error description'

ERROR_RESPONSE_INVALID_GRANT = {}
ERROR_RESPONSE_INVALID_GRANT['network_error'] = False
ERROR_RESPONSE_INVALID_GRANT['http_status'] = 401
ERROR_RESPONSE_INVALID_GRANT['json'] = {}
ERROR_RESPONSE_INVALID_GRANT['json']['error'] = 'invalid_grant'
ERROR_RESPONSE_INVALID_GRANT['json']['error_description'] = 'Invalid grant error description'

ERROR_RESPONSE_INVALID_CLIENT = {}
ERROR_RESPONSE_INVALID_CLIENT['network_error'] = False
ERROR_RESPONSE_INVALID_CLIENT['http_status'] = 401
ERROR_RESPONSE_INVALID_CLIENT['json'] = {}
ERROR_RESPONSE_INVALID_CLIENT['json']['error'] = 'invalid_client'
ERROR_RESPONSE_INVALID_CLIENT['json']['error_description'] = 'Invalid client error description'

ERROR_RESPONSE_INVALID_SCOPE = {}
ERROR_RESPONSE_INVALID_SCOPE['network_error'] = False
ERROR_RESPONSE_INVALID_SCOPE['http_status'] = 401
ERROR_RESPONSE_INVALID_SCOPE['json'] = {}
ERROR_RESPONSE_INVALID_SCOPE['json']['error'] = 'invalid_scope'
ERROR_RESPONSE_INVALID_SCOPE['json']['error_description'] = 'Invalid scope error description'

ACCESS_TOKEN_GRANTED_RESPONSE = {}
ACCESS_TOKEN_GRANTED_RESPONSE['network_error'] = False
ACCESS_TOKEN_GRANTED_RESPONSE['http_status'] = 200
ACCESS_TOKEN_GRANTED_RESPONSE['json'] = {}
ACCESS_TOKEN_GRANTED_RESPONSE['json']['access_token'] = ""
ACCESS_TOKEN_GRANTED_RESPONSE['json']['expires_in'] = 5*60 # Token expiration time in seconds
ACCESS_TOKEN_GRANTED_RESPONSE['json']['scope'] = ""
ACCESS_TOKEN_GRANTED_RESPONSE['json']['token_type'] = 'Bearer'
ACCESS_TOKEN_GRANTED_RESPONSE['json']['user_id'] = USER_ID

def refresh_token_granted_response_for_scope(scope):
  response = copy.deepcopy(ACCESS_TOKEN_GRANTED_RESPONSE)
  response['json']['scope'] = scope
  response['json']['refresh_token'] = str(uuid.uuid4())
  response['json']['access_token'] = str(uuid.uuid4())
  return response

def access_token_granted_response_for_scope(scope):
  response = copy.deepcopy(ACCESS_TOKEN_GRANTED_RESPONSE)
  response['json']['scope'] = scope
  response['json']['access_token'] = str(uuid.uuid4())
  return response

RESPONSE_VALUES = {
    'SYNC_COMMAND_RESPONSE': [
        'SCR_OK',
        'SCR_AUTH_ERROR',
        'SCR_TRANSIENT',
        'SCR_NETWORK',
        'SCR_HTTP_500'
    ],
    'XMPP_CONNECT_RESPONSE': [
        'XCR_OK',
        'XCR_AUTH_ERROR',
        'XCR_TRANSIENT',
        'XCR_NETWORK',
    ],
    'FETCH_REFRESH_TOKEN_RESPONSE': [
        'FRTR_OK',
        'FRTR_INVALID_CLIENT',
        'FRTR_INVALID_GRANT',
        'FRTR_INVALID_REQUEST',
        'FRTR_INVALID_SCOPE',
        'FRTR_HTTP_ERROR',
        'FRTR_NETWORK_ERROR',
        'FRTR_PARSE_ERROR',
        'FRTR_REDIRECT',
        'FRTR_RETRY_AFTER'
    ],
    'FETCH_ACCESS_TOKEN_RESPONSE_SYNC': [
        'FATRS_OK',
        'FATRS_INVALID_CLIENT',
        'FATRS_INVALID_GRANT',
        'FATRS_INVALID_REQUEST',
        'FATRS_INVALID_SCOPE',
        'FATRS_HTTP_ERROR',
        'FATRS_NETWORK_ERROR',
        'FATRS_PARSE_ERROR',
        'FATRS_REDIRECT',
        'FATRS_RETRY_AFTER'
    ],
    'FETCH_ACCESS_TOKEN_RESPONSE_XMPP': [
        'FATRX_OK',
        'FATRX_INVALID_CLIENT',
        'FATRX_INVALID_GRANT',
        'FATRX_INVALID_REQUEST',
        'FATRX_INVALID_SCOPE',
        'FATRX_HTTP_ERROR',
        'FATRX_NETWORK_ERROR',
        'FATRX_PARSE_ERROR',
        'FATRX_REDIRECT',
        'FATRX_RETRY_AFTER'
    ],
    'FETCH_ACCESS_TOKEN_RESPONSE_CDN': [
        'FATRC_OK',
        'FATRC_INVALID_CLIENT',
        'FATRC_INVALID_GRANT',
        'FATRC_INVALID_REQUEST',
        'FATRC_INVALID_SCOPE',
        'FATRC_HTTP_ERROR',
        'FATRC_NETWORK_ERROR',
        'FATRC_PARSE_ERROR',
        'FATRC_REDIRECT',
        'FATRC_RETRY_AFTER'
    ],
    'REVOKE_TOKEN_RESPONSE': [
        'RTR_OK',
        'RTR_INVALID_CLIENT',
        'RTR_INVALID_GRANT',
        'RTR_INVALID_REQUEST',
        'RTR_INVALID_SCOPE',
        'RTR_HTTP_ERROR',
        'RTR_NETWORK_ERROR',
        'RTR_PARSE_ERROR',
        'RTR_REDIRECT',
        'RTR_RETRY_AFTER'
    ],
    'FETCH_MIGRATION_TOKEN_RESPONSE': [
        'FMTR_OK',
        'FMTR_INVALID_CLIENT',
        'FMTR_INVALID_GRANT',
        'FMTR_INVALID_REQUEST',
        'FMTR_INVALID_SCOPE',
        'FMTR_HTTP_ERROR',
        'FMTR_NETWORK_ERROR',
        'FMTR_PARSE_ERROR',
        'FMTR_REDIRECT',
        'FMTR_RETRY_AFTER'
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
    'OAUTH1_TOKEN_RENEWAL_RESPONSE': [
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
    return "[{}] {} {} {}".format(self.event_timestamp_, self.event_type_,
                              str(self.event_details_), self.event_response_)

class OperaOAuth2Auth:
  def __init__(self):
    self.response_config_ = {}
    self.setup_initial_responses()
    self.events_ = []

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

  def append_event(self, event_type, event_details, event_response):
    new_event = Event(event_type, event_details, event_response)
    self.events_.append(new_event)

  def clear_events(self):
    self.events_ = []

  def verify_token(self, parsed_auth_header, client_addr):
    origin = parsed_auth_header.get('origin', None)
    oauth2_token = parsed_auth_header.get('oauth2_token', None)

    if type(client_addr) is str:
      client_addr = client_addr.split(':')

    if not origin or not oauth2_token:
      raise Exception("Empty origin or oauth2_token, fatal.")

    if origin == 'sync':
      response = self.get_response('SYNC_COMMAND_RESPONSE')
      self.append_event(
          'SYNC_AUTHORIZE', {
              'client_ip': client_addr[0],
              'oauth2_token': oauth2_token
          }, response)

      if response not in RESPONSE_VALUES['SYNC_COMMAND_RESPONSE']:
        raise Exception("Unknown SYNC_COMMAND_RESPONSE: '%s'" % response)


        # 'jid': USER_EMAIL'jid': USER_EMAIL'jid': USER_EMAIL
        # 'jid': USER_EMAIL

      if response == 'SCR_OK':
        return {'verification_error': opera_auth.VERIFICATION_ERROR_NO_ERROR}
      elif response == 'SCR_AUTH_ERROR':
        return {'verification_error': VERIFICATION_ERROR_OAUTH2}
      elif response == 'SCR_TRANSIENT':
        return {'network_error': opera_auth.NETWORK_ERROR_SOFT}
      elif response == 'SCR_NETWORK':
        return {'network_error': opera_auth.NETWORK_ERROR_HARD}
      elif response == 'SCR_HTTP_500':
        return {'http_error': 500}
    elif origin == 'xmpp':
      response = self.get_response('XMPP_CONNECT_RESPONSE')
      self.append_event(
          'XMPP_AUTHORIZE', {
              'client_ip': client_addr[0],
              'oauth2_token': oauth2_token
          }, response)

      if response not in RESPONSE_VALUES['XMPP_CONNECT_RESPONSE']:
        raise Exception("Unknown XMPP_CONNECT_RESPONSE: '%s'" % response)

      if response == 'XCR_OK':
        return {
            'verification_error': opera_auth.VERIFICATION_ERROR_NO_ERROR,
            'jid': USER_EMAIL
        }
      elif response == 'XCR_AUTH_ERROR':
        return {'verification_error': VERIFICATION_ERROR_OAUTH2}
      elif response == 'XCR_TRANSIENT':
        return {'network_error': opera_auth.NETWORK_ERROR_SOFT}
      elif response == 'XCR_NETWORK':
        return {'network_error': opera_auth.NETWORK_ERROR_HARD}

    else:
      raise Exception("Unknown origin, fatal.")

  def on_xmpp_connected(self, client_addr):
    self.append_event('XMPP_CONNECT', {'client_ip': client_addr[0]}, 'NONE')

  def on_xmpp_disconnected(self, client_addr):
    self.append_event('XMPP_DISCONNECT', {'client_ip': client_addr[0]}, 'NONE')

  def on_sync_command(self, command_info_dict):
    self.append_event('SYNC_COMMAND', command_info_dict, 'NONE')

  def serve_file_response(self, filename, replacements={}):
    filepath = OperaHTTP.AsDataDirPath(filename)
    content = open(filepath, 'r').read()
    for key in replacements:
      findstr = '[%s]' % key
      replacestr = str(replacements[key])
      content = content.replace(findstr, replacestr)
    return OperaHTTP.response(content_type='text/html', content=content)

  def serve_redirect_response(self, url):
    headers = {}
    headers['Location'] = url
    return OperaHTTP.response(code=302, headers=headers)

  def handle_request(self, request_vars):
    path = request_vars['path']
    replaced_path = path.replace('-', '_')
    replaced_path = replaced_path.replace('/', '_')
    replaced_path = replaced_path.rstrip('_')
    handler_name = 'handle_' + replaced_path
    handler_ref = getattr(self, handler_name, None)
    if handler_ref is None:
      return OperaHTTP.response(code=404, content='Handler {} not found'.format(handler_name))

    return handler_ref(request_vars)

  def handle__account_login(self, request_vars):
    get = request_vars['get']
    post = request_vars['post']

    was_posted = False
    if post:
      was_posted = True

    if not was_posted:
      r = {}
      self.append_event(
          'WLR_STAGE_1', {
              'POST': post,
              'GET': get
          }, 'NONE')

      return OperaHTTP.response(code=200, file_name='oauth2_login.html', replacements={'HIDDEN_NEXT': urllib.urlencode(r)})
    else:
      headers = {}
      headers['Location'] = '/account/login/success'
      headers['X-Opera-Auth-AuthToken'] = str(uuid.uuid4())
      headers['X-Opera-Auth-UserId'] = '1234'
      headers['X-Opera-Auth-UserEmail'] = 'test@test.com'
      headers['X-Opera-Auth-UserName'] = 'testusername'
      headers['X-Opera-Auth-EmailVerified'] = '1'

      self.append_event(
          'WLR_STAGE_2', {
              'POST': post,
              'GET': get
          }, 'NONE')

      response = OperaHTTP.response(code=200, headers=headers)
      return response

  def handle__oauth2_v1_token(self, request_vars):
    post = request_vars['post']

    grant_type = post.get('grant_type', None)
    client_secret = post.get('client_secret', None)
    client_id = post.get('client_id', None)
    refresh_token = post.get('refresh_token', None)
    scope = post.get('scope', None)

    json_response = None

    if grant_type == 'refresh_token':
      if scope == SYNC_SCOPE:
        response = self.get_response('FETCH_ACCESS_TOKEN_RESPONSE_SYNC')

        if response not in RESPONSE_VALUES['FETCH_ACCESS_TOKEN_RESPONSE_SYNC']:
          raise Exception("Unknown FETCH_ACCESS_TOKEN_RESPONSE_SYNC: '%s'" % response)

        if response == 'FATRS_OK':
          json_response = access_token_granted_response_for_scope(scope)
        elif response == 'FATRS_INVALID_CLIENT':
          json_response = ERROR_RESPONSE_INVALID_CLIENT
        elif response == 'FATRS_INVALID_GRANT':
          json_response = ERROR_RESPONSE_INVALID_GRANT
        elif response == 'FATRS_INVALID_REQUEST':
          json_response = ERROR_RESPONSE_INVALID_REQUEST
        elif response == 'FATRS_INVALID_SCOPE':
          json_response = ERROR_RESPONSE_INVALID_SCOPE
        elif response == 'FATRS_HTTP_ERROR':
          json_response = HTTP_ERROR_RESPONSE
        elif response == 'FATRS_NETWORK_ERROR':
          json_response = NETWORK_ERROR_RESPONSE
        elif response == 'FATRS_PARSE_ERROR':
          json_response = PARSE_ERROR_RESPONSE
        elif response == 'FATRS_REDIRECT':
          json_response = REDIRECT_RESPONSE
        elif response == 'FATRS_RETRY_AFTER':
          json_response = RETRY_AFTER_RESPONSE

        self.append_event(
            'FETCH_ACCESS_TOKEN_SYNC', {
                'POST': post,
                'RESPONSE': json_response
            }, response)

      elif scope == XMPP_SCOPE:
        response = self.get_response('FETCH_ACCESS_TOKEN_RESPONSE_XMPP')

        if response not in RESPONSE_VALUES['FETCH_ACCESS_TOKEN_RESPONSE_XMPP']:
          raise Exception("Unknown FETCH_ACCESS_TOKEN_RESPONSE_XMPP: '%s'" % response)

        if response == 'FATRX_OK':
          json_response = access_token_granted_response_for_scope(scope)
        elif response == 'FATRX_INVALID_CLIENT':
          json_response = ERROR_RESPONSE_INVALID_CLIENT
        elif response == 'FATRX_INVALID_GRANT':
          json_response = ERROR_RESPONSE_INVALID_GRANT
        elif response == 'FATRX_INVALID_REQUEST':
          json_response = ERROR_RESPONSE_INVALID_REQUEST
        elif response == 'FATRX_INVALID_SCOPE':
          json_response = ERROR_RESPONSE_INVALID_SCOPE
        elif response == 'FATRX_HTTP_ERROR':
          json_response = HTTP_ERROR_RESPONSE
        elif response == 'FATRX_NETWORK_ERROR':
          json_response = NETWORK_ERROR_RESPONSE
        elif response == 'FATRX_PARSE_ERROR':
          json_response = PARSE_ERROR_RESPONSE
        elif response == 'FATRX_REDIRECT':
          json_response = REDIRECT_RESPONSE
        elif response == 'FATRX_RETRY_AFTER':
          json_response = RETRY_AFTER_RESPONSE

        self.append_event(
            'FETCH_ACCESS_TOKEN_XMPP', {
                'POST': post,
                'RESPONSE': json_response
            }, response)

      elif scope == CDN_SCOPE:
        response = self.get_response('FETCH_ACCESS_TOKEN_RESPONSE_CDN')

        if response not in RESPONSE_VALUES['FETCH_ACCESS_TOKEN_RESPONSE_CDN']:
          raise Exception("Unknown FETCH_ACCESS_TOKEN_RESPONSE_CDN: '%s'" % response)

        if response == 'FATRC_OK':
          json_response = access_token_granted_response_for_scope(scope)
        elif response == 'FATRC_INVALID_CLIENT':
          json_response = ERROR_RESPONSE_INVALID_CLIENT
        elif response == 'FATRC_INVALID_GRANT':
          json_response = ERROR_RESPONSE_INVALID_GRANT
        elif response == 'FATRC_INVALID_REQUEST':
          json_response = ERROR_RESPONSE_INVALID_REQUEST
        elif response == 'FATRC_INVALID_SCOPE':
          json_response = ERROR_RESPONSE_INVALID_SCOPE
        elif response == 'FATRC_HTTP_ERROR':
          json_response = HTTP_ERROR_RESPONSE
        elif response == 'FATRC_NETWORK_ERROR':
          json_response = NETWORK_ERROR_RESPONSE
        elif response == 'FATRC_PARSE_ERROR':
          json_response = PARSE_ERROR_RESPONSE
        elif response == 'FATRC_REDIRECT':
          json_response = REDIRECT_RESPONSE
        elif response == 'FATRC_RETRY_AFTER':
          json_response = RETRY_AFTER_RESPONSE

        self.append_event(
            'FETCH_ACCESS_TOKEN_CDN', {
                'POST': post,
              'RESPONSE': json_response
            }, response)

      else:
        raise Exception("Unknown scope '{}'".format(scope))

    elif grant_type == 'sso' or grant_type == 'auth_token':
      response = self.get_response('FETCH_REFRESH_TOKEN_RESPONSE')

      if response not in RESPONSE_VALUES['FETCH_REFRESH_TOKEN_RESPONSE']:
        raise Exception("Unknown FETCH_REFRESH_TOKEN_RESPONSE: '%s'" % response)

      if response == 'FRTR_OK':
        json_response = refresh_token_granted_response_for_scope(scope)
      elif response == 'FRTR_INVALID_CLIENT':
        json_response = ERROR_RESPONSE_INVALID_CLIENT
      elif response == 'FRTR_INVALID_GRANT':
        json_response = ERROR_RESPONSE_INVALID_GRANT
      elif response == 'FRTR_INVALID_REQUEST':
        json_response = ERROR_RESPONSE_INVALID_REQUEST
      elif response == 'FRTR_INVALID_SCOPE':
        json_response = ERROR_RESPONSE_INVALID_SCOPE
      elif response == 'FRTR_HTTP_ERROR':
        json_response = HTTP_ERROR_RESPONSE
      elif response == 'FRTR_NETWORK_ERROR':
        json_response = NETWORK_ERROR_RESPONSE
      elif response == 'FRTR_PARSE_ERROR':
        json_response = PARSE_ERROR_RESPONSE
      elif response == 'FRTR_REDIRECT':
        json_response = REDIRECT_RESPONSE
      elif response == 'FRTR_RETRY_AFTER':
        json_response = RETRY_AFTER_RESPONSE

      self.append_event(
          'FETCH_REFRESH_TOKEN', {
              'POST': post,
              'RESPONSE': json_response
          }, response)

    elif grant_type == 'oauth1_token':
      response = self.get_response('FETCH_MIGRATION_TOKEN_RESPONSE')

      if response not in RESPONSE_VALUES['FETCH_MIGRATION_TOKEN_RESPONSE']:
        raise Exception("Unknown FETCH_MIGRATION_TOKEN_RESPONSE: '%s'" % response)

      if response == 'FMTR_OK':
        json_response = refresh_token_granted_response_for_scope(scope)
      elif response == 'FMTR_INVALID_CLIENT':
        json_response = ERROR_RESPONSE_INVALID_CLIENT
      elif response == 'FMTR_INVALID_GRANT':
        json_response = ERROR_RESPONSE_INVALID_GRANT
      elif response == 'FMTR_INVALID_REQUEST':
        json_response = ERROR_RESPONSE_INVALID_REQUEST
      elif response == 'FMTR_INVALID_SCOPE':
        json_response = ERROR_RESPONSE_INVALID_SCOPE
      elif response == 'FMTR_HTTP_ERROR':
        json_response = HTTP_ERROR_RESPONSE
      elif response == 'FMTR_NETWORK_ERROR':
        json_response = NETWORK_ERROR_RESPONSE
      elif response == 'FMTR_PARSE_ERROR':
        json_response = PARSE_ERROR_RESPONSE
      elif response == 'FMTR_REDIRECT':
        json_response = REDIRECT_RESPONSE
      elif response == 'FMTR_RETRY_AFTER':
        json_response = RETRY_AFTER_RESPONSE

      self.append_event(
          'FETCH_MIGRATION_TOKEN', {
              'POST': post,
              'RESPONSE': json_response
          }, response)

    else:
      raise Exception("Unknown grant type '{}'".format(grant_type))
      json_response = ERROR_RESPONSE_INVALID_REQUEST

    assert response, "No response for handle__oauth2_v1_token"
    return OperaHTTP.response(code=json_response['http_status'],\
                              to_json=json_response['json'],\
                              network_error=json_response['network_error'],\
                              headers=json_response.get('headers', []))

  def handle__oauth2_v1_revoketoken(self, request_vars):
    post = request_vars['post']
    response = self.get_response('REVOKE_TOKEN_RESPONSE')

    if response not in RESPONSE_VALUES['REVOKE_TOKEN_RESPONSE']:
      raise Exception("Unknown REVOKE_TOKEN_RESPONSE: '%s'" % response)

    json_response = {}

    if response == 'RTR_OK':
      json_response['json'] = None
      json_response['content'] = ''
      json_response['http_status'] = 200
      json_response['network_error'] = False
    elif response == 'RTR_INVALID_CLIENT':
      json_response = ERROR_RESPONSE_INVALID_CLIENT
    elif response == 'RTR_INVALID_GRANT':
      json_response = ERROR_RESPONSE_INVALID_GRANT
    elif response == 'RTR_INVALID_REQUEST':
      json_response = ERROR_RESPONSE_INVALID_REQUEST
    elif response == 'RTR_INVALID_SCOPE':
      json_response = ERROR_RESPONSE_INVALID_SCOPE
    elif response == 'RTR_HTTP_ERROR':
      json_response = HTTP_ERROR_RESPONSE
    elif response == 'RTR_NETWORK_ERROR':
      json_response = NETWORK_ERROR_RESPONSE
    elif response == 'RTR_PARSE_ERROR':
      json_response = PARSE_ERROR_RESPONSE
    elif response == 'RTR_REDIRECT':
      json_response = REDIRECT_RESPONSE
    elif response == 'RTR_RETRY_AFTER':
      json_response = RETRY_AFTER_RESPONSE


    self.append_event(
        'REVOKE_TOKEN_RESPONSE', {
            'POST': post,
            'RESPONSE': json_response
        }, response)

    content = json_response.get('content', '')

    return OperaHTTP.response(code=json_response['http_status'],\
                              to_json=json_response['json'],\
                              network_error=json_response['network_error'],\
                              content=content,
                              headers=json_response.get('headers', {}))

  def handle__account_access_token_renewal(self, request_vars):

    def _generate_renewal_response(old_token, token_ok, secret_ok):
      if not old_token:
        raise Exception("old_token is empty")

      new_token = ''
      if token_ok:
        new_token = self.generate_token(old_token)
      new_secret = ''
      if secret_ok:
        new_secret = new_token + '_secret'
      json_dict = {
          'auth_token': new_token,
          'auth_token_secret': new_secret,
          'userID': USER_ID,
          'userName': USER_NAME,
          'userEmail': USER_EMAIL,
      }
      return json_dict

    get = request_vars['get']
    response = self.get_response('OAUTH1_TOKEN_RENEWAL_RESPONSE')

    if response not in RESPONSE_VALUES['OAUTH1_TOKEN_RENEWAL_RESPONSE']:
      raise Exception("Unknown OAUTH1_TOKEN_RENEWAL_RESPONSE: '%s'" % response)

    json_response = {}

    if response == 'TRR_OK':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = _generate_renewal_response(get.get('old_token', None), True, True)
    elif response == 'TRR_AUTH_420':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = {'err_code': 420, 'err_msg': 'Request not authorized'}
    elif response == 'TRR_AUTH_421':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = {'err_code': 421, 'err_msg': 'Bad request'}
    elif response == 'TRR_AUTH_422':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = {'err_code': 422, 'err_msg': 'Opera user not found'}
    elif response == 'TRR_AUTH_424':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = {'err_code': 424, 'err_msg': 'Access token not found'}
    elif response == 'TRR_AUTH_425':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = {'err_code': 425, 'err_msg': 'Invalid Opera token'}
    elif response == 'TRR_AUTH_426':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = {'err_code': 426, 'err_msg': 'Could not generate Opera token'}
    elif response == 'TRR_AUTH_428':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = {'err_code': 428, 'err_msg': 'Access token is not expired'}
    elif response == 'TRR_PARSE':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = None
      json_response['content'] = 'A Parse Error'
    elif response == 'TRR_NETWORK':
      json_response['http_status'] = -1
      json_response['network_error'] = True
      json_response['json'] = {}
    elif response == 'TRR_HTTP_500':
      json_response['http_status'] = 500
      json_response['network_error'] = False
      json_response['json'] = {}
    elif response == 'TRR_EMPTY_TOKEN':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = _generate_renewal_response(get.get('old_token', None), False, True)
    elif response == 'TRR_EMPTY_SECRET':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = _generate_renewal_response(get.get('old_token', None), True, False)
    elif response == 'TRR_EMPTY_BOTH':
      json_response['http_status'] = 200
      json_response['network_error'] = False
      json_response['json'] = _generate_renewal_response(get.get('old_token', None), False, False)

    self.append_event(
        'OAUTH1_TOKEN_RENEWAL_RESPONSE', {
            'GET': get,
            'RESPONSE': json_response
        }, response)

    content = json_response.get('content', '')

    return OperaHTTP.response(code=json_response['http_status'],\
                              to_json=json_response['json'],\
                              network_error=json_response['network_error'],\
                              content=content,
                              headers=json_response.get('headers', {}))


  def handle__oauth2_auth_response_control(self, request_vars):
    replacements = {}
    replacements['RESPONSES_JSON'] = json.dumps(RESPONSE_VALUES)
    replacements['CURRENT_RESPONSES'] = self.response_config_

    return self.serve_file_response('oauth2_auth_control.html', replacements)

  def handle__oauth2_auth_change_response(self, request_vars):
    key = request_vars['get'].get('key')
    value = request_vars['get'].get('value')

    self.set_response(key, value)
    return self.serve_redirect_response('/oauth2_auth/response_control')

  def handle__oauth2_auth_event_log(self, request_vars):
    return self.serve_file_response('oauth2_auth_event_log.html', {})

  def handle__oauth2_auth_fetch_event_log(self, request_vars):
    content = ""
    for e in self.events_:
      content = content + e.to_string() + "\n"

    return OperaHTTP.response(content=content)

  def handle__oauth2_auth_reset_event_log(self, req_vars):
    self.clear_events()
    return OperaHTTP.response(content="Auth event log cleared OK")
