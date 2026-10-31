# 2.0.0.0

## Added

* External TLS proxy is used when listen_ip equals tls_listen_ip, configuration tls_force_myip to force it
* Lua 5.5 support
* Consider users with hidden chat on welcome message
* Multithreading for MySQL, host lookup and other slow work, configuration adv_worker_threads
* Configuration use_banlist_cache, ban check on login without database query
* Custom MySQL port with db_host = <host>:<port>
* Configuration db_tls to require encrypted MySQL connection
* Configuration db_socket for custom MySQL socket path
* Epoll connection chooser by default, CMake flag USE_EPOLL
* Windows support using MinGW-w64, see Install.md
* CMake package configuration for plugins built outside of Verlihub source
* Native TLS using OpenSSL, plain and encrypted clients on same port
* Lua calls VH:HTTPGet, VH:HTTPPost and VH:HTTPDownload, see Wiki
* Commands !getvar and !getconfig list unused variables separately
* CMake target POT and Transifex integration
* Hub messages are translated also when dbconfig locale is not installed
* Configuration classdif_rights for rights and restrictions commands

## Updated

* Wiki is rewritten to match current version: https://github.com/verlihub/verlihub/wiki
* Shell scripts are replaced by "verlihub" executable itself, see "verlihub --help"
* Syslog options are only long --syslog and --syslog-suffix
* Multi hub manager "vhm" is "verlihub" itself
* Hub utility "vh" is "verlihub" itself, old "vh" commands keep working
* Update check and crash report use HTTPS
* Hub stop no longer waits for hublist registration or update check
* Translation files are UTF-8, converted to locale charset at runtime
* PCRE2 instead of old PCRE, see Install.md
* Main database connection reconnects by itself
* MySQL memory in command !sysinfo is read directly from its engine, see Install.md
* Command !sysinfo on BSD without "ps"
* Login no longer flushes data of all users at once
* ZLib reuses one compression stream
* LAN detection, GeoIP and IP zones use IP number of connection
* Partial send no longer copies unsent data every time
* Delayed messages to slow user wait for writable socket
* Timer flushes delayed messages once over user list
* Connection timer is spread over 20 parts of adv_timer_conn_period
* GeoIP cache keeps at most 100000 addresses
* Message parser frees memory of big messages
* Variables in broadcast and triggers are found once
* Nick hash is made without copy of nick
* Login sends kept lists without copying them
* Kept user lists are rebuilt without reallocations
* MyINFO list without TLS flags is kept for myinfo_tls_filter
* Lua call VH:SendPMToAll has optional sender
* Build shows -Woverloaded-virtual warnings again
* Ban messages show banned value with its own name
* Lua call VH:Ban behavior, see Wiki
* Command !regset follows classdif_reg like other reg commands
* Command !triggerget is now !gettrigger for administrators
* Broadcast commands to staff reach their class and higher, sent by same class or higher
* Command !ccbroadcast uses min_class_bc like !broadcast
* Command !ban and Lua call VH:Ban type 0 ban nick and IP of online user
* Help commands !h are complete and translated
* New database created by installation uses same charset as hub connection
* Command !reload also reloads connection types
* Welcome messages accept %[NICK] same as triggers and broadcasts
* Default triggers, redirects, clients and connection types are added to new tables and once after upgrade, deleted ones stay deleted
* Command !getvar finds variables that contain given text in name or value, without "*" around it

## Fixed

