#include "pch.h"

#include "jniutils.h"

#undef GetStringUTFChars
#undef ReleaseStringUTFChars
#undef NewStringUTF

#include <cassert>
#include <cstring>

extern "C" {
#include "bream_types.h"
#include "bream_utils.h"
}

// Modified UTF-8 or standard UTF-8 to standard UTF-8
static char *unmodifyUTF8(const char *str, bool allowInline);
// Standard UTF-8 to Modified UTF-8
static char *modifyUTF8(const char *str);

jnistring::jnistring(const jnistring& str) {
  reset();
  if (str.string_) {
    string_ = strdup(str.string_);
  }
}

jnistring& jnistring::operator=(const jnistring& str) {
  reset();
  if (str.string_) {
    string_ = strdup(str.string_);
  }
  return *this;
}

void jnistring::reset() {
  if (jstring_) {
    jstring_.env()->ReleaseStringUTFChars(*jstring_, string_);
    jstring_.reset();
  } else {
    free(string_);
  }
  string_ = NULL;
}

char* jnistring::pass() {
  char* ret;
  if (jstring_) {
    ret = strdup(string_);
    reset();
  } else {
    ret = string_;
    string_ = NULL;
  }
  return ret;
}

jnistring GetStringUTF8Chars(
    JNIEnv *env, jstring str, bool safe, bool takeOwnership) {
  if (!str) {
    return jnistring();
  }
  jboolean isCopy;
  const char *string = env->GetStringUTFChars(str, &isCopy);
  char *fixed = !safe ? unmodifyUTF8(string, isCopy) : 0;
  if (fixed) {
    env->ReleaseStringUTFChars(str, string);
    if (takeOwnership) {
      env->DeleteLocalRef(str);
    }
    return jnistring(fixed);
  }
  return jnistring(
      scoped_localref<jstring>(
          env, takeOwnership ? str : (jstring) env->NewLocalRef(str)),
      const_cast<char*>(string));
}

jnistring GetAndReleaseStringUTF8Chars(
    scoped_localref<jstring>& str, bool safe) {
  return GetStringUTF8Chars(str.env(), str.pass(), safe, true);
}

jnistring GetAndReleaseStringUTF8Chars(JNIEnv *env, jstring str, bool safe) {
  return GetStringUTF8Chars(env, str, safe, true);
}

scoped_localref<jstring> NewStringUTF8(
    JNIEnv *env, const char *str, bool safe) {
  char *fixed = !safe ? modifyUTF8(str) : 0;
  if (fixed) {
    scoped_localref<jstring> ret(env, env->NewStringUTF(fixed));
    delete[] fixed;
    return ret;
  } else {
    return scoped_localref<jstring>(env, env->NewStringUTF(str));
  }
}

/**
 * Helper to unmodifyUTF8.
 * @param str pointer to the first surrogate character or null-character found
 *            in the string we are removing them from
 * @param end pointed to the terminating NULL at the end of the string
 * @param c the unicode codepoint of the first surrogate character we found
 */
static void unmodifyUTF8Internal(char *str, char *end, unsigned int c) {
  for (;;) {
    if (c == 0) {
      /* null character, remove */
      assert(false);
      end -= 2;
      memmove(str, str + 2, end - str + 1);
    } else if (c >= 0xdc00 && c <= 0xdfff) {
      /* low surrogate without high surrogate, remove */
      assert(false);
      end -= 3;
      memmove(str, str + 3, end - str + 1);
    } else {
      unsigned int d = bream_utf8_decode_char(str + 3);
      if (d >= 0xdc00 && d <= 0xdfff) {
        char * const start = str;
        str = bream_utf8_encode_char(start,
                  0x10000 + (((c - 0xd800) << 10) | (d - 0xdc00)));
        assert(str - start == 4);
        memmove(str, start + 6, end - (start + 6) + 1);
        end -= 6 - (str - start);
      } else {
        /* high surrogate without low surrogate, remove */
        assert(false);
        end -= 3;
        memmove(str, str + 3, end - str + 1);
      }
    }

    for (;;) {
      if (str == end) {
        return;
      }
      const size_t len = bream_utf8_get_char_length(*str);
      if (len == 3 || len == 2) {
        c = bream_utf8_decode_char(str);
        if (c == 0 || (c >= 0xd800 && c <= 0xdfff)) {
          break;
        }
      }
      str += len;
    }
  }
}

