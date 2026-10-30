# 1.7.0.0

* Added: Try to detect external TLS proxy when running on same server
* Added: Configuration tls_force_myip to force external TLS proxy
* Added: Lua 5.5 support
* Added: Consider users with hidden chat on welcome message
* Added: Worker threads for blocking work, configuration adv_worker_threads (0 = all CPU cores)
* Added: Host name lookup on worker threads, with dns_lookup new connection waits up to 5 seconds for host name
* Added: Database thread with its own connection for writes and background reads, queued writes are done before hub stops
* Added: Configuration use_banlist_cache, ban check on login without database query, ban table changed by VH:SQLQuery of Lua script reloads cache at once, so script ban and unban work as before, other programs that change database are seen after timer_reloadcfg_period
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
* Added: Lua calls VH:HTTPGet(url, func, [headers], [timeout]), VH:HTTPPost(url, data, func, [headers], [timeout]) and VH:HTTPDownload(url, file, func, [headers], [timeout]), they return at once with request id and call script function func(id, code, body, head, err) later, so scripts no longer need curl or wget and hub is not blocked
* Added: HTTP requests of scripts run on own two threads, follow two redirects, timeout is seconds without data, headers can be table or string, download saves to file relative to configuration folder through file.part, so failed download keeps old file
* Added: HTTP code is status of reply, or negative reason when there is no reply: -1 timeout, -2 connection refused, -3 host not found, -4 TLS handshake failed, -5 broken reply, -6 file can not be written, -7 hub is stopping, -8 other network error
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
* Updated: Login statistics of registered users and ban hits are written in background, every query on main connection, also VH:SQLQuery of Lua script, first waits up to 3 seconds for them, so script reads same values as when they were written at once
* Updated: FindVerlihub.cmake is not installed into CMake root on Windows by default, CMake flag INSTALL_CMAKE_FILES
* Updated: Translation files, they are UTF-8 now and without codepage in file name, hub converts them to charset of its locale at runtime, CMake flag LOCALE_CONVERT converts at build time instead, po/LANGS is removed, languages are found from files
* Added: CMake target pot extracts translation templates from source, po/transifex.yml for Transifex GitHub integration
* Added: Hub messages are translated also when locale from dbconfig is not installed on system, for example ru_RU.CP1251, then other installed locale like en_US.UTF-8 is used with language of hub locale and messages are still converted to its codeset
* Added: Commands of verlihub executable are translated to language of system environment, on Windows console they are shown as UTF-8
* Updated: Regular expressions use PCRE2 library instead of old PCRE, which new Linux distributions no longer have, required package is libpcre2-dev on Debian, mingw-w64-ucrt-x86_64-pcre2 on Windows
* Updated: Main database connection reconnects by itself when connection is lost, for example after database restart, at most once in 5 seconds with 3 seconds timeout, operators get OpChat message when it is lost and back, log shows failed attempts once a minute, deprecated automatic reconnect of client library and its warning are gone
* Updated: Command sysinfo shows memory usage reported by database server itself instead of ps and pgrep, so it works on Windows and with database on other machine, on MySQL account needs select permission on performance_schema.memory_summary_global_by_event_name, verlihub --install grants it when administrator credentials are asked, see Install.md
* Updated: On BSD command sysinfo shows highest resident memory of hub and no virtual memory, without running ps
* Fixed: Translation with wrong format arguments could crash the hub, for example %s instead of %d, such translation is not used now and original text is shown instead
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
* Fixed: On Windows dates with microseconds, for example time in log lines, were read as wrong time, also date printing is thread safe now
* Fixed: Mean upload, download and main loop frequency were up to half too low, because of integer division, too low frequency could set hub load status and block searches of users without real load
* Fixed: Time of hub start, loading of database, plugins and scripts, was counted as slow main loop, so right after start hub load status could block searches and refuse new connections
* Fixed: Main loop frequency showed 0 for few seconds after every timer_reloadcfg_period
* Fixed: IP range with mask, for example 1.2.3.0/24, started from wrong address, invalid addresses in range are refused now
* Fixed: Commands unbanrange and infobanrange did not find range ban by its range, now range finds that same ban and single address still finds all ranges that contain it
* Fixed: Command getconfig put file name into database query without escaping, so admin could read other tables, for example passwords of masters
* Fixed: Kick search did not escape operator nick, so ban of recently kicked user failed for operator with quote or backslash in nick
* Fixed: Database thread sent query again when connection was lost during query, so for example login counter could be increased twice, now only query that was not sent is repeated, same as main connection
* Fixed: On Windows connection to hublist, update check and crash report server waited about 21 seconds instead of 5 seconds for server that does not answer
* Fixed: Lua script call that started other script calls, for example VH:ParseCommand or VH:ScriptCommand, had its own stack cleared, so its strings could be freed while still in use and hub could crash
* Fixed: Unloading or reloading Lua script from inside script call, also script itself, deleted running script and changed script list during calls, which could crash hub, now unloaded script gets no more calls and is deleted when no script runs
* Fixed: VH:SQLFetch wrote past end of Lua stack for query with more than 19 columns, for example all columns of reglist
* Fixed: VH:GetLuaBots returned only last bot of each script and left holes in list
* Fixed: Variable with empty value in dbconfig, for example locale =, took value of previous line when it was last line, or next line as its value, which was then lost
* Fixed: Trigger file with Windows line ends, for example edited on Windows and copied to Linux, was sent with double carriage return
* Fixed: ZLib compression wrote past end of its output buffer when compressed data was bigger than 256 kb, output buffer size was increased but buffer itself was never reallocated
* Fixed: Protocol data of client that did not wait for $Lock was parsed before connection was started, while host name was looked up, so ban and script checks of new connection were skipped and user count of zone was changed outside of its table, which later could refuse all users of that zone, now such data is parsed after connection is started
* Fixed: User whose read was blocked because of full output buffer was never read again when rest of buffer was sent at once
* Fixed: Kick, ban and redirect messages were often lost, connection was closed at once when user sent anything more or when one send could not write data, instead of waiting for close time
* Fixed: Changing hub_security or opchat_name at runtime kept bot under old nick, so it could not be reached and new nick was not reserved
* Fixed: Removing script bot that had nick of registered user, who logged in meanwhile, removed that user from user list and could crash hub later
* Fixed: New users got old MyINFO of users who changed it, and old IP after fake IP was set, until next logout
* Fixed: Single message bigger than adv_max_outbuf_size, for example nick list of hub with thousands of users, disconnected new user with output buffer is too big, now it is allowed when nothing else is waiting
* Fixed: Protocol command ending in same read could be longer than max_message_size
* Fixed: Changed MyINFO that came within int_myinfo was dropped, so others kept old one while total share already counted new one, now newest one is sent when interval is over
* Fixed: Plugins and scripts got OnCloseConn for connections that never got OnNewConn, for example failed TLS handshake or connection refused before it, so counting of connections went wrong
* Fixed: Dead user without closed connection, for example lost network, stayed in hub until system gave up sending, often 15 to 30 minutes, because pings of hub still went into system buffer, now logged in user who sends nothing for 270 seconds is closed with general timeout, clients ping every minute
* Fixed: Error on listening socket used connection list position that was never set, now it is logged and hub keeps listening
* Fixed: Command bufinfo counted only buffers of users in list, now it counts all connections, also those that log in or close, and shows data waiting for connection start and TLS
* Fixed: Reply to $GetINFO was not sent at once
* Fixed: Waiting of new connections for host name and TLS used clock time, so clock set back, for example by time sync, stopped new connections that long
* Fixed: Connection closed before it was registered or after it was removed left stale entry in connection chooser, with epoll main loop stopped sleeping and used full CPU
* Fixed: Lowering class of user with show_keys below operator class removed operator key, now it stays, same as on login
* Updated: Login no longer flushes data of all users at once, so delayed_myinfo works for logins too, timer still sends it every second in same order, this saves a lot of CPU when many users login after hub start
* Updated: ZLib reuses one compression stream, before it was set up again for every compressed flush
* Fixed: TCP urgent data from client gave read event that hub could not read, so main thread waited in read retries and then closed user, urgent data is not watched now
* Fixed: Database thread queue had no limit, when database was slow or down statistics writes piled up in memory and hub stop waited for all of them, now queue holds 10000 queries, more writes are dropped, hub stop waits at most 5 seconds
* Updated: Database logs only lost queries, failed after retry or dropped, as one line once a minute on level 0, slow database is not logged, lost and back main connection is logged only when reconnect failed
* Fixed: Main database connection had no read and write timeout, so database that accepted query and did not answer froze whole hub, now query fails after 10 seconds and connection is reconnected
* Fixed: CTM2HUB detection kept one item for every filtered connection and searched all of them for each new one, so long attack used hundreds of megabytes and more CPU every minute, now it keeps every hub address once and counts connections, reports stay same
* Fixed: Data received during TLS handshake was kept without limit, now connection is closed when it is bigger than read buffer, real handshake is few kilobytes
* Fixed: Ban check of ban cache parsed IP address of user again for every range, share and prefix ban on every login, now range bans use IP number of connection
* Updated: LAN detection and GeoIP lookup of user use IP number of connection instead of parsing address again, IP zone ranges are converted once when configuration is loaded or set instead of on every new connection
* Fixed: Lua callback number arguments were made with malloc and freed with delete in builds with USE_CUSTOM_AUTOSPRINTF, and share size of VH_OnNewBan was cut to 32 bits on Windows, now arguments are strings of callback itself
* Updated: Partial send of output buffer moves send position instead of copying all unsent data every time, sent part is cut off when it is more than half of buffer, so big nick list to slow user is copied about once
* Updated: Delayed messages to slow user with more than half of adv_max_outbuf_size waiting are not flushed one by one while socket is full, they wait for socket to be writable and go out compressed together
* Updated: Timer flushes delayed messages once over user list, users of active, passive, chat and opchat lists are in it, so same users are not walked four more times every second
* Updated: Connection timer is spread over 20 parts of adv_timer_conn_period, every connection is still handled once per period, but big hub does not handle all connections in one main loop step, flush of delayed messages still runs for all users at once, so same waiting data is compressed once
* Updated: GeoIP cache keeps at most 100000 addresses, about 20 MB, when it is full oldest tenth is removed, also with mmdb_cache_mins 0
* Updated: Message parser of connection frees memory of big message, bigger than 4 KB, before next one, small messages keep their buffer
* Updated: HTTP reply without chunks goes from receive buffer straight to body or file, body with known size up to 16 MB is allocated once
* Updated: New ban in ban cache is found and added by index, without walking all bans and building whole index again, removing bans still builds it again
* Updated: Broadcast and triggers with variables check once which variables are in text, for each user only those are replaced, text without variables is sent without searching it
* Fixed: Broken wakeup socket pair of worker threads on Windows could make main loop use whole CPU core, now it is logged once and main loop sleeps step delay and handles worker results once per step
* Fixed: Command userlimit crashed hub with adv_timer_serv_period 0, finished userlimit functions left empty slots that timer walked every second
* Updated: Nick lookup in user lists makes lower case hash directly from nick without copy of nick, value stays same, so nicks are found case insensitive as before
* Updated: Login sends kept nick, MyINFO, operator, bot and IP lists straight from user lists to output buffer, without copy of whole list for every user, pipe is added in buffer, so kept list is not changed
* Updated: Kept user lists are made again into their old memory without reallocations, memory is freed only when it is more than twice of list, so lists still shrink when users leave at night
* Updated: With myinfo_tls_filter MyINFO list without TLS flags for users without TLS is kept and made again only when MyINFO list changed, instead of building it from all users for every such login, TLS flag is taken from MyINFO that is really sent
* Fixed: User who is online when command ban hits him gets same ban message as on login and is closed after it is sent, also redirect for kick reason is sent when it is enabled, before connection was closed at once without message or redirect
* Fixed: NMDC command SetTopic changed topic only in memory, users did not get new hub name and next configuration reload brought old topic back, now it works same as command topic, with same rights, length limit, plugin check, saving and message
* Updated: Lua call VH:SendPMToAll(data, [from], [min_class], [max_class]) has optional sender nick, missing, nil or empty sends message from hub security
* Fixed: Main loop frequency and upload counters stood still at old values after system clock went back, for example by time sync, now they start again at once, upload counters are no longer reset every timer_reloadcfg_period, which made them show too low values for few seconds
* Updated: Build shows warning -Woverloaded-virtual again, hidden virtual functions that are known to be safe are marked in code with reason
* Removed: Conversion of time to microseconds as long, it overflowed on Windows and was not used
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
