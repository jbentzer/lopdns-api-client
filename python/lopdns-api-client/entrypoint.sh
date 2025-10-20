#!/usr/bin/env bash
set -euo pipefail

# Optional: print environment for debugging
if [ "${DEBUG:-}" = "1" ]; then
  echo "Environment variables:" >&2
  env >&2
fi

# Run the python app
python /app/app.py
