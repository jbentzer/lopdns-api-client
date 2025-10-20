# Containerized Python REST Client for LOP DNS

## Introduction

The LOP DNS is a free DNS service provided by lopnet AB (https://lopdns.se). This application cosnumes the REST API to read and modify DNS records.

API documentation: https://api.lopdns.se/v2/docs

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