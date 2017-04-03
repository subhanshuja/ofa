#!/usr/bin/env python
# Copyright (C) 2015 Opera Software ASA.  All rights reserved.
#
# This file is an original work developed by Opera Software ASA
""" This file implements the Python part of the TokenSyncTest event handling
framework.
The framework allows controlling the requests/responses to/from the sync mechanism
implemented in Opera in such a way that it is possible to have fixed and repeatable
scenarios related to behaviour of the auth/sync/syncpush services, including the
network conditions and communication between sync/syncpush and auth.

The TokenSyncTest framework needs to be set up for each test case explicitly
with expectations for client requests and corresponding responses. Setting up
the framework for a test case is referred to as creating an auth Script for the
test case.
An auth script consists of a series of expected STEPS introduced by a C++ Expect()
and a series of allowed OPTIONALS introduced by C++ Allow() calls. While the client
is running, each request to auth/sync/syncpush services is intercepted by the Python
framework and fed as an exectution EVENT to the auth Script.
The most fundamental construcing block of the SCRIPT is an EXPECTATION. An EXPECTATION
consists of an EVENT, ACTION and TRIGGERS and has an ID and TYPE. A STEP may consist
of several EXPECTATIONS, an OPTIONAL may include only one EXPECTATION. A STEP that
combines more than one EXPECTATION is referred to as an INTERLEAVE.

Each incoming EVENT has to match the current STEP or the list of currently allowed
OPTIONALS. In case an EVENT fails to match either, execution of the SCRIPT fails
and no further processing is done. Matching a STEP means matching any of its unmatched
EXPECTATIONS. Matching an OPTIONAL means matching its EXPECTATION, without caring
whether the EXPECTATION has already been met.

Matching an EVENT causes performing the attached ACTION, either immediately or in a
deferred manner. An actions is performed in a deferred manner if it depends on
at least one TRIGGER (compare DelayResponseUntilTriggered()), in which case the action
is performed the moment the last untriggerred TRIGGER fires.

A TRIGGER may fire as a result of an EXPECTATION becoming matched (compare Triggers())
or as a result of a direct call to the SendTrigger() method. Note that calling the
SendTrigger() method only makes sense as a result of an async test event, most probably
one coming from an observer hooked on the sync/xmpp client mechanisms.
In case several triggers are passed to the Trigger() call, they fire in the order
in which they were passed in the call.

Matching an incoming EVENT to an event found in the SCRIPT includes going through all the
fields found in the EVENT found in the script and matching against the corresponding
fields found in the incoming EVENT. Note that this makes partial matches possible, e.g.
since there is no token secret set up within an expectation of a verification_requested
type, the value of the token secret sent by the client is ignored.

The last C++ call in a TokenSyncTest test case should be a call to the SCRIPT verification
endpoint, i.e.:
  ASSERT_TRUE(AuthOKState()) << AuthOKMessage();

In case the SCRIPT execution fails along the way, the AuthOKState() method will return
false and the AuthOKMessage() method will return a description of the failures that
occurred during the execution.

Calling the AuthOKMessage() method will also cause outputting stats for OPTIONALS
utilization during the SCRIPT execution. In case there were no optionals set up in
the SCRIPT, the message will state:

AUTH: No optionals used in test.

Otherwise, full stats will be shown:

AUTH: ******** Optionals match stats ********
AUTH: Optional with ID = 0 matched 0 times.
AUTH: Optional with ID = 1 matched 0 times.
AUTH: Optional with ID = 2 matched 0 times.
AUTH: Optional with ID = 5 matched 0 times.
AUTH: Optional with ID = 6 matched 0 times.
AUTH: Optional with ID = 11 matched 5 times.
AUTH: ***************************************

As communication between the C++ and Python is done using the JSON format, it is easiest
to show the structures described above using JSON.

Example of a a single EXPECTATION:
{
  "id":1,
  "type":"verification_requested"
  "event":{
    "oauth_token":"TokenSyncTest.TokenOkayOnStartupXmppFirst.5B4C0B4C-A5C4-4533-A238-682633A05BA2",
    "origin":"xmpp"
  },
  "action":{
    "delay_until":[
      "xmpp_verified",
      "sync_verified"
    ],
    "jid":"user@user.com",
    "network_error":0,
    "verification_error":0
  },
  "triggers":[
    "xmpp_verified"
  ],
}

Explanation:
ID: A unique ID of a C++ Expectation::IDType type. Note that the ID is unique across all the
    expectations fed to the SCIRPT from a C++ test case.
TYPE: Type of the expectation. The type determines contents the EVENT and ACTION format.

Current list of handled expectation types:

1. verification_requested
  Designates the auth header verification request sent by
  the sync/syncpush services, as seen by the auth service. Note that even thou the
  format of communication beteen the client and remote is absolutely different in case
  of sync and syncpush, the verification_requested expectation looks exactly the same
  for both the the auth service perspective.

1.1. verification_requested.event
  An event describing an auth header verification request
  sent to the auth service. Contents of the event is the contents of the auth header
  sent by the client.
  Note that each authorization reuqest sent by the client is signed with an OAuth 1.0 auth
  HTTP header that has the following format (note that the header does not contain any
  new line characters in reality):

  Authorization: OAuth realm="127.0.0.1", oauth_consumer_key="LDKgAObtMIV1eVp1Jb2b0ZlqK1TUDkk3",
    oauth_nonce="QKlMeQuHQL_MlBj", oauth_signature="ngsz%2FiI%2B%2FXFUlK7silG05xcSTaI%3D",
    oauth_signature_method="HMAC-SHA1", oauth_timestamp="1422613403",
    oauth_token="TokenSyncTest.TokenOkayOnStartupXmppFirst.D40D6642-36ED-49B8-BDE3-DDB62BBC4A71",
    oauth_version="1.0"

  For each incoming authorization header verification request the header is parsed into a dictionary
  containing all the key=value pairs found there. Additionally the dictionary is extended with an
  "origin" key that is assigned a value depending on which service made the verification reuqest, i.e.
  "xmpp" or "sync".

1.2. verification_requested.action
  Describes the auth service response for the given event. The response includes the following keys:

  network_error: Designates how the request is served from the network point of view. Possible values:
    0: No network error
    1: Network error, i.e. socket closed before response was sent (NOT FULLY IMPLEMENTED/TESTED)
    2: Transient error response sent back by the sync/syncpush service (NOT FULLY IMPLEMENTED/TESTED)
    Note that the transient error response is not really a network problem on the client<->sync/syncpush
    connection but rather a network problem on the sync/syncpush<->auth connection, that is eventually
    signalled to the client by a corresponding transient error response to the verificatino request.
    The response format differs completely between the sync and syncpush services, i.e. at the time
    of writing this it is a HTTP 200 response with a ClientToSeverReponse protobuf message having the
    error() field set properly for the sync service and a <temporary-auth-failure/> XML element sent
    back by the XMPP syncpush service.

  verification_error: Designates the auth header verification request response as sent by the auth_error
    service. Possible values include 0 for success or an auth verification problem code for a failure.
    Auth verification failure codes are described by the opera::VerificationError enum.

  jid: JID stands for Jabber ID here and it is meant to be the auth user ID as resolved basing on the
    authorization header sent by the client. Note that the real auth service uses a different protocol
    and sends back a numeric user ID, but we don't test auth<->sync/syncpush communication here and thus
    we can allow the simplification.

  NOTE: The response does not include a HTTP code since the request is not made by the client in the
    reality, but rather by the sync/syncpush services. We don't care about implementation details here
    and a HTTP code other than 200 is such a detail that is seen as a transient error by the client.

2. clock_validation_requested
  An event describing the clock validation request that should be performed from the client to the
  auth service in case there is a problem with the timestamp sent within the verification header.

2.1. clock_validation_requested.event
  Describes the clock validation request as sent by the client. Contains the following information:
  localtime: The local time value in UNIX format that is used by the auth service to send back a
    time skew offset.

2.2. clock_validation_requested.action
  network_error: 0 for no error, 1 for socket closed

  http_code: The HTTP response code sent back for the client request from the auth service.

  code: The code, diff, message and status fields constitue a clock validation response as sent by
    the auth service. Compare auth server documentation for more details.
  diff: See 'code' above.
  message: See 'code' above.
  status: See 'code' above.

3. token_renewal_requested
  The token renewal request as sent by the client. Compare the auth server documentation for more
  information regarding the request/response.

3.1. token_renewal_requested.event
  Contains all the data sent within the token renewal request in the GET request, i.e. with the current
  implementation the data contains: old_token, consumer_key, service, signature.

3.2. token_renewal_requested.action
  Token renewal response contains the following information: userName, auth_token, userID,
  auth_token_secret, userEmail. Note that the naming convention changes from camel case to underscore
  come from the auth service implementation.

4. xmpp_authorization_finished
  This event is not sent by the client in real conditions, it is an artificial test-only event that
  designates the moment in which the XMPP server authorizes the client properly or refuses connection.

4.1. xmpp_authorization_finished.event
  No parameters.

4.2. xmpp_authorization_finished.action
  In case the action contains a "do_disconnect" key, the server will disconnect the socket
  immediately after sending the initial invalidation.

5. xmpp_initialized
  This event is not sent by the client but rather by the XMPP server. Designates the moment the
  client sends the initialization message.

The Opera's OAuth 1.0 - based authentication API. The API's documentation may
be found at:
https://wiki.oslo.osa/developerwiki/Auth.opera.com/OAuth
https://wiki.oslo.osa/developerwiki/Auth.opera.com/WebUI/AccessToken
https://wiki.oslo.osa/developerwiki/Auth.opera.com/ValidationAPI
"""

