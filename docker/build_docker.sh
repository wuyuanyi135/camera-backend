#!/usr/bin/env bash

docker build -f Dockerfile.amd64 -t microvision/mvcam:latest-amd64 ..
docker build -f Dockerfile.arm64 -t microvision/mvcam:latest-arm64v8 ..