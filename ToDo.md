* zLib: data that does not end with pipe is not compressed and stays in flush buffer until next write, also timer flush can not send it, see "Missing ending pipe in compress data" in cAsyncConn::Write()
* System info: MySQL/MariaDB memory usage is found with ps and pgrep, which Windows does not have, find database server process another way, for example by pid_file server variable when database runs on same machine, see cInfoServer::SystemInfo()
* Windows: test stack backtrace on crash with "!core_dump", it forces crash and writes backtrace to log, but does not send it to crash server
