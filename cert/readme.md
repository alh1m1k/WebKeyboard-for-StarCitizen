- cert.pem - Server certificate in pem format
- privkey.pem - Private key in pem format
- cacert.pem - (optional, detect presence) CA certificate ((CA used to sign clients, or client cert itself)

> [!NOTE]
> The certificate file descriptions are taken from the documentation
> https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/esp_https_server.html as is.
> And at the moment, I don't understand what espressif meant. Usually, when the API provides separate cert sections, 
> "cert" contains the server certificate, "cacert" contains the concat of trust chain certificates.
> That not work anymore. <br>
> The HTTPS server in release 0.3 uses a two-file configuration where cert.pem contains the entire chain of trust from 
> the server certificate to (but not including) the root of trust. privkey.pem contains the private key.

use idf.py build -DEMBED_CERT=ON  to include cert file in firmware
use idf.py build -DEMBED_CERT=OFF to exclude cert file from firmware

use config.h to setup https