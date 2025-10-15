import json
from typing import Any
from xmlrpc.client import DateTime
import restclient
from restclient import RestClient

''' Wrapper for the LopDNS API (https://api.lopdns.se/v2/docs) '''

class Token():
    def __init__(self, token: str = "", expires: DateTime = DateTime(), epochExpires: int = 0, tz: str = ""):
        self.token = token
        self.expires = expires
        self.epochExpires = epochExpires
        self.tz = tz

class Zone():
    def __init__(self, name: str = "", serial: int = 0, refresh: int = 0, retry: int = 0, expire: int = 0, minimum: int = 0):
        self.name = name
        self.serial = serial
        self.refresh = refresh
        self.retry = retry
        self.expire = expire
        self.minimum = minimum

class Record():
    def __init__(self, name: str = "", type: str = "", content: str = "", ttl: int = 0, prio: int = 0):
        self.name = name
        self.type = type
        self.content = content
        self.ttl = ttl
        self.prio = prio

class LopApiClient:
    def __init__(self, rest_client: RestClient, token: Token = Token()):
        self.rest_client = rest_client
        self.token = token

    def authenticate(self, clientId: str, duration: int = 600) -> Token:
        path = "/auth/token"
        params = {"duration": duration}
        headers = {"x-clientid": f"{clientId}"}
        try:
            response = self.rest_client.get(path, params=params, headers=headers)
            self.token = Token(
                token=response.get("token", ""),
                expires=response.get("expires", DateTime()),
                epochExpires=response.get("epochExpires", 0),
                tz=response.get("tz", "")
            )
        except Exception as e:
            return {"error": str(e)}
        return self.token

    def isTokenExpired(self, minTimeLeftInSeconds: int = 30) -> bool:
        import time
        current_time = int(time.time())
        if not self.token or not self.token.epochExpires:
            return False
        return current_time > self.token.epochExpires - minTimeLeftInSeconds

    def validate_token(self) -> bool:
        if self.isTokenExpired():
            return False

        path = "/auth/validate"
        headers = {"x-token": f"{self.token.token}"}
        try:
            response = self.rest_client.get(path, headers=headers)
            return response.get("valid", False)
        except Exception as e:
            return {"error": str(e)}

    def getZones(self) -> Zone | dict:
        path = "/zones"
        headers = {"x-token": f"{self.token.token}"}
        try:
            response = self.rest_client.get(path, headers=headers)
            ''' According to documentation this should return a list of zones, but it returns a dict of zones names'''
            zones = []
            for zoneName in response:
                zones.append(Zone(name=zoneName))
            return zones
        except Exception as e:
            return {"error": str(e)}

    def getRecords(self, zone: str) -> dict | Record:
        path = f"/records/{zone}"
        headers = {"x-token": f"{self.token.token}"}
        try:
            response = self.rest_client.get(path, headers=headers)
            return [Record(**record) for record in response] if isinstance(response, list) else {}
        except Exception as e:
            return {"error": str(e)}

