Usage:

  netmon [-t <check_interval>] [-n <max_failure>] [-l <log_file>]
         [-c <cmd>] [-p <ping_host>] [-d]
  
  -t <check_interval>  specify how many seconds to wait between two checks
  -n <max_failure>     specify how many continuous network failures we get
                       before we reboot the system
  -l <log_file>        specify the log file
  -c <cmd>             the command line to be executed when
                       network failure is detected
  -p <ping_host>       test the network by pinging given host
  -d                   run as a daemon process


Debugging:

  Declare macro `DEBUG` to enable debug level logging.
  This is enabled in CMake task by default.
