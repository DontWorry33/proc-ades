#!/usr/bin/env bash

curl  -v  -s -L "http://localhost:7777/watchjob/processes/UndeployProcess/jobs/$1/result" -H "accept: application/json"
