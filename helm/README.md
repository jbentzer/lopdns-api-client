# Helm Charts

## lopdns-api-client

Usage example:

```bash
helm upgrade --install oci://registry-1.docker.io/janben/lopdns-api-client lopdns-api-client -n lopdns --create-namespace  -f values.yaml
```

Create a values.yaml with the CLIENT_ID and the config.json tasks. Example:

    env:
    - name: CLIENT_ID
        value: "MY_API_CLIENT_ID"
    - name: VERBOSE
        value: "true"
    - name: INTERVAL
        value: "900"

    configMap: |-
    {
        "dnsTasks":
        [
            {
                "zone": "somedomain.se",
                "sourceRecord": {
                    "name": "dyn.somedomain.com",
                    "type": "A"
                },
                "targetRecord": {
                    "name": "somedomain.se",
                    "type": "MX"
                },
                "taskType": "COPY_RECORD_CONTENT",
                "dryRun": true
            }
        ]
    }
