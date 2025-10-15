# Use official slim python base
FROM python:3.11-slim

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
ENV BASE_URL=https://api.example.com
ENV ENDPOINT=/health
ENV TIMEOUT=10
ENV INTERVAL=60
ENV ONCE=true

ENTRYPOINT ["/app/entrypoint.sh"]