import json
import urllib
import urlparse

AUTH_PREFIX = "/opera_auth"

ACCOUNT_ACCESSTOKEN_RENEWAL = "account/access-token/renewal"
ADD_EXPECTATION = "add_expectation"
CHECK_STATE = "check_state"
CLIENT_TRIGGER_FIRED = "client_trigger_fired"
LIST_STEPS = "list_steps"
POST_ALLOW = "post_allow"
POST_DISALLOW = "post_disallow"
POST_STEP = "post_step"
SERVICE_VALIDATION_CLOCK = "service/validation/clock"
NOTIFY_SCRIPT_FINISHED = "notify_script_finished"
"""
Hard network error means an immediate failure, as in socket error.
Soft network error means that the service returns the transient error
response.
Implementation depends on which service is used.
"""
NETWORK_ERROR_HARD = 1
NETWORK_ERROR_SOFT = 2
"""
Value found in the response dict, means "no error - verification passed".
"""
VERIFICATION_ERROR_NO_ERROR = 0

OPERA_FLAGS = None

AUTH_ERROR_OK = 0
AUTH_ERROR_TEMPORARY = 1
AUTH_ERROR_INVALID_TOKEN = 2
AUTH_ERROR_EXPIRED_TOKEN = 3
AUTH_ERROR_INVALID_CONSUMER_APPLICATION = 4
AUTH_ERROR_TOKEN_KEY_MISMATCH = 5
AUTH_ERROR_BAD_OAUTH_REQUEST = 6
AUTH_ERROR_TIMESTAMP_INCORRECT = 7
AUTH_ERROR_INVALID_TIMESTAMP_NONCE = 8

