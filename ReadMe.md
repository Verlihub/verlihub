# Verlihub 2.0.0.0

Verlihub is advanced NMDC protocol server for Linux and Windows operating systems that provides high level functionality such as:

  * Low RAM and CPU usage, epoll on Linux and Windows
  * It can hold more than 25k users
  * Worker threads for slow work
  * TLS-encrypted connection using OpenSSL, plain and encrypted users on same port
  * Simple installation and hub management
  * Backup and restore of hub database, MySQL or MariaDB
  * User management with privileges
  * Logging of user IPs and complete MyINFOs, requires Ledokol
  * Clients and connections management
  * GeoIP country, city and ASN of users, using MaxMindDB
  * Hub messages in many languages
  * Extend functionality with Lua scripts, including HTTP requests
  * Generate statistics for your hub, requires Ledokol
  * Plus much more

# Windows

Verlihub builds on Windows with MinGW-w64, see `Install.md` for dependencies and build steps.

# TLS

TLS support is built in using OpenSSL and enabled by default, to build without it use `-DUSE_TLS=OFF` flag.

External TLS proxy is still supported, it sends real address of user with `$MyIP` command, see `tls_listen_ip` and `tls_force_myip` configuration.

# Links

  * Website: https://github.com/verlihub
  * Wiki: https://github.com/verlihub/verlihub/wiki
  * Crash: https://crash.verlihub.net
  * Translate: https://explore.transifex.com/feardc/verlihub
  * Ledokol: https://ledo.feardc.net
  * MMDB: https://ledo.feardc.net/mmdb
  * Support: nmdcs://hub.verlihub.net:7777
  * Hublist: https://te-home.net/?do=hublist

# Donation

PayPal: [webmaster@feardc.net](https://www.paypal.com/paypalme/feardc/)
