# Copyright (C) 2013 Opera Software ASA.  All rights reserved.
# This file is an original work developed by Opera Software ASA

# Note that signfile-v2.sh is not backward compatible with opera based on presto.

# Sign using an RSA key

#Create a signature using the algorithm sha1WithRSAEncryption
openssl dgst -sha1 -sign verify_key.priv -out sign.bin  $1

# Convert the signature into base64
if [[ "$(uname -s)" == "Darwin" ]];
then
    base64 sign.bin > sign.b64
else
    base64 -w0 sign.bin > sign.b64
fi

# Concat the preamble, the signature and the signed file.
echo -n '// ' > $2
cat sign.b64 >> $2
if [[ "$(uname -s)" != "Darwin" ]];
then
    echo "" >> $2
fi
cat $1 >> $2

# Verify the signature
openssl dgst -sha1 -verify verify_key.b64 -signature sign.bin  $1

