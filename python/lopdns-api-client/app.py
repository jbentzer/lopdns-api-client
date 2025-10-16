#!/usr/bin/env python3
"""Simple CLI that calls an external REST API periodically or once.
"""
import os
import time
import logging
from config import Settings
from lopclient import LopApiClient
from restclient import RestClient

logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger(__name__)

clientId = os.environ.get('CLIENT_ID', '')
if not clientId:
    logger.error("CLIENT_ID environment variable is not set")
    exit(1)
    
def main():
    settings = Settings()
    rest_client = RestClient(base_url=settings.BASE_URL, timeout=settings.TIMEOUT)
    client = LopApiClient(rest_client)

    token = client.authenticate(clientId, duration=int(settings.TOKEN_DURATION_SEC))
    logger.info("Response: %s", token)
    if not client.validate_token():
        logger.error("Token validation failed")
        return
    logger.info("Token retrieved successfully")

    if settings.ONCE:
        logger.info("Running single request")
        do_logic(client)
    else:
        logger.info("Running continuous mode, interval=%s", settings.INTERVAL)
        try:
            while True:
                do_logic(client)
                time.sleep(settings.INTERVAL)
        except KeyboardInterrupt:
            logger.info("Shutting down")

def do_logic(client: LopApiClient):
    zones = client.getZones()
    logger.info("Zones: %s", zones)
    if isinstance(zones, dict) and "error" in zones:
        logger.error("Error fetching zones: %s", zones["error"])
        return
    for zone in zones if isinstance(zones, list) else []:
        records = client.getRecords(zone.name)
        logger.info("Records for zone %s: ", zone.name)
        for record in records if isinstance(records, list) else []:
            logger.info("Record for record %s of type %s: %s", record.name, record.type, record.content)

if __name__ == "__main__":
    main()
