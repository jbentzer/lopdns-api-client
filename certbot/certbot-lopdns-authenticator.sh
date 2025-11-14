#!/bin/bash

#
# certbot-lopdns-authenticator.sh
#
# Prerequisites: lopdns-api-client (C++ version)
#
#
# Usage:
#   certbot certonly --script                                 \
#   --preferred-challenges=http                               \
#   --script-auth=/path/to/certbot-lopdns-authenticator.sh    \
#   --script-cleanup=/path/to/http/certbot-lopdns-cleanup.sh  \
#   -d somehost.somedomain.com
#

if [ -z "$CERTBOT_DOMAIN" ] || [ -z "$CERTBOT_VALIDATION" ]; then
    echo "This script is meant to be run by certbot."
    exit 1
fi

if [ -z "$CLIENT_ID" ]; then
   CLIENT_ID="your-client-id-here"
fi

# Strip only the top domain to get the zone id
DOMAIN=$(expr match "$CERTBOT_DOMAIN" '.*\.\(.*\..*\)')

lopdns-api-client \
        -c "$CLIENT_ID" \
        -a updateorcreate-record \
        -z "$DOMAIN" \
        -r "TXT" \
        -n "_acme-challenge.$CERTBOT_DOMAIN" \
        -w "$CERTBOT_VALIDATION" \
        -T 120

# Exit with the status of the last command
exit $?