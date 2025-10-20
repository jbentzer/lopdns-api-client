# Containerized Python REST Client for LOP DNS

## Introduction

The LOP DNS is a free DNS service provided by lopnet AB (https://lopdns.se). This application cosnumes the REST API to read and modify DNS records.

API documentation: https://api.lopdns.se/v2/docs

This client application modifies the contents of a DNS record based on the content of another record. This can for instance be used to update the contents of a TXT record based on a dynamic IP address in an A record, or update a an MX record with the same IP address.

The tasks to run are configured in the config.json file. A task uses regular expressions to find the source and target records as well as the substitution of data within the contents of the target task.

The first example in the config.json file copies the contents of the A record dyn.somedomain.com to the MX record somedomain.com. 
The second example uses the contents of the same A record to replace the IP address of the target record (TXT) containing an IP address like "... ipv4:AA.BB.CC.DD/32 ..." 

## Build

```bash
docker build -t lopdns-api-client:latest .
```

## Debug

Set environment variables, for instance CLIENT_ID, in a file called env.debug, which is include in the gitignore to ensure that secret values are not committed.

## Run (single-run mode)

```bash
docker run --rm \
  -e CLIENT_ID=[MY_API_CLIENT_ID] \
  -e ONCE=true
  -e VERBOSE=true \
  --mount type=bind,source=./debug.config.json,target=/app/config.json,readonly \
  janben/lopdns-api-client:latest
```

## Run (continuous polling mode)

```bash
docker run -d \
  -e CLIENT_ID=[MY_API_CLIENT_ID] \
  -e VERBOSE=false \
  --mount type=bind,source=./debug.config.json,target=/app/config.json,readonly \
  --name lopdns-api-client \
  janben/lopdns-api-client:latest
```

You can also mount a `.env` file or pass environment variables via docker-compose.