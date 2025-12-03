- cert.pem - server certificate in pem format
- privkey.pem - server private key in pem format
- cacert.pem - (optional, detect presence) server authority center certificate in pem format

use idf.py build -DEMBED_CERT=ON  to include cert file in firmware
use idf.py build -DEMBED_CERT=OFF to exclude cert file from firmware

use config.h to setup https