LOCAL_ERROR_TO_AUTH_ERROR = {
    AUTH_ERROR_OK: 0,
    AUTH_ERROR_INVALID_TOKEN: 400,
    AUTH_ERROR_EXPIRED_TOKEN: 401,
    AUTH_ERROR_INVALID_CONSUMER_APPLICATION: 402,
    AUTH_ERROR_TOKEN_KEY_MISMATCH: 403,
    AUTH_ERROR_BAD_OAUTH_REQUEST: 404,
    AUTH_ERROR_TIMESTAMP_INCORRECT: 405,
    AUTH_ERROR_INVALID_TIMESTAMP_NONCE: 406,
}

AUTH_ERROR_TO_LOCAL_ERROR = {v: k
                             for k, v in LOCAL_ERROR_TO_AUTH_ERROR.iteritems()}


def LocalAuthErrorToAuthOperaComErrCode(auth_error):
  return LOCAL_ERROR_TO_AUTH_ERROR.get(auth_error, -1)


def OperaComErrCodeToLocalAuthError(opera_error):
  return AUTH_ERROR_TO_LOCAL_ERROR.get(int(opera_error), -1)


class UniquenessTracker:

  def __init__(self):
    self.values = {}

  def start(self, tracked_type):
    if self.values.get(tracked_type, None) is not None:
      Log(0, "Internal error: Already tracking type '%s'" % tracked_type)
      return

    self.values[tracked_type] = []

  def is_unique(self, tracked_type, value):
    if self.values.get(tracked_type, None) == None:
      Log(0, "Internal error, type '%s' is not tracked." % tracked_type)
      return

    try:
      self.values[tracked_type].index(value)
      return (False, "Value '%s' for type '%s' seen at least once already." %
              (value, tracked_type))
    except ValueError:
      self.values[tracked_type].append(value)
      return (True, "")


