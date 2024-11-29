# Wazuh-Hook

Fork of [Wazuh](https://github.com/wazuh/wazuh/) with hook mechanism to intervene log analysis by accessing external REST API servers.

## Hook Architecture

![Hook Architecture](./wazuh-hook.png)

## Changes

|New File|Path|
|---|---|
|hook.c|src/analysisd/output|
|hook.h|src/analysisd/output|
|pii.json|hook/|

|Modified File|Path|
|---|---|
|analysisd.c|src/analysisd|
|Makefile|src|

## Build

```
make -C src PII_ENABLED=1 TARGET=server 
```

Copy pii.json to ossec path:

```
cp src/pii.json /var/ossec/etc
```

## Example Result

> {"output":"2:172.26.0.3:1 <DATE_TIME>1621-29-46700:06:625044162</DATE_TIME> ... This is a test log from '<EMAI
L_ADDRESS>VGAo81Gi6@RJAi0.w6d</EMAIL_ADDRESS>'"}                               
