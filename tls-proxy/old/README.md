# TLS proxy

TLS proxy server for NMDC protocol. Currently supported by Verlihub 1.2.0.5 and later.

## Generate self signed certificate

`openssl req -new -newkey rsa:4096 -x509 -sha256 -days 365 -nodes -out "hub.crt" -keyout "hub.key"`

## Install Go

`sudo apt install golang-go`

## Compile proxy server

```
git clone https://github.com/verlihub/tls-proxy.git
cd tls-proxy
export CGO_ENABLED=0 && go build -ldflags "-libgcc=none" -tags netgo proxy.go
```

## Start proxy server

`./proxy --cert="/path/to/hub.crt" --key="/path/to/hub.key" --host="1.2.3.4:411" --hub="127.0.0.1:411"`

`1.2.3.4:411` is the proxy listening socket, the address that hub would normally be listening on. `127.0.0.1:411` is the hub listening socket, the address that accepts connections from the proxy. Add `&` at the end of command to run the process in background.

## Listen on multiple ports

`--host="1.2.3.4:411,1.2.3.4:1411"`

Host parameter takes a list of listening addresses separated by comma.

## Required TLS version

By default TLS proxy requires TLS version `1.2` and higher, to change that use `--ver 0/1/2/3` where the parameter is minor version number.

## Configure the hub

`!set listen_ip 127.0.0.1`

`!set tls_proxy_ip 127.0.0.1`

## Revert the configuration

`!set listen_ip 1.2.3.4`

`!set tls_proxy_ip `

Then start the hub as usual.

## Protocol specification

Following command is sent to the hub right after the connection is established:

`$MyIP 2.3.4.5 1.0|`

`2.3.4.5` is the real IP address of connected user. Second parameter stands for TLS version used by client, in case of non secure connection, `0.0` is passed to hub.

## Important to know

Remember to set twice the regular `ulimit` before starting both servers, because there will be twice as many connections than you would usually have.

## Command line options

`host` - Comma separated list of hosts to listen on

`wait` - Time to wait to detect the protocol

`hub` - Hub address to connect to

`ip` - Send client IP

`log` - Enable connection logging

`cert` - TLS .crt file

`key` - TLS .key file

`ver` - Minimum required TLS version

`buf` - Buffer size in KB
