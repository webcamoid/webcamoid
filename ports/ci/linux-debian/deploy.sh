#!/bin/bash

# Webcamoid, webcam capture application.
# Copyright (C) 2017  Gonzalo Exequiel Pedone
#
# Webcamoid is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamoid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

git clone https://github.com/webcamoid/DeployTools.git

DEPLOYSCRIPT=deployscript.sh
export PYTHONPATH=${PWD}/DeployTools

cat << EOF > ${DEPLOYSCRIPT}
#!/bin/sh

export PATH="\$PWD/.local/bin:\$PATH"
export PYTHONPATH="\${PWD}/DeployTools"
export DAILY_BUILD=${DAILY_BUILD}
xvfb-run --auto-servernum python3 DeployTools/deploy.py \
    -d "\${PWD}/webcamoid-data" \
    -c "\${PWD}/build/package_info.conf" \
    -o "\${PWD}/webcamoid-packages/linux"
EOF

chmod +x ${DEPLOYSCRIPT}
${EXEC} bash ${DEPLOYSCRIPT}
