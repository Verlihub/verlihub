# 1.7.0.0

* Added: Try to detect external TLS proxy when running on same server
* Added: Configuration tls_force_myip to force external TLS proxy
* Added: Lua 5.5 support
* Added: Consider users with hidden chat on welcome message
* Added: Worker threads for blocking work, configuration adv_worker_threads (0 = all CPU cores)
* Added: Host name lookup on worker threads, with dns_lookup new connection waits up to 5 seconds for host name
* Added: Database thread with its own connection for writes and background reads, queued writes are done before hub stops
* Added: Configuration use_banlist_cache, ban check on login without database query
* Added: Custom MySQL port in dbconfig with db_host = host:port, connection uses TCP then, also in vh script
* Added: Configuration db_tls in dbconfig to require encrypted connection to MySQL server, disabled by default, also newer MariaDB client library no longer refuses server without TLS
* Added: Configuration db_socket in dbconfig, path of local MySQL socket when it differs from built-in path of client library, verlihub --install finds it, same as vh script did
* Added: Epoll connection chooser when available, CMake flag USE_EPOLL, poll stays as fallback
* Added: Windows support using MinGW-w64, see Install.md
* Added: On Windows Ctrl+C, Ctrl+Break or closing console window stops hub cleanly, second Ctrl+C ends it at once
* Added: On Windows crash in any thread writes stack backtrace, plugins are loaded from DLL files
* Added: Epoll connection chooser on Windows using bundled wepoll library, select stays as fallback, see ThirdParty.md
* Added: CMake package configuration and installed config.h, plugins built outside of Verlihub source use find_package(Verlihub) and get same defines as Verlihub, see Install.md
* Added: Native TLS using OpenSSL instead of separate proxy, plain and encrypted clients on same port, CMake flag USE_TLS, configuration tls_enable (enabled by default)
* Added: TLS handshakes run on worker threads, one per CPU core, so many connecting users dont block the hub
* Added: TLS versions 1.0 and 1.1 still work when allowed with tls_min_ver, also on systems where OpenSSL disables them by default
* Added: Self-signed ECDSA P-256 certificate is created when missing and renewed when expired, using tls_cert_org, tls_cert_mail and tls_cert_host
* Added: Changes of tls_enable, tls_cert_file, tls_key_file and tls_min_ver apply without restart, replaced certificate file is loaded by itself
* Added: TLS protocol negotiation with nmdcs for clients that ask for it, certificate keyprint in log
* Added: On Windows verlihub --stop and --reload work with named events, there are no signals
* Added: Commands !getvar and !getconfig list unused hub variables separately, those that current hub version doesn't know anymore, variables of plugins and scripts are listed as before
* Updated: Shell scripts and their commands are replaced by verlihub executable itself, same commands and parameters, they work on Linux and Windows, see verlihub --help
* Updated: Syslog options of hub are only long --syslog and --syslog-suffix, short -s is stop command, same as in vh script
* Updated: Multi hub manager vhm is verlihub itself, on Linux it is installed as symbolic link, on Windows use verlihub -m, hubs are started as their process owner without su, so owner without login shell works too
* Updated: Hub utility vh is verlihub itself, on Linux it is installed as symbolic link, so existing vh commands and autostart scripts keep working, same as vh script it shows help without command and version with -v
* Updated: Hublist registration and update check run on their own threads without access to hub data
* Updated: Update check and crash report use HTTPS, small built-in HTTP client on OpenSSL that is already required, server certificate is not verified
* Updated: Hub stop no longer waits for running hublist registration or update check
* Updated: Results of worker threads are processed without waiting for adv_step_delay
* Updated: Commands gethost and getinfo, and user reports to operator chat, look up host names in background
* Updated: Login statistics of registered users and ban hits are written in background
* Updated: FindVerlihub.cmake is not installed into CMake root on Windows by default, CMake flag INSTALL_CMAKE_FILES
* Updated: Translation files, they are UTF-8 now and without codepage in file name, hub converts them to charset of its locale at runtime, CMake flag LOCALE_CONVERT converts at build time instead, po/LANGS is removed, languages are found from files
* Added: CMake target pot extracts translation templates from source, po/transifex.yml for Transifex GitHub integration
* Added: Hub messages are translated also when locale from dbconfig is not installed on system, for example ru_RU.CP1251, then other installed locale like en_US.UTF-8 is used with language of hub locale and messages are still converted to its codeset
* Fixed: Hublist registration could hang for minutes on hublist that does not answer
* Fixed: Hublist registration and crash report could send incomplete data
* Fixed: NMDC command UnBan never removed any ban
* Fixed: Poll connection chooser added up timeouts of every 1024 sockets, which delayed active connections on big hubs
* Fixed: Data past end of output buffer could be sent when compressed data was missing ending pipe
* Fixed: Command $MyIP is treated as unknown unless it comes once from enabled external TLS proxy
* Fixed: Total download and upload show real network traffic, including TLS and data that was sent later from output buffer
* Fixed: CMake did not notice missing ICU library and passed it to linker anyway
* Fixed: Upload statistics wrote outside of zone table for data sent before user zone was set, now it is added to zone of user when it is known
* Fixed: Command core_dump did not write stack backtrace to log, now it is only not sent to crash server
* Fixed: Hub aborted with terminate message when MySQL connection failed on start, now it writes error and exits
* Fixed: MySQL connection error was written twice
* Fixed: Password encryption method 1 took salt from memory instead of random, existing passwords still work
* Fixed: Hub that ended with code 3 was not restarted by vh_daemon script, now watchdog of verlihub --run restarts it, same as crash
* Fixed: Crash during hub start, for example while loading plugins, gave no stack backtrace and no crash report
* Removed: Deprecated plugins, only Lua and Plugman are left
* Removed: Shell scripts vh, vh_daemon, vh_gui, vhm, vh_regimporter, vh_migration and verlihub_config, verlihub executable does all their work, Bash and Dialog are no longer needed, vh and vhm are symbolic links to verlihub
* Removed: Old verlihub_config way of FindVerlihub.cmake, plugins built outside of Verlihub source use installed package configuration
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
