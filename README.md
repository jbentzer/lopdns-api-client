# lopdns-api-client
API example client for LOP DNS.

API location: https://api.lopdns.se/v2/docs

## Postman

There is a Postman definition that can be used for testing the API functionality and developing queries.

## Python client

There is also a Python implementation of a client which copies information between records, typically to be used when a dynamic IP shifts and other records need to be updated (for instance the MX-record) or parts of a TXT record needs to adjusted. It can copy the complete contents of a source record into a target record, or extract data from a source record and substitute the data in the target record using regular expressions.
The execution can be one shot or periodic using a specified interval.

This application is containerized to be executed as a Docker contaier. There is also a helm chart if the app is to be deployed in a Kubernetes cluster.

## C++ client

The C++ client is a minimalistic application that can: 

- Read all zones
- Read all records
- Update a single record based on zone, record type, record name and current contents.
- Update all records based on zone, record type and record name.

## API Comments

### Zones

The /zones API returns a list of zone names, not a list of zone objects as indicated in the documentation.

### Records

The records API returns 

- The content of a record in a member called 'content' and not 'value'. 
- The priority in a member called 'prio' instead of 'priority'.
- Deleting a record results in http error 500.