class AuthScript:
  # AuthScript is the class that holds the execution steps and optionals
  # set up via calls to Expect() and Allow()/Disallow().
  def __init__(self):
    self._steps = []
    self._fired_triggers = []

    self._current_step_index = 0
    self._failure_messages = []

    # This is the currently allowed optionals list.
    # The list is dynamically updated after each step passes.
    self._currently_allowed_optionals = {}
    self._optionals_match_stats = {}

    self._uniqueness_tracker = UniquenessTracker()
    self._uniqueness_tracker.start('nonce')

    self._script_finished_handler = None

  def ListSteps(self):
    Log(0, "**** OPTIONALS ALLOWED FROM START ****")
    for i, oid in enumerate(self._currently_allowed_optionals):
      o = self._currently_allowed_optionals[oid]
      Log(0, "#%s %s" % (i, str(o)))

    for i, s in enumerate(self._steps):
      Log(0, "#%d %s" % (i, str(s)))

    return {'http_code': 200, 'title': "OK"}

  def last_step(self):
    if self.step_count() == 0:
      return None

    return self._steps[self.step_count() - 1]

  def find_optional_by_id(self, optional_id):
    return self._currently_allowed_optionals.get(optional_id, None)

  def allow_optional(self, optional):
    if self.find_optional_by_id(optional.id()) is not None:
      Log(0, "Cannot allow an already-allowed optional.")
      return False

    Log(0, "Allowing an optional: %s" % str(optional))
    self._currently_allowed_optionals[optional.id()] = optional
    return True

  def disallow_optional(self, id):
    optional = self.find_optional_by_id(id)
    if optional is None:
      Log(0, "Cannot disallow an optional that is not currently allowed.")
      return False

    Log(0, "Disallowing an optional: %s" % str(optional))
    self._optionals_match_stats[optional.id()] = optional.match_count()
    self._currently_allowed_optionals.pop(optional.id(), None)
    return True

  def step_count(self):
    return len(self._steps)

  def is_script_finished(self):
    last_step_index = self.step_count() - 1
    if self._current_step_index > last_step_index:
      Log(0, "Current step past last step (current = {}, last = {})".format(
          self._current_step_index, last_step_index))
      return True

    if self._current_step_index == last_step_index:
      Log(0, "All {} steps walked through.".format(self.step_count()))
      return True

    Log(1, "Script still in progress (current = {}, last = {}).".format(
        self._current_step_index, last_step_index))
    return False

  def notify_script_fininshed(self, result):
    if self._script_finished_handler:
      Log(0, "Sending script fininshed notification: {}".format(result))
      self._script_finished_handler(result)
      self._script_finished_handler = None
    else:
      Log(1, "Script finished handler has not (yet?) been attached.")

  def remove_pending_optionals(self):
    for optional_id in self._currently_allowed_optionals.keys():
      self.disallow_optional(optional_id)

  def sum_up_matched_optionals(self):
    self.remove_pending_optionals()
    if len(self._optionals_match_stats) == 0:
      Log(0, "No optionals used in test.")
      return

    Log(0, "******** Optionals match stats ********")
    for optional_id, times in self._optionals_match_stats.iteritems():
      msg = "Optional with ID = %s matched %s times." % (optional_id, times)
      Log(0, msg)
    Log(0, "***************************************")

  def verify_execution(self):
    messages = []

    # Current step index should point to last_step + 1
    # at the end of the test.
    if not self._current_step_index == self.step_count():
      msg = "Not all steps processed (%d/%d)" % (self._current_step_index,
                                                 self.step_count())
      messages.append(msg)

    for s in self._steps:
      if not s.fully_matched():
        messages.append("Step not fully matched:\n%s" % str(s))

      if not s.fully_responded():
        messages.append("Step not fully responded:\n%s" % str(s))

    return messages

  def all_failure_messages(self):
    tmp = self.verify_execution()
    return tmp + self._failure_messages

  def failed(self):
    return len(self.all_failure_messages()) > 0

  def failure_message(self):
    ret = ""

    for m in self.all_failure_messages():
      ret = "%s\n-> %s" % (ret, m)

    return ret

  def steps(self):
    return self._steps

  def AddStep(self, postvars):
    """
    A sync token test consists of steps, that are set up in the C++ code.
    A step can have multiple expectations.
    Only one step is active at a time, the execution only passes to the next one
    once all expectations from the given step have been met.
    Note that actual actions responding to the events may or may not yet trigger
    at that time, as they can be delayed sing the delay_until set up value.
    An expectation consists of:
      type  - Type of the expectation, i.e. verification_requested, xmpp_connected,
              token_renewal_requested and others.
      event - A subset (including the whole set) of event properties that we expect
              to match the properties of an incoming event of the given type.
              Note that a property name missing from here will cause any property value
              for the given property name coming from the incoming event to match, i.e.
              a missing property key is like adding a property key with value set to '*'.
              (We don't handle wildcards thou).
      action - A set of action properties that are used to send the response for the
              incoming event that caused this expectation to match.
              Note that the type of the action is determined by the type of the expectation
              i.e. you cannot respond with "token renewal response" to a "auth header verification
              request".
              Do note that an action may include a 'delay_until' key that will delay
              sending the response back until ALL of the triggers set up for the given action
              fire.
      triggers - A list of triggers that will fire once the event is met.
    """

    if 'input_json' not in postvars:
      return {'http_code': 404,
              'title': "No input_json in postvars, internal error."}

    step_json_dict = json.loads(postvars['input_json'])

    step = AuthStep()
    (err, msg) = step.LoadFromJSON(step_json_dict)

    if not err == 200:
      Log(0, "Cannot add step from JSON: %s" % step_json_dict)
    else:
      Log(1, "Added step %s" % str(step))
      self._steps.append(step)

    return {'http_code': err, 'title': msg}

  def AddAllow(self, postvars):
    # In the C++ test code you can use Expect() to add steps and Allow() to add optionals.
    # An optional is an event along with a response that may or may not appear during
    # script execution and it doesn't change the execution flow or result in any way.
    # There are two reasons for having optionals:
    # a) to be able to properly serve client events with a desired response in case the test
    #    doesn't case about the event itself but it needs to respond with a specific result
    #    in order for the test to complete successfully;
    # b) to be able to cover client events that may or may not appear and may cause test
    #    flakiness.
    #
    # Not that the /post_allow endpoint only handles a single expectation as an input, i.e.
    # it is illegal to send an interleave within an Allow() call from within C++.

    if 'input_json' not in postvars:
      return {'http_code': 404,
              'title': "No input_json in postvars, C++ side error."}

    allow_json_dict = json.loads(postvars['input_json'])

    # This becomes a bit hacky here, we don't really need the step, yet we get a JSON
    # formatted as one in order to avoid additional coding.
    optional = AuthOptional()
    (err, msg) = optional.LoadFromJSON(allow_json_dict)

    if not err == 200:
      Log(0, "Cannot add an optional from JSON: %s" % (allow_json_dict))
    else:
      # Add the optional to the list of enabled optionals to the last step.
      # If there were no steps added, i.e. if no Expect() was called before this
      # call to Allow(), add this optional explicitly to the list of currently
      # allowed optionals.
      last_step = self.last_step()

      if last_step is None:
        self._currently_allowed_optionals[optional.id()] = optional
      else:
        last_step.add_enabled_optional(optional)

    return {'http_code': err, 'title': msg}

  def AddDisallow(self, postvars):
    # An optional that was Allow()ed at some point can be Disallow()ed at any later
    # point.
    # This is where we handle the Disallow() call from C++.
    if 'input_json' not in postvars:
      return {'http_code': 400,
              'title': "No input_json in postvars, C++ side error."}

    input_json_dict = json.loads(postvars['input_json'])

    id = input_json_dict.get('id')
    if id is None:
      return {'http_code': 400, 'title': "No id in postvars, C++ side error."}

    # Cannot add a disallow in case there is no last step, it makes no sense to start the
    # script with a Disallow().
    last_step = self.last_step()
    if last_step is None:
      err = "Disallowing an optional is only possible after at least one step has been added."
      Log(0, err)
      return {'http_code': 400, 'title': err}

    # It also doesn't make much sense for a step to allow and disallow the same optional
    if last_step.allows_optional_by_id(id):
      err = "A step cannot allow and disallow the same optional."
      Log(0, err)
      return {'http_code': 400, 'title': err}

    last_step.add_disabled_optional(id)
    return {'http_code': 200, 'title': "OK"}

  def TryMatchOneOfCurrentOptionals(self, in_type, event, handler):
    for optional_id, optional in self._currently_allowed_optionals.iteritems():
      if optional.MatchesTypeAndEvent(in_type, event):
        Log(0, "An optional was matched: %s" % str(optional))
        optional.RunAction(handler)
        return True

    return False

  def HandleIncomingEvent(self, type, event, handler):
    Log(0, "Incoming event with type '%s': %s" % (type, str(event)))

    if len(self._failure_messages) > 0:
      Log(0,
          "**** Script execution already failed, ignoring incoming event! ****")
      return

    if self._current_step_index >= self.step_count():
      if not self.TryMatchOneOfCurrentOptionals(type, event, handler):
        Log(0, "**** Current step is past the defined steps! ****")
        msg = "Current step is past defined steps for incoming event %s" % (
            str(event))
        self._failure_messages.append(msg)

      return

    step = self._steps[self._current_step_index]
    Log(2, "Current step: %s" % str(step))

    (err, msg, triggers, current_step_fully_matched) = step.HandleIncomingEvent(
        type, event, handler)
    if not err:
      Log(0, "Event matches step #%d" % self._current_step_index)

      # Fire triggers.
      self.HandleTriggersFired(triggers)

      if current_step_fully_matched:
        Log(1, "Advancing to next step.")

        # 1. Disable/enable optionals as the step orders.
        for o in step.optionals_disabled:
          self.disallow_optional(o)

        for o in step._optionals_enabled:
          self.allow_optional(o)

        if self.is_script_finished():
          self.notify_script_fininshed({'code': 200,
                                        'content': 'Just walked last step.',
                                        'handled': True})

        self._current_step_index += 1
    else:
      # The incoming event has yet another chance, to match any of the
      # currently allowed optionals.
      if not self.TryMatchOneOfCurrentOptionals(type, event, handler):
        Log(0, "**** Failed matching incoming event to current step! ****")
        Log(0, "Incoming event with type '%s': %s" % (type, str(event)), 1)
        Log(0, str(step), 1)
        self._failure_messages.append(msg)

    if type == 'verification_requested':
      (ok, msg) = self._uniqueness_tracker.is_unique(
          'nonce', event.expected()['oauth_nonce'])
      if not ok:
        Log(0, "**** %s ****" % msg)
        Log(0, "Incoming event with type '%s': %s" % (type, str(event)), 1)
        Log(0, str(step), 1)
        self._failure_messages.append(msg)

  def QueueNotifyScriptFinished(self, path, handler):
    Log(1, "Queuing script finished notification.")
    if self._script_finished_handler:
      Log(1, "Script finished handler has already been set, replacing.")

    self._script_finished_handler = handler

    if self.is_script_finished():
      self.notify_script_fininshed(
          {'code': 200,
           'content':
               'Script already finished when queuing the notifcation request.',
           'handled': True})

  def HandleTriggersFired(self, triggers):
    for t in triggers:
      if not t in self._fired_triggers:
        Log(0, "New trigger fired: '%s'" % t)
        self._fired_triggers.append(t)
      else:
        Log(0, "Trigger '%s' already fired!" % t)

    # Go through all pending actions to check which should be
    # performed now.
    # A pending action is a not-yet-performed action belonging to
    # one of already completed steps plus the current step.
    for i, step in enumerate(self._steps):
      if i > self._current_step_index:
        break

      step.HandleTriggersFired(self._fired_triggers)


