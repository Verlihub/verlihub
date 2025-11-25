# FearTLS

FearDC TLS proxy server library for NMDC protocol, written in Pascal, currently supported by Verlihub version `1.7.0.0` and later.

It is included in source code with ability to use as built-in library with following option:

`cmake -DUSE_TLS_PROXY=ON ..`

## Important to know

Remember to set twice the regular `ulimit` before starting both servers, because there will be twice as many connections than you would usually have - half of them are local connections.

Verlihub does that automatically when using `vh` command line utility.

## Common setup

By default, to use TLS proxy you only need to change `tls_listen_port` from `0` to any local port, `tls_listen_ip` is already set to `127.0.0.1`, and restart your hub.

To stop using TLS proxy, simply set back `tls_listen_port` to `0` and restart the hub.

`listen_ip`, `listen_port` and `extra_listen_ports` will be applied automatically either to externally listening hub without TLS proxy, or to proxy itself with locally listening hub.

## Moving from command line

If you were previously using command line version of TLS proxy, all you need to do is to change `listen_ip` from `127.0.0.1` to your external IP address, `tls_listen_port` from `0` to any local port, shut down command line server and restart your hub.

## Required TLS version

By default TLS proxy requires TLS version `1.2` and higher, to change that use `tls_min_ver` variable, parameter is minor version number: `0` - `1.0`, `1` - `1.1`, `2` - `1.2`, `3` - `1.3`

## Generating certificates

You can generate your own self-signed certificates using OpenSSL binary:

`openssl req -new -newkey rsa:4096 -x509 -sha256 -days 1800 -nodes -out "hub.crt" -keyout "hub.key"`

## Configuration variables

`listen_ip` - Listening address either for hub or proxy, default: `0.0.0.0`

`listen_port` - Main listening port either for hub or proxy, default: `4111`

`extra_listen_ports` - Extra listening ports either for hub or proxy, separated by space, default: `<none>`

`tls_listen_ip` - Local listening address for hub when proxy is enabled, default: `127.0.0.1`

`tls_listen_port` - Local listening port for hub, `0` means proxy is disabled, default: `0`

`tls_only_mode` - Allow only TLS-encrypted users to enter hub, default: `No`

`not_tls_redirect` - Redirect address for users who are not TLS-encrypted, default: `<none>`

`tls_detect_wait` - Protocol detection time in milliseconds, default: `600`

`tls_cert_file` - Name of public certificate file in hub configuration directory, default: `hub.crt`

`tls_key_file` - Name of private key file in hub configuration directory, default: `hub.key`

`tls_min_ver` - Minimum required TLS version, default: `2` - equals to `1.2`

`tls_err_log` - Enable TLS proxy error logging to standard output, default: `No`

## Protocol specification

Following command is sent to the hub right after the connection is established:

`$MyIP 2.3.4.5 1.0|`

`2.3.4.5` is the real IP address of connected user.
Second parameter stands for TLS version used by client, in case of non secure connection, `0.0` is passed to hub.
