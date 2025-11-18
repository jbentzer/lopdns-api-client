#!/bin/bash

#
# certbot-lopdns-authenticator.sh
#
# Prerequisites: lopdns-api-client (C++ version)
#
#
# Usage:
#   certbot certonly --manual                                      \
#   --preferred-challenges=dns-01                                  \
#   --manual-auth-hook=/path/to/certbot-lopdns-authenticator.sh    \
#   --manual-cleanup-hook=/path/to/http/certbot-lopdns-cleanup.sh  \
#   -d somehost.somedomain.com
#

if [ -z "$CERTBOT_DOMAIN" ] || [ -z "$CERTBOT_VALIDATION" ]; then
    echo "This script is meant to be run by certbot."
    exit 1
fi

if [ -z "$CLIENT_ID" ]; then
   CLIENT_ID="your-client-id-here"
fi

lopdns-api-client \
        -c "$CLIENT_ID" \
        -a createorupdate-record \
        -z "$CERTBOT_DOMAIN" \
        -r "TXT" \
        -n "_acme-challenge.$CERTBOT_DOMAIN" \
        -w "$CERTBOT_VALIDATION" \
        -T 120

# Allow some time for DNS propagation
sleep 25

# Exit with the status of the last command
exit $?