class AuthOptional:

  def __init__(self):
    self._expectation = None
    self._match_counter = 0

  def match_count(self):
    return self._match_counter

  def MatchesTypeAndEvent(self, in_type, event):
    if self._expectation is None:
      Log(0,
          "An attempt to match an event to an optional with empty expectation, internal error.")
      return False

    if not self._expectation.type() == in_type:
      return False

    # Ignore the fact whether the event was matched or not.
    # We don't need this, an optional may match any numer
    # of times.

    if self._expectation._event.Matches(event):
      self._match_counter += 1
      return True

    return False

  def RunAction(self, handler):
    # Ingore triggers, run immediately.
    # Do we want triggers?

    self._expectation._action.SetHandler(handler, True)
    self._expectation._action.perform(True)

  def id(self):
    if self._expectation is None:
      return -1

    return self._expectation.id()

  def expectation(self):
    return self._expectation

  def LoadFromJSON(self, json):
    if 'expectations' not in json:
      return (400, "No expectations in step setup JSON.")

    expectations = json['expectations']
    expectation_count = len(expectations)

    if not expectation_count == 1:
      return (
          400,
          "An Allow() call must contain exacely one expecation, actual count = %d"
          % expectation_count)

    expectation = expectations[0]
    exp = AuthExpectation()
    (err, msg) = exp.LoadFromJSON(expectation)
    if err == 200:
      self._expectation = exp
    else:
      self._expectation = None
      return (err, msg)

    return (200, "OK")

  def __str__(self):
    ret = "Optional matched %d times:" % (self._match_counter)
    if self._expectation:
      ret = "%s\n\t%s" % (ret, str(self._expectation))

    return ret


