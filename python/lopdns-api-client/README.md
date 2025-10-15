# Containerized Python REST Client

## Build

```bash
docker build -t python-rest-client:latest .
```

## Run (single-run mode)

```bash
docker run --rm -e BASE_URL=https://api.publicapis.org \
  -e ENDPOINT=/entries \
  -e ONCE=true \
  python-rest-client:latest
```

## Run (continuous polling mode)

```bash
docker run --rm -e BASE_URL=https://api.publicapis.org \
  -e ENDPOINT=/entries \
  -e ONCE=false \
  -e INTERVAL=60 \
  python-rest-client:latest
```

You can also mount a `.env` file or pass environment variables via docker-compose.