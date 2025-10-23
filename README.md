# lopdns-api-client
API example client(s) for LOP DNS.

API location: https://api.lopdns.se/v2/docs

There is a Postman definition that can be used for testing the API functionality and developing queries.

There is also a Python implementation of a client which copies information between records, typically to be used when a dynamic IP shifts and other records need to be updated (for instance the MX-record) or parts of a TXT record needs to adjusted.

## API Comments

### Zones

The /zones API returns a list of zone names, not a list of zone objects as indicated in the documentation.

### Records

The records API returns 

- The content of a record in a member called 'content' and not 'value'. 
- The priority in a member called 'prio' instead of 'priority'.
- Deleting a record results in http error 500.