class AuthStep:

  @property
  def optionals_disabled(self):
    return self._optionals_disabled

  @optionals_disabled.setter
  def optionals_disabled(self, value):
    pass

  def __init__(self):
    self._expectations = []

    self._optionals_enabled = []
    self._optionals_disabled = []

  def allows_optional_by_id(self, optional_id):
    for e in self._optionals_enabled:
      if e.id() == optional_id:
        return True

    return False

  def add_enabled_optional(self, optional):
    self._optionals_enabled.append(optional)

  def add_disabled_optional(self, optional_id):
    self._optionals_disabled.append(optional_id)

  def fully_responded(self):
    for e in self._expectations:
      if e.response_pending():
        return False

    return True

  def fully_matched(self):
    for e in self._expectations:
      if not e.event_matched():
        return False

    return True

  def LoadFromJSON(self, json):
    if 'expectations' not in json:
      return (404, "No expectations in step setup JSON.")

    for e in json['expectations']:
      exp = AuthExpectation()
      (err, msg) = exp.LoadFromJSON(e)
      if err == 200:
        self._expectations.append(exp)
      else:
        self._expectations = []
        return (err, msg)

    return (200, "OK")

  def HandleTriggersFired(self, all_triggers):
    for ex in self._expectations:
      ex.HandleTriggersFired(all_triggers)

  def HandleIncomingEvent(self, in_type, event, handler):
    # Search for a matching expectation
    for e in self._expectations:
      if e.NotMachedAndMatchesTypeAndEvent(in_type, event):
        newly_fired_triggers = e.Match(handler)
        return (False, "", newly_fired_triggers, self.fully_matched())

    return (True, "Could not match any epectation to event.", [],
            self.fully_matched())

  def __str__(self):
    ret = "**** STEP ****"
    ret = "%s\nExpectations:" % ret
    for i, e in enumerate(self._expectations):
      ret = "%s\n\t#%d %s" % (ret, i, str(e))

    if len(self._optionals_enabled) > 0:
      ret = "%s\nEnables optionals:\n" % ret
      for i, o in enumerate(self._optionals_enabled):
        ret = "%s\n\t%s" % (ret, str(o))

    if len(self._optionals_disabled) > 0:
      ret = "%s\nDisables optionals:\n" % ret
      for i, o in enumerate(self._optionals_disabled):
        ret = "%s\n\tID = %s" % (ret, str(o))

    return ret


