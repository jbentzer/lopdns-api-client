#!/bin/bash

#
# certbot-lopdns-cleanup.sh
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

if [ -z "$CERTBOT_DOMAIN" ]; then
    echo "This script is meant to be run by certbot."
    exit 1
fi

if [ -z "$CLIENT_ID" ]; then
   CLIENT_ID="your-client-id-here"
fi

# Strip only the top domain to get the zone id
DOMAIN=$(expr match "$CERTBOT_DOMAIN" '.*\.\(.*\..*\)')

# The delete method in the LOPDNS API is not working so we just skip this step for now

echo "No cleanup method implemented, please delete the record manually if needed"

exit 0

lopdns-api-client \
        -c "$CLIENT_ID" \
        -a delete-record \
        -z "$DOMAIN" \
        -r "TXT" \
        -n "_acme-challenge.$CERTBOT_DOMAIN" \
        -A

# Exit with the status of the last command
exit $?
