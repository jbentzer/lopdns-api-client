# Certbot script plugin

Based on an example from https://github.com/certbot/certbot/pull/3521.

Get your API key from https://lopdns.se/api.php.

The scripts are currently created based on the usage of the C++ version of the lopdns-api-client but can easily be modified for use with the containerized Python version.

## Usage

Usage:
   certbot certonly                                          \
   --script                                                  \
   --preferred-challenges=http                               \
   --script-auth=/path/to/certbot-lopdns-authenticator.sh    \
   --script-cleanup=/path/to/http/certbot-lopdns-cleanup.sh  \
   -d somehost.somedomain.com