class AuthExpectation:

  def __init__(self):
    self._type = ""
    self._event = AuthEvent()
    self._action = AuthAction()
    self._triggers = AuthTriggers()
    self._action_handler = None
    self._id = None

  def type(self):
    return self._type

  def id(self):
    return self._id

  def response_pending(self):
    if self._action.has_handler():
      if self._action.performed():
        return False
      else:
        return True

    return False

  def event_matched(self):
    return self._event.matched()

  def known_types(self):
    return ['verification_requested', 'clock_validation_requested',
            'token_renewal_requested', 'xmpp_authorization_finished',
            'xmpp_initialized', 'sync_disabled_event_sent']

  def LoadFromJSON(self, json):
    required_keys = ['type', 'action', 'triggers', 'id']
    for r in required_keys:
      if not r in json or json[r] is None:
        return (404, "No '%s' in expectation." % r)

    # The event dict may not exist in the request for some types.
    if 'event' not in json:
      json['event'] = {}

    ex_type = json['type']
    if not ex_type in self.known_types():
      return (404, "Type '%s' is not known. Known types are: %s" %
              (ex_type, str(self.known_types())))

    event = AuthEvent()
    action = AuthAction()
    triggers = AuthTriggers()

    (err, msg) = event.LoadFromJSON(json['event'])
    if not err == 200:
      return (err, msg)

    (err, msg) = action.LoadFromJSON(json['action'])
    if not err == 200:
      return (err, msg)

    (err, msg) = triggers.LoadFromJSON(json['triggers'])
    if not err == 200:
      return (err, msg)

    self._event = event
    self._action = action
    self._triggers = triggers
    self._type = ex_type
    self._id = json['id']

    return (200, "OK")

  def __str__(self):
    return "Expectation ID=%s type='%s':\n\t\t%s\n\t\t%s\n\t\t%s" % (
        self.id(), self._type, str(self._event), str(self._action),
        str(self._triggers))

  def NotMachedAndMatchesTypeAndEvent(self, in_type, event):
    if not self._type == in_type:
      return False

    if self._event.matched():
      return False

    if self._event.Matches(event):
      return True

    return False

  def Match(self, handler):
    Log(1, "Event becomes matched: (%s)" % str(self))

    self._event.SetMatched()
    self._action.SetHandler(handler)

    if self._action.delay_until() is None or len(self._action.delay_until(
    )) == 0:
      Log(0, "Event action is not delayed")
      self._action.perform()
    else:
      Log(0, "Event action is delayed until %s" % (self._action.delay_until()))

    return self._triggers.trigger_list()

  def HandleTriggersFired(self, all_triggers):
    if self._action.performed():
      return

    if self._action.delay_until() is None:
      Log(0,
          "Internal error, an immediate action was reached in HandleTriggersFired!")
      return

    if self._action.triggers_matched(all_triggers):
      if self._action.has_handler():
        Log(0, "Performing action for expectation %s" % str(self))
        self._action.perform()


class AuthEvent:

  def __init__(self):
    self._expected = {}
    self._matched = False

  def matched(self):
    return self._matched

  def SetMatched(self):
    self._matched = True

  def expected(self):
    return self._expected

  def LoadFromJSON(self, json):
    self._expected = json

    return (200, "OK")

  def __str__(self):
    if self._matched:
      matched = "matched"
    else:
      matched = "NOT MATCHED"
    return "Event [%s]: %s" % (matched, self._expected)

  def Matches(self, other):
    for k in self.expected().keys():
      if not k in other.expected():
        Log(1, "Event not matched since key '%s' not found in other." % k)
        return False

      o = other.expected()[k]
      t = self.expected()[k]
      if not o == t:
        Log(1,
            "Event not matched since value key '%s' does not match: this = '%s', other = '%s'"
            % (k, t, o))
        return False

    return True


class AuthAction:

  def __init__(self):
    self._params = {}
    self._performed = False
    self._handler = None

  def triggers_matched(self, all_triggers):
    if self.delay_until() is None:
      Log(0, "Should not happen")
      return False

    for d in self.delay_until():
      if d not in all_triggers:
        return False

    return True

  def delay_until(self):
    return self._params.get('delay_until')

  def has_handler(self):
    return not self._handler is None

  def perform(self, force=False):
    # If |force| is set to True, we don't check whether the action was
    # performed.
    # That is used by AuthOptional.
    if not force and self.performed():
      Log(0, "An action is being performed more than once! Internal error.")
      return

    if self._handler is None:
      Log(0,
          "An action is being performed with an empty handler! Internal error (or not?). %s"
          % str(self))
      return

    Log(0, "Performing action '%s'." % str(self))
    self._handler(self._params)
    self._performed = True

  def performed(self):
    return self._performed

  def SetHandler(self, handler, force=False):
    if not force and self._handler is not None:
      Log(0,
          "Internal error, setting a handler for an action that already has one: %s"
          % str(self))
      return

    self._handler = handler

  def LoadFromJSON(self, json):
    self._params = json

    return (200, "OK")

  def __str__(self):
    if self._performed:
      performed = "performed"
    else:
      performed = "NOT PERFORMED"

    return "Action [%s] [handler: %s]: %s" % (performed, str(self._handler),
                                              self._params)


class AuthTriggers:

  def __init__(self):
    self._trigger_list = []

  def LoadFromJSON(self, json):
    self._trigger_list = json

    return (200, "OK")

  def __str__(self):
    return "Triggers: %s" % (self._trigger_list)

  def trigger_list(self):
    return self._trigger_list


def Log(level, msg, indent_level=0):
  indent_step = 3
  current_level = 0

  if OperaAuth.opera_flags_.Get('expectations_debug'):
    current_level = 1

  if current_level >= level:
    print "AUTH: %s%s" % (indent_level * indent_step * " ", msg)


