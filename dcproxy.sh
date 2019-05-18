#!/usr/bin/env bash
set -e

PROXY_VERSION=eb5a38290df7bd8f393d31e7b7e35146f316fa26
BUILD_DIR="$(pwd)/build-proxy"
CLONE_DIR="$(pwd)/tls-proxy"
SRC_DIR="$(pwd)/src"

mkdir -p "${BUILD_DIR}"

if [[ ! -d "${CLONE_DIR}" ]]; then
    git clone https://github.com/Verlihub/tls-proxy.git "${CLONE_DIR}"
    cd "${CLONE_DIR}"
else
    cd "${CLONE_DIR}"
    git fetch
fi
git checkout "${PROXY_VERSION}"
go build -buildmode=c-archive -o "${BUILD_DIR}/dcproxy.a" ./lib/lib.go
mv ${BUILD_DIR}/*.h "${SRC_DIR}/"
cp ./lib/dcproxy_types.h "${SRC_DIR}/"