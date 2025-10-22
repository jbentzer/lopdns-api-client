import os
from dataclasses import dataclass

from pydantic import BaseModel
from dnstask import DnsTask

@dataclass
class Settings:
    BASE_URL: str = os.environ.get('BASE_URL', 'https://api.lopdns.se/v2/')
    TIMEOUT: float = float(os.environ.get('TIMEOUT', '10'))
    TOKEN_DURATION_SEC: float = float(os.environ.get('TOKEN_DURATION_SEC', '600'))
    INTERVAL: int = int(os.environ.get('INTERVAL', '60'))
    ONCE: bool = os.environ.get('ONCE', 'true').lower() in ('1', 'true', 'yes')
    CLIENT_ID: str = os.environ.get('CLIENT_ID', '')
    CONFIG_FILE: str = os.environ.get('CONFIG_FILE', 'config.json')
    VERBOSE: bool = os.environ.get('VERBOSE', 'false').lower() in ('1', 'true', 'yes')
    CONFIG_FILE: str = os.environ.get('CONFIG_FILE', 'config.json')
    CONTENT_STATIC: str = os.environ.get('CONTENT_STATIC', '') 
    
class ConfigFile(BaseModel):
    dnsTasks: list[DnsTask] = []
