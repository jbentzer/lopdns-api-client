# Use official slim python base
FROM python:3.13-slim

# Create app directory
WORKDIR /app

# Install runtime dependencies
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY . /app

# Make entrypoint executable
RUN chmod +x /app/entrypoint.sh

# Default environment variables (can be overridden at runtime)
ENV CLIENT_ID=""
ENV CONFIG_FILE="config.json"
ENV INTERVAL=60
ENV ONCE=false

ENTRYPOINT ["/app/entrypoint.sh"]
