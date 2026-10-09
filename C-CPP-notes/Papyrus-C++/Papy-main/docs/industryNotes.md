# Notes from SS

- Access to both basic auth and industry recommended auth
- Auto created report after execution
    - Flag enabled
    - Saves to current directroy by default
    - Lets you specify a file path to save to
    - JSON
        - Fields
            - Total payloads sent
            - Payloads failed
            - Average payloads per second at the time
            - Payload (not per line but have it there)

```json
{
   "papyReport":{
      "Payload": "{PLACEHOLDER}",
      "Paramaters": {
        "{PLACEHOLDER}": "{PLACEHOLDER}",
        "{PLACEHOLDER}": "{PLACEHOLDER}"
        ...
      },
      "payloadsSent": "{PLACEHOLDER}",
      "payloadsSuccessful": "{PLACEHOLDER}",
      "payloadsFailed": "{PLACEHOLDER}",
      "elapsedTimeOfExecution": "{PLACEHOLDER}",
   }
}
```