/* -----------------------------------------------------------------------------
 * string16.i shamelessly plagiarised from std_string.i
 *
 * Typemaps for string16 and const string16&
 * These are mapped to a Java String and are passed around by value.
 *
 * To use non-const string16 references use the following %apply.  Note
 * that they are passed by value.
 * %apply const string16 & {string16 &};
 * ----------------------------------------------------------------------------- */

%{
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
%}

namespace base {

%naturalvar string16;

class string16;

// string
%typemap(jni) string16 "jstring"
%typemap(jtype) string16 "String"
%typemap(jstype) string16 "String"
%typemap(javadirectorin) string16 "$jniinput"
%typemap(javadirectorout) string16 "$javacall"

%typemap(in) string16
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
     return $null;
    }
    const base::char16 *$1_pstr = (const base::char16 *)jenv->GetStringChars($input, 0);
    if (!$1_pstr) return $null;
    size_t $1_lstr = (size_t)jenv->GetStringLength($input);
    $1.assign($1_pstr, $1_lstr);
    jenv->ReleaseStringChars($input, $1_pstr); %}

%typemap(directorout) string16
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
     return $null;
   }
   const base::char16 *$1_pstr = (const base::char16 *)jenv->GetStringChars($input, 0);
   if (!$1_pstr) return $null;
   size_t $1_lstr = (size_t)jenv->GetStringLength($input);
   $result.assign($1_pstr, $1_lstr);
   jenv->ReleaseStringChars($input, $1_pstr); %}

%typemap(directorin,descriptor="Ljava/lang/String;") string16
%{ $input = jenv->NewString($1.c_str(), $1.length()); %}

%typemap(out) string16
%{ $result = jenv->NewString($1.c_str(), $1.length()); %}

%typemap(javain) string16 "$javainput"

%typemap(javaout) string16 {
    return $jnicall;
  }

// TODO(ohrn) Fix typecheck to get base::char16* buffers converted to jstrings for free.
//%typemap(typecheck) string16 = base::char16 *;

%typemap(throws) string16
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, UTF16ToUTF8($1).c_str());
   return $null; %}

// const string &
%typemap(jni) const string16 & "jstring"
%typemap(jtype) const string16 & "String"
%typemap(jstype) const string16 & "String"
%typemap(javadirectorin) const string16 & "$jniinput"
%typemap(javadirectorout) const string16 & "$javacall"

%typemap(in) const string16 &
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
     return $null;
   }
   const base::char16 *$1_pstr = (const base::char16 *)jenv->GetStringChars($input, 0);
   if (!$1_pstr) return $null;
   size_t $1_lstr = (size_t)jenv->GetStringLength($input);
   $*1_ltype $1_str($1_pstr, $1_lstr);
   $1 = &$1_str;
   jenv->ReleaseStringChars($input, $1_pstr); %}

%typemap(directorout,warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const string16 &
%{ if(!$input) {
     SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "null string");
     return $null;
   }
   const base::char16 *$1_pstr = (const base::char16 *)jenv->GetStringChars($input, 0);
   if (!$1_pstr) return $null;
   /* possible thread/reentrant code problem */
   static $*1_ltype $1_str;
   size_t $1_lstr = (size_t)jenv->GetStringLength($input);
   $1_str.assign($1_pstr, $1_lstr);
   $result = &$1_str;
   jenv->ReleaseStringChars($input, $1_pstr); %}

%typemap(directorin,descriptor="Ljava/lang/String;") const string16 &
%{ $input = jenv->NewString($1.c_str(), $1.length()); %}

%typemap(out) const string16 &
%{ $result = jenv->NewString($1->c_str(), $1->length()); %}

%typemap(javain) const string16 & "$javainput"

%typemap(javaout) const string16 & {
    return $jnicall;
  }

// TODO(ohrn) Fix typecheck to get char16* buffers converted to jstrings for free.
//%typemap(typecheck) const string16 & = base::char16 *;

%typemap(throws) const string16 &
%{ SWIG_JavaThrowException(jenv, SWIG_JavaRuntimeException, UTF16ToUTF8($1).c_str());
   return $null; %}

}
