ulimit -c unlimited >/dev/null 2>&1
sysctl -w fs.suid_dumpable=2