/**
 * Remove any surrogate pairs found in a string encoded using "modified UTF-8"
 * (Java's name for broken UTF-8) and if needed return a newly allocated string
 * in UTF-8 encoding.
 * Also remove any \u0000 encoded as 0c80.
 * By setting allowInline to true you can save an allocation as correctly
 * encoded UTF-8 takes less space than "modified UTF-8" (4 bytes vs. 6 bytes).
 * @param str string to check
 * @param allowInline if true no new string need be allocated instead we
 *                    can correct input string directly
 * @return if any surrogate pairs were found and allowInline is false then
 *         a newly allocated UTF-8 string is returned. Otherwise 0 is returned.
 */
char *unmodifyUTF8(const char *str, bool allowInline) {
  const char *pos = str;
  while (*pos) {
    const size_t len = bream_utf8_get_char_length(*pos);
    if (len == 3 || len == 2) {
      const unsigned int c = bream_utf8_decode_char(pos);
      if (c == 0 || (c >= 0xd800 && c <= 0xdfff)) {
        if (allowInline) {
          char *ptr = const_cast<char*>(pos);
          unmodifyUTF8Internal(ptr, ptr + strlen(ptr), c);
          return 0;
        } else {
          const size_t len = (pos - str) + strlen(pos);
          char *out = (char *)malloc(len + 1);
          memcpy(out, str, len + 1);
          unmodifyUTF8Internal(out + (pos - str), out + len, c);
          return out;
        }
      }
    }
    pos += len;
  }
  return 0;
}

/**
 * Helper to modifyUTF8
 * @param str start of the string we are adding surrogate pairs to
 * @param pos position in the string of the first byte of the first 4 byte
 *            encoded unicode point
 */
static char *addSurrogatePairs(const char *str, const char *pos) {
  size_t len = strlen(pos);
  // more than enough for worst case + 1
  char *out = new char[(pos - str) + len * 2], *o;
  memcpy(out, str, pos - str);
  o = out + (pos - str);
  len = bream_utf8_get_char_length(*pos);
  do {
    unsigned int c = bream_utf8_decode_char(pos);
    if (c >= 0x10000) {
      c -= 0x10000;
      o = bream_utf8_encode_char(o, 0xd800 + (c >> 10));
      o = bream_utf8_encode_char(o, 0xdc00 + (c & 0x3ff));
    } else {
      // Overlong encoding
      assert(false);
      o = bream_utf8_encode_char(o, c);
    }
    pos += len;
    const char * const start = pos;
    while (*pos) {
      len = bream_utf8_get_char_length(*pos);
      if (len > 3) {
        break;
      }
      pos += len;
    }
    memcpy(o, start, pos - start);
    o += pos - start;
  } while (*pos);
  *o = '\0';
  return out;
}

/**
 * If a string contains any unicode points >= 0x10000 add surrogate pairs
 * where needed. Note that the resulting string is not valid UTF-8 but
 * "modified UTF-8" as Java calls it - exactly what JNI NewStringUTF excepts.
 * @param str string to check
 * @return if the string contained any unicode points >= 0x10000 then
 *         a new string using modified UTF-8 encoding is returned
 *         if the string did not contain any unicode points >= 0x10000 then
 *         0 is returned.
 */
char *modifyUTF8(const char *str) {
  const char *pos = str;
  while (*pos) {
    const size_t len = bream_utf8_get_char_length(*pos);
    // All unicode points >= 0x10000 (all that need surrogate pairs in UTF-16)
    // are encoded as 4 bytes in UTF-8.
    // All unicode points < 0x10000 are encoded as 3 bytes or lower in UTF-8.
    // Overlong UTF-8 sequences (they are invalid) will cause false positives.
    if (len > 3) {
      return addSurrogatePairs(str, pos);
    }
    pos += len;
  }
  return 0;
}
