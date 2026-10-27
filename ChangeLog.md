# 1.7.0.0

* Added: Try to detect external TLS proxy when running on same server
* Added: Configuration tls_force_myip to force external TLS proxy
* Added: Lua 5.5 support
* Added: Consider users with hidden chat on welcome message
* Added: Worker threads for blocking work, configuration adv_worker_threads (0 = all CPU cores)
* Added: Host name lookup on worker threads, with dns_lookup new connection waits up to 5 seconds for host name
* Added: Database thread with its own connection for writes and background reads, queued writes are done before hub stops
* Added: Configuration use_banlist_cache, ban check on login without database query
* Added: Epoll connection chooser when available, CMake flag USE_EPOLL, poll stays as fallback
* Added: Native TLS using OpenSSL instead of separate proxy, plain and encrypted clients on same port, CMake flag USE_TLS, configuration tls_enable (enabled by default)
* Added: TLS handshakes run on worker threads, one per CPU core, so many connecting users dont block the hub
* Added: TLS versions 1.0 and 1.1 still work when allowed with tls_min_ver, also on systems where OpenSSL disables them by default
* Added: Self-signed ECDSA P-256 certificate is created when missing and renewed when expired, using tls_cert_org, tls_cert_mail and tls_cert_host
* Added: Changes of tls_enable, tls_cert_file, tls_key_file and tls_min_ver apply without restart, replaced certificate file is loaded by itself
* Added: TLS protocol negotiation with nmdcs for clients that ask for it, certificate keyprint in log
* Updated: Hublist registration and update check run on their own threads without access to hub data
* Updated: Hub stop no longer waits for running hublist registration or update check
* Updated: Results of worker threads are processed without waiting for adv_step_delay
* Updated: Commands gethost and getinfo, and user reports to operator chat, look up host names in background
* Updated: Login statistics of registered users and ban hits are written in background
* Updated: Translation files
* Fixed: Hublist registration could hang for minutes on hublist that does not answer
* Fixed: Hublist registration and crash report could send incomplete data
* Fixed: NMDC command UnBan never removed any ban
* Fixed: Poll connection chooser added up timeouts of every 1024 sockets, which delayed active connections on big hubs
* Fixed: Data past end of output buffer could be sent when compressed data was missing ending pipe
* Fixed: Command $MyIP is treated as unknown unless it comes once from enabled external TLS proxy
* Fixed: Total download and upload show real network traffic, including TLS and data that was sent later from output buffer
* Fixed: Upload statistics wrote outside of zone table for data sent before user zone was set, now it is added to zone of user when it is known
* Removed: Deprecated plugins, only Lua and Plugman are left
* Removed: Old thread classes cThread, cWorkerThread, cThreadWork and cMutex
* Removed: Built-in Go TLS proxy and FearTLS library, with configurations tls_listen_port and tls_buf_size

# 1.6.1.0

* Fixed: Multithreading got locked by TLS proxy
* Added: More useful output on CMake configuration
* Added: Manual reconnect to MySQL on lost connection

# 1.6.0.0

* Added: Built-in TLS proxy instead of standalone command line utility, requires Go
* Added: New vh command line utility options --getconf and --setconf

# Older

Please follow commit log: https://github.com/verlihub/verlihub/commits/master
