python-v4l2
===========

A Python binding for the v4l2 (video4linux2) userspace api, using
ctypes.  Basic example usage::

    >>> import v4l2
    >>> import fcntl
    >>> vd = open('/dev/video0', 'rw')
    >>> cp = v4l2.v4l2_capability()
    >>> fcntl.ioctl(vd, v4l2.VIDIOC_QUERYCAP, cp)
    0
    >>> cp.driver
    'uvcvideo'
    >>> cp.card
    'USB 2.0 Camera'

See the ``linux/videodev2.h`` header file for details.  Currently the
bindings are up to date with the 2.6.34 kernel headers.

* `Video for Linux Two Specification <http://linuxtv.org/downloads/v4l-dvb-apis/ch07s02.html>`_
* `Reporting bugs <http://bugs.launchpad.net/python-v4l2>`_
