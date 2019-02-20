#!/usr/bin/env bash

docker build -f Dockerfile.env.amd64 -t microvision/mvcam-env:latest-amd64 ..
docker build -f Dockerfile.env.arm64 -t microvision/mvcam-env:latest-arm64v8 ..