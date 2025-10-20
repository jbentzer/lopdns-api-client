import requests
from requests.exceptions import RequestException

class RestClient:
    def __init__(self, base_url: str, timeout: float = 10.0):
        self.base_url = base_url.rstrip('/')
        self.timeout = timeout

    def _build_url(self, path: str) -> str:
        return f"{self.base_url}/{path.lstrip('/')}"

    def get(self, path: str, params: dict | None = None, headers: dict | None = None) -> dict | str:
        url = self._build_url(path)
        try:
            r = requests.get(url, params=params, headers=headers, timeout=self.timeout)
            r.raise_for_status()
            try:
                return r.json()
            except ValueError:
                return r.text
        except RequestException as e:
            return {"error": str(e)}

    def post(self, path: str, json: dict | None = None, headers: dict | None = None) -> dict | str:
        url = self._build_url(path)
        try:
            r = requests.post(url, json=json, headers=headers, timeout=self.timeout)
            r.raise_for_status()
            try:
                return r.json()
            except ValueError:
                return r.text
        except RequestException as e:
            return {"error": str(e)}

    def put(self, path: str, json: dict | None = None, headers: dict | None = None) -> dict | str:
        url = self._build_url(path)
        try:
            r = requests.put(url, json=json, headers=headers, timeout=self.timeout)
            r.raise_for_status()
            try:
                return r.json()
            except ValueError:
                return r.text
        except RequestException as e:
            return {"error": str(e)}
