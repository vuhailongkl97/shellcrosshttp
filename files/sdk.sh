#!/bin/sh

. /usr/lib/functions

RETVAL=0
PROG=hello

start()
{
  ${PROG} &
  return $?
}

stop()
{
  killproc ${PROG}
  RETVAL=$?
  return $RETVAL
}

restart()
{
  stop
  start
}

status()
{
  pidof -o %PPID ${PROG} >/dev/null 2>&1
  RETVAL=$?
  [ $RETVAL -eq 0 ] && echo ${PROG} is running || echo ${PROG} is stopped
  return $RETVAL
}

status_sync()
{
  return 0
}

case "$1" in
  start)
  	start
	;;
  stop)
  	stop
	;;
  restart)
  	restart
	;;
  status)
  	status
	;;
  on_startup)
	start
	;;
  on_link_up)
	;;
  on_link_down)
	;;
  on_reboot)
    stop
	;;
  status_sync)
  	status_sync
	;;
  *)
	echo $"Usage: $0 {start|stop|restart|status|on_startup|on_link_up|on_link_down|on_reboot|status_sync}"
	exit 2
esac

exit $?
