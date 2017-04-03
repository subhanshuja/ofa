# Copyright (C) 2013 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA

# Generate 2048 bit RSA key
openssl genrsa -f4 -des3 -out verify_key.priv 2048
openssl rsa  -pubout -inform PEM -in verify_key.priv -outform der -out verify_key.pub
openssl rsa  -pubin -pubout -inform der -in verify_key.pub -outform pem -out verify_key.b64

echo "static const uint8_t DOM_BROWSERJS_KEY[] = {" > verify_key.h
cat verify_key.pub | xxd -i >> verify_key.h
echo "};" >> verify_key.h