class OperaAuth:
  opera_flags_ = None

  def __init__(self, opera_flags):
    OperaAuth.opera_flags_ = opera_flags
    self._script = AuthScript()

  # Requests an asynchronous authorization header verification.
  # Note that the handler should be called eventually, simulating
  # network errors is done using one of values found in the request_data
  # dictionary.
  # For expected response format compare
  # HandshakeTask.OnAuthHeaderVerificationResponseAvailable().
  def HandleAuthHeaderVerificationAsync(self, request_dict, handler):
    incoming_event = AuthEvent()
    (err, msg) = incoming_event.LoadFromJSON(request_dict)

    if not err == 200:
      Log(0, "ERROR: Incoming event cannot be interpreted: (%s)" %
          str(request_dict))
      return  # Async method, ignore the handler?

    self._script.HandleIncomingEvent('verification_requested', incoming_event,
                                     handler)

  def NormalizePostVars(self, postvars):
    norm_postvars = {}
    for k in postvars.keys():
      norm_postvars[k] = postvars[k][0]

    return norm_postvars

  def HandleURL(self, parsed_path, postvars, handler=None):
    path = parsed_path.path
    query = parsed_path.query
    result = None
    response = {}

    if path.endswith(POST_STEP):
      if handler is not None:
        print "WARNING: Handler ignored for post_step."
      postvars = self.NormalizePostVars(postvars)
      return self._script.AddStep(postvars)
    elif path.endswith(POST_ALLOW):
      if handler is not None:
        print "WARNING: Handler ignored for post_allow."
      postvars = self.NormalizePostVars(postvars)
      return self._script.AddAllow(postvars)
    elif path.endswith(POST_DISALLOW):
      if handler is not None:
        print "WARNING: Handler ignored for post_allow."
      postvars = self.NormalizePostVars(postvars)
      return self._script.AddDisallow(postvars)
    elif path.endswith(LIST_STEPS):
      if handler is not None:
        print "WARNING: Handler ignored for list_steps."
      return self._script.ListSteps()
    elif path.endswith(CHECK_STATE):
      if handler is not None:
        print "WARNING: Handler ignored for check_state."
      return self.HandleCheckState()
    elif path.endswith(CLIENT_TRIGGER_FIRED):
      if handler is not None:
        print "WARNING: Handler ignored for client_trigger_fired."
      return self.HandleClientTriggerFired(postvars)
    elif path.endswith(SERVICE_VALIDATION_CLOCK):
      if handler is None:
        print "ERROR: Can't handle service/validation/clock without handler."
      postvars = self.NormalizePostVars(postvars)
      self.HandleServiceValidationClock(postvars, handler)
    elif path.endswith(ACCOUNT_ACCESSTOKEN_RENEWAL):
      if handler is None:
        print "ERROR: Can't handle account/access_token/renewal without handler."
      self.HandleAccountAccesstokenRenewal(query, handler)
    elif path.endswith(NOTIFY_SCRIPT_FINISHED):
      if handler is None:
        print "ERROR: notify_script_fininshed requires a handler."
      self.HandleNotifyScriptFinished(query, handler)

  def HandleClientTriggerFired(self, params):
    self._script.HandleTriggersFired(params['trigger_name'])
    return {'http_code': 200, 'title': "OK"}

  def HandleCheckState(self):
    self._script.sum_up_matched_optionals()
    self._script.verify_execution()
    if self._script.failed():
      return {'http_code': 404, 'title': self._script.failure_message()}

    return {'http_code': 200, 'title': "OK"}

  def HandleSyncEvent(self, event_name, event_dict, handler=None):
    incoming_event = AuthEvent()
    (err, msg) = incoming_event.LoadFromJSON(event_dict)

    if not err == 200:
      Log(0, "ERROR: Incoming event cannot be interpreted: (%s)" % str(h))
      return  # Async method, ignore the handler?

    self._script.HandleIncomingEvent(event_name, event_dict, handler)

  def HandleXMPPFinished(self, handler=None):
    incoming_event = AuthEvent()
    # No params to load from JSON. No JSON actually. Do we need anything here?
    self._script.HandleIncomingEvent('xmpp_authorization_finished',
                                     incoming_event, handler)

  def HandleXMPPInitialized(self, handler=None):
    incoming_event = AuthEvent()
    self._script.HandleIncomingEvent('xmpp_initialized', incoming_event,
                                     handler)

  def HandleServiceValidationClock(self, postvars, handler):
    incoming_event = AuthEvent()
    (err, msg) = incoming_event.LoadFromJSON(postvars)

    if not err == 200:
      Log(0,
          "ERROR: Incoming event cannot be interpreted: (%s)" % str(postvars))
      return  # Async method, ignore the handler?

    self._script.HandleIncomingEvent('clock_validation_requested',
                                     incoming_event, handler)

  def HandleAccountAccesstokenRenewal(self, path, handler):
    h = dict(urlparse.parse_qsl(path))

    incoming_event = AuthEvent()
    (err, msg) = incoming_event.LoadFromJSON(h)

    if not err == 200:
      Log(0, "ERROR: Incoming event cannot be interpreted: (%s)" % str(h))
      return  # Async method, ignore the handler?

    self._script.HandleIncomingEvent('token_renewal_requested', incoming_event,
                                     handler)

  def HandleNotifyScriptFinished(self, path, handler):
    self._script.QueueNotifyScriptFinished(path, handler)
