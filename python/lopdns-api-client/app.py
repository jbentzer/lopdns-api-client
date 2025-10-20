#!/usr/bin/env python3
"""Simple CLI that calls an external REST API periodically or once.
"""
from logging import config
import os
import re
import time
import logging
import re

from config import Settings
from config import ConfigFile
from lopdnsclient import LopApiClient
from restclient import RestClient
from dnstask import DnsTask
import json
from pydantic import BaseModel


logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
logger = logging.getLogger(__name__)
    
def main():
    settings = Settings()
    if not settings.CLIENT_ID:
        logger.error("CLIENT_ID not set in environment")
        return
    rest_client = RestClient(base_url=settings.BASE_URL, timeout=settings.TIMEOUT)
    client = LopApiClient(rest_client)

    with open(settings.CONFIG_FILE, 'r') as f:
        configData = f.read()

    configFile = ConfigFile.model_validate_json(configData)
    dns_tasks = [configFile.dnsTasks[i] for i in range(len(configFile.dnsTasks))]

    token = client.authenticate(settings.CLIENT_ID, duration=int(settings.TOKEN_DURATION_SEC))
    if not client.validate_token():
        logger.error("Token validation failed")
        return
    logger.info("Token retrieved successfully")

    if settings.ONCE:
        logger.info("Running single request")
    else:
        logger.info("Running continuous mode, interval=%s", settings.INTERVAL)
    try:
        while True:
            for dnsTask in dns_tasks:
                do_task(client, dnsTask, settings.VERBOSE)
            if settings.ONCE:
                break
            time.sleep(settings.INTERVAL)
    except KeyboardInterrupt:
        logger.info("Shutting down")

def do_task(client: LopApiClient, dnsTask: DnsTask, verbose: bool = False):
    if not isinstance(dnsTask, DnsTask):
        logger.error("Invalid dnsTask provided")
        return
    
    zones = client.getZones()
    if (verbose):
        logger.info("Zones: %s", [zone.name for zone in zones] if isinstance(zones, list) else zones)
    if isinstance(zones, dict) and "error" in zones:
        logger.error("Error fetching zones: %s", zones["error"])
        return
    if dnsTask.zone not in [zone.name for zone in zones]:
        logger.error("Zone %s not found in account", dnsTask.zone)
        return

    sourceRecordData = ""
    dataInRecordToBeUpdated = ""
    oldContentInRecordToBeUpdated = ""
    sourceFound = False
    targetFound = False

    records = client.getRecords(dnsTask.zone)

    if (verbose):
        logger.info("Zone for task: %s", dnsTask.zone)
        logger.info("Records in zone %s: ", dnsTask.zone)

    # Iterate records to find source and target records
    for record in records if isinstance(records, list) else []:
        # Break early if both records are found
        if sourceRecordData and dataInRecordToBeUpdated:
            break

        if (verbose):
            logger.info("Record %s of type %s: %s", record.name, record.type, record.content)
        
        # Find the source record and extract the current data
        if (
            not sourceFound and
            record.type == dnsTask.sourceRecord.type and 
            record.name == dnsTask.sourceRecord.name and 
            (not dnsTask.sourceRecord.contentMatchRegEx 
                or re.search(dnsTask.sourceRecord.contentMatchRegEx.pattern, record.content).group(dnsTask.sourceRecord.contentMatchRegEx.group)
                )
            ):
            if dnsTask.sourceRecord.contentMatchRegEx:
                sourceRecordData = re.search(dnsTask.sourceRecord.contentDataExtractRegEx.pattern, record.content).group(dnsTask.sourceRecord.contentDataExtractRegEx.group)
            else:
                sourceRecordData = record.content
            sourceFound = True

            if (sourceRecordData and verbose):
                logger.info("Source data: %s", sourceRecordData)

        # Find the target record to be updated and extract the data to be replaced
        if (
            not targetFound and
            record.type == dnsTask.targetRecord.type and 
            record.name == dnsTask.targetRecord.name and 
            (not dnsTask.targetRecord.contentMatchRegEx 
                or re.search(dnsTask.targetRecord.contentMatchRegEx.pattern, record.content).group(dnsTask.targetRecord.contentMatchRegEx.group)
                )
            ):
            # Extract the data to be replaced from the record content using regex
            oldContentInRecordToBeUpdated = record.content
            if dnsTask.targetRecord.contentDataExtractRegEx:
                dataInRecordToBeUpdated = re.search(dnsTask.targetRecord.contentDataExtractRegEx.pattern, record.content).group(dnsTask.targetRecord.contentDataExtractRegEx.group)
            else:
                dataInRecordToBeUpdated = record.content
            targetFound = True

            if (oldContentInRecordToBeUpdated and verbose):
                logger.info("Target data: %s", dataInRecordToBeUpdated)

    if not sourceFound:
        logger.error("Could not find source record")
        return
    if not targetFound:
        logger.error("Could not find the target record")
        return
    if not dataInRecordToBeUpdated:
        logger.error("Could not find the data within the target record")
        return
    if (verbose):
        logger.info("Existing data to be replaced: %s", sourceRecordData)
        logger.info("New data: %s", dataInRecordToBeUpdated)
    if dataInRecordToBeUpdated != sourceRecordData:
        if (verbose):
            logger.info("Data has changed, updating record")
        # Substitute the data in the record content
        new_content = ""
        if dnsTask.targetRecord.contentReplaceRegEx:
            new_content = re.sub(dnsTask.targetRecord.contentReplaceRegEx.pattern, sourceRecordData, oldContentInRecordToBeUpdated)
        else:
            new_content = sourceRecordData
        if dnsTask.dryRun:
            logger.info("[Dry run] Record of type %s and old content %s in zone %s updated with new content %s", dnsTask.targetRecord.type, oldContentInRecordToBeUpdated, dnsTask.zone, new_content)
            return
        update_response = client.updateRecord(dnsTask.zone, dnsTask.targetRecord.name, dnsTask.targetRecord.type, oldContentInRecordToBeUpdated, new_content)
        logger.info("Record of type %s and old content %s in zone %s updated with new content %s: %s", dnsTask.targetRecord.type, oldContentInRecordToBeUpdated, dnsTask.zone, new_content, update_response)
    else:
        if (verbose):
            logger.info("Data in source and target records are identical, no update needed")

if __name__ == "__main__":
    main()
