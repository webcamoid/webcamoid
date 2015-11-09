#
# Regular cron jobs for the webcamoid package
#
0 4	* * *	root	[ -x /usr/bin/webcamoid_maintenance ] && /usr/bin/webcamoid_maintenance
