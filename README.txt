
This README explains how to build a custom native library for the Opera browser.


1. Prerequisites

Unpack the source tarball in a directory of your choice.

Install the development tools needed to build Chromium for Android as described
here: https://www.chromium.org/developers/how-tos/android-build-instructions

Clone the Android SDK and NDK as follows:

  cd chromium/src/third_party
  git clone https://chromium.googlesource.com/android_tools
  cd android_tools
  git checkout 25d57ead05d3dfef26e9c19b13ed10b0a69829cf
  git clone https://chromium.googlesource.com/android_ndk ndk
  cd ndk
  git checkout a0190968500138134251e1c0e264d8cb496eabe0

2. Compiling

Enter mobile/mobile and build the chromium target. Set DEBUG to NO and
TARGET_ARCH to a CPU architecture, possible values are armv7 or x86.

Example:

  cd mobile/mobile
  make chromium DEBUG=NO TARGET_ARCH=armv7 GN_FLAGS=is_public_source_release=true

The output library is named libopera.so and placed in chromium_libs/<abi>/.

3. Rebuilding the APK

Look up the apk file name using pm path and pull it off your phone, e.g.

  adb shell pm path com.opera.browser.beta
  adb pull /data/app/com.opera.browser.beta-1.apk opera.apk

Use apktool from http://ibotpeaches.github.io/Apktool/ to unpack it:

  apktool d -s opera.apk

Replace the library file in the assets, using the one you built in the
previous step.

  cp chromium_libs/armeabi-v7a/libopera.so opera/lib/armeabi-v7a/libopera.so

Build a new apk using apktool:

  apktool b opera -o custom.apk

See http://developer.android.com/tools/publishing/app-signing.html for
instructions on how to sign and align the apk file.

