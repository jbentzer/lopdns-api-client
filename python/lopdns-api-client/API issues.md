# API issues

## Zones

The /zones API returns a list of zone names, not a list of zone objects as indicated in the documentation.

## Records

The records API returns 

- The content of a record in a member called 'content' and not 'value'. 
- The priority in a member called 'prio' instead of 'priority'.
