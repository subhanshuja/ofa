import os
import mimetypes
import json

class OperaHTTP:
  """ HTTP comm helper methods to be used across all the Python testserver
  modules. """


  @staticmethod
  def AsDataDirPath(path):
    syncserver_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(syncserver_dir, os.pardir, 'data', path)

  # Generates a response dictionary to be used to send the HTTP response with
  # the _send_response() method.
  #
  # @handled: Whether the response was succesfully handled by the currently
  #   running handler, setting this to False should cause remaining handlers
  #   to run until one is found that returns True.
  # @code: The HTTP code that comes with the response.
  # @headers: HTTP headers to be included in the response.
  # @content_type: Allows overriding the Content-Type HTTP header sent with the
  #   response.
  # @content: Raw content response, e.g. a HTML document or a binary file to be
  #   sent to the client. Note that if content is not empty, to_json has to
  #   be equal to None.
  # @to_json: A dictionary of values that will be JSON-encoded and sent as the
  #   raw content response. Note that in case this param is not None, the
  #   explicit content parameter has to be empty.
  # @network_error: Setting this to True will casue closing the socket even
  #   before the HTTP response is sent, effectively causing a network error
  #   on the client side.
  # @file_name: A helper for uploading a specific file to the client. The file
  #   contents will be returned along with a mime type guessed basing on its
  #   extension. Note that the file will be looked for in the
  #   sync/testserver/data directory.
  @staticmethod
  def response(handled=True,
               code=200,
               headers=[],
               content_type='text/html',
               content='',
               to_json=None,
               network_error=False,
               file_name=None,
               replacements={}):

    if file_name is not None:
      assert (content is ''), "content must be empty if file_name given"
      assert (to_json is None), "to_json must be None if file_name given"

      file_name = OperaHTTP.AsDataDirPath(file_name)

      if not os.path.isfile(file_name):
        raise Exception("File '%s' not found." % file_name)

      content = open(file_name, 'r').read()
      content_type = mimetypes.MimeTypes().guess_type(file_name)

      for key in replacements:
        findstr = '[%s]' % key
        replacestr = str(replacements[key])
        content = content.replace(findstr, replacestr)

    elif to_json is not None:
      assert (content is ''), "content must be empty if to_json given"
      assert (file_name is None), "file_name must be None if to_json given"

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
        'content': content,
        'network_error': network_error,
    }
    return ret_dict