* Translation with wrong format arguments could crash hub
* Hublist registration could hang for minutes on hublist that does not answer
* Hublist registration and crash report could send incomplete data
* NMDC command $UnBan never removed any ban
* Poll connection chooser delayed active connections on big hubs
* Data past end of output buffer could be sent
* NMDC command $MyIP is accepted only from enabled external TLS proxy
* Total download and upload show real network traffic
* CMake did not notice missing ICU library
* Upload statistics wrote outside of zone table
* Command !core_dump did not write stack backtrace to log
* Hub aborted when MySQL connection failed on start
* MySQL connection error was written twice
* Password encryption method 1 used salt from memory instead of random
* Hub that ended with code 3 was not restarted
* Crash during hub start gave no stack backtrace and crash report
* Mean upload, download and main loop frequency were too low
* Hub start was counted as slow main loop
* Main loop frequency showed 0 for few seconds after every timer_reloadcfg_period
* IP range with mask started from wrong address
* Commands !unbanrange and !infobanrange did not find range ban by its range
* Command !getconfig did not escape file name in database query
* Kick search did not escape operator nick
* Possible crash in nested Lua script calls
* Possible crash when Lua script was unloaded from inside script call
* Lua call VH:SQLFetch wrote past end of Lua stack for more than 19 columns
* Lua call VH:GetLuaBots returned only last bot of each script
* Empty value in dbconfig took value of other line
* Trigger file with Windows line ends was sent with double carriage return
* ZLib compression wrote past end of output buffer
* User with blocked read was never read again
* Kick, ban and redirect messages were often lost
* Changing hub_security or opchat_name at runtime kept bot under old nick
* Removing script bot with nick of registered user could crash hub
* New users got old MyINFO and old IP of users who changed them
* Single message bigger than adv_max_outbuf_size disconnected new user
* Protocol command ending in same read could be longer than max_message_size
* Changed MyINFO within int_myinfo was dropped
* Plugins and scripts got OnCloseConn without OnNewConn
* Error on listening socket used unset connection list position
* Command !bufinfo counted only users in list
* Reply to NMDC command $GetINFO was not sent at once
* Lowering class of user with show_keys removed operator key
* TCP urgent data from client closed user
* Database that did not answer froze whole hub
* CTM2HUB detection used too much memory and CPU during long attack
* Lua callback arguments mixed malloc and delete
* Command !userlimit crashed hub with adv_timer_serv_period 0
* User hit by command !ban got no ban message or redirect
* NMDC command $SetTopic works same as command !topic
* Main loop frequency and upload counters stood still after clock went back
* Configuration timeout_setpass had no effect, timeout_flush is removed
* Configuration zlib_compress_level -1 compressed at level 1
* With mmdb_conv_depth 0 all names were shown as "--"
* Slots per hub ratio lost its fraction
* Configuration show_desc_len cut tag too
* Lua call VH:ParseCommand always ran its first command
* Bots of Lua script stayed after script was stopped
* Lua calls VH:UnRegBot and VH:EditBot could change bots of other scripts
* Lua GeoIP calls gave different results for unknown address, see Wiki
* GeoIP cache kept lookups with own database file of script
* Lua call VH:SendToChat ignored minimum class alone and doubled pipe
* Lua call VH:IsBot was false for hub security and operator chat
* Bot share above 2 GB in Lua call VH:GetBots
* Lua calls VH:AddRegUser and VH:SetRegClass allow same classes as reg commands
* Lua call VH:SetTopic sent topic before saving it
* Lua call VH:KickRedirUser kept redirect when kick was refused
* Ban with time 0 from Lua call VH:Ban or NMDC command $Ban was not permanent
* Unknown ban type made host level 1 ban
* Lua event returning false did not discard
* Lua values VH.HubSec and VH.OpChat changed before hub accepted new nick
* Possible hub crash when Lua script replaced VH table
* Host ban of user without host name banned every user without host
* Command !ban checked class only for host level 1
* Commands !ban, !banip and !banshare saved invalid values as bans that matched nobody
* Command !ban dropped online users only for IP, range and nick bans
* Possible crash in ban information with unknown ban_type
* Trigger, redirect, client, connection type, Plugman and Lua commands worked only with "!" prefix
* Lua plugin settings changed with command !set were used only after plugin reload
* Possible crash after command !delconntype, and changed connection speed was used only after reconnect
* File trigger with uppercase letters in name could not be used
* Rights and restrictions commands had no class check of target user
* Right !noshare had no effect, user with it can share less than minimum
* User with any temporary right got back search and download taken by share, class or client limits

## Removed

* NMDC commands $MultiSearch and $MultiConnectToMe
* Configurations max_extra_ops, max_extra_cheefs and max_extra_admins
* Command !oc, same as !ops
* Unused time conversion to microseconds
* Deprecated plugins, only Lua and Plugman are left
* Shell scripts "vh", "vh_daemon", "vh_gui", "vhm", "vh_regimporter", "vh_migration" and "verlihub_config"
* Old "verlihub_config" way of FindVerlihub.cmake
* Old thread classes cThread, cWorkerThread, cThreadWork and cMutex
* Built-in Go TLS proxy and FearTLS library, with configurations tls_listen_port and tls_buf_size

# 1.6.1.0

## Added

* More useful output on CMake configuration
* Manual reconnect to MySQL on lost connection

## Fixed

* Multithreading got locked by TLS proxy

# 1.6.0.0

## Added

* Built-in TLS proxy instead of standalone command line utility, requires Go
* New "vh" command line utility options --getconf and --setconf

# Older

Please follow commit log: https://github.com/verlihub/verlihub/commits/master
