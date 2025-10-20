from enum import Enum
from pydantic import BaseModel

class RegExSearch(BaseModel):
    pattern: str = ""
    group: int = 0

COPY_RECORD_CONTENT = "COPY_RECORD_CONTENT"
UPDATE_RECORD_CONTENT_REGEX = "UPDATE_RECORD_CONTENT_REGEX"

class DnsRecord(BaseModel):
    name: str = ""
    type: str = ""
    contentMatchRegEx: RegExSearch = None
    contentDataExtractRegEx: RegExSearch = None
    contentReplaceRegEx: RegExSearch = None

class DnsTask(BaseModel):
    zone: str = ""
    sourceRecord: DnsRecord = None
    targetRecord: DnsRecord = None
    taskType: str = COPY_RECORD_CONTENT
    dryRun: bool = False
