from distutils.core import setup


setup(
    name='v4l2',
    version='0.2',
    license='GPLv2',
    requires=('ctypes',),
    py_modules=('v4l2',),

    maintainer='python-v4l2-devel',
    maintainer_email='https://launchpad.net/~python-v4l2-devel/+contactuser',
    url='http://pypi.python.org/pypi/v4l2',
    keywords='v4l2 video4linux video4linux2 binding ctypes',
    description='Python bindings for the v4l2 userspace api.',

    classifiers=(
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License (GPL)',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Topic :: Multimedia :: Video',
        'Topic :: Multimedia :: Video :: Capture',
    ),
)
