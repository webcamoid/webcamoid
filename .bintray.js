{
    "package": {
        "name": "webcamoid",
        "repo": "webcamoid",
        "subject": "webcamoid",
        "desc": "Daily build",
        "website_url": "https://webcamoid.github.io",
        "issue_tracker_url": "https://github.com/webcamoid/webcamoid/issues",
        "vcs_url": "https://github.com/webcamoid/webcamoid.git",
        "licenses": ["GPLv3+"],
        "labels": ["webcam", "virtual-camera", "desktop-capturing"],
        "public_download_numbers": true,
        "public_stats": true
    },

    "version": {
        "name": "daily",
        "desc": "Daily build",
        "gpgSign": false
    },

    "files": [
        {"includePattern": "/home/travis/build/webcamoid/webcamoid/ports/deploy/packages_auto/(.*)/(.*)",
         "uploadPattern": "$1/$2",
         "matrixParams": {"override": 1}}
    ],

    "publish": true
}
