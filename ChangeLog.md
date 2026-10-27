# 1.7.0.0

* Added: CMake flag USE_FEARTLS_PROXY to use precompiled FearTLS proxy library instead
* Added: Try to detect external TLS proxy when running on same server
* Added: Configuration tls_force_myip to force external TLS proxy
* Added: Built-in TLS proxy regenerates certificates on expiration
* Added: Lua 5.5 support
* Added: Consider users with hidden chat on welcome message
* Added: Worker threads for blocking work, configuration adv_worker_threads (0 = all CPU cores)
* Added: Host name lookup on worker threads, with dns_lookup new connection waits up to 5 seconds for host name
* Added: Database thread with its own connection for writes and background reads, queued writes are done before hub stops
* Added: Configuration use_banlist_cache, ban check on login without database query
* Updated: Hublist registration and update check run on their own threads without access to hub data
* Updated: Hub stop no longer waits for running hublist registration or update check
* Updated: Results of worker threads are processed without waiting for adv_step_delay
* Updated: Commands gethost and getinfo, and user reports to operator chat, look up host names in background
* Updated: Login statistics of registered users and ban hits are written in background
* Updated: Translation files
* Fixed: Memory of built-in TLS proxy configuration was released with wrong function
* Fixed: Hublist registration could hang for minutes on hublist that does not answer
* Fixed: Hublist registration and crash report could send incomplete data
* Fixed: NMDC command UnBan never removed any ban
* Removed: Deprecated plugins, only Lua and Plugman are left
* Removed: Old thread classes cThread, cWorkerThread, cThreadWork and cMutex

# 1.6.1.0

* Fixed: Multithreading got locked by TLS proxy
* Added: More useful output on CMake configuration
* Added: Manual reconnect to MySQL on lost connection

# 1.6.0.0

* Added: Built-in TLS proxy instead of standalone command line utility, requires Go
* Added: New vh command line utility options --getconf and --setconf

# Older

Please follow commit log: https://github.com/verlihub/verlihub/commits/master
