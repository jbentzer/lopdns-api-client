# Containerized Python REST Client for LOP DNS

## Introduction

The LOP DNS is a free DNS service provided by lopnet AB (https://lopdns.se). This application cosnumes the REST API to read and modify DNS records.

API documentation: https://api.lopdns.se/v2/docs

## Build

```bash
docker build -t lopdns-api-client:latest .
```

## Run (single-run mode)

```bash
docker run --rm \
  -e CLIENT_ID=[MY_API_CLIENT_ID] \
  -e ONCE=true \
  lopdns-api-client:latest
```

## Run (continuous polling mode)

```bash
docker run --rm \
  -e CLIENT_ID=[MY_API_CLIENT_ID] \
  -e ONCE=false \
  -e INTERVAL=60 \
  python-rest-client:latest
```

You can also mount a `.env` file or pass environment variables via docker-compose.