#!/bin/sh
# Writes "<package>.json" next to each package that Webcamoid can update by
# itself. Usage: write-build-info.sh <packages-dir>
#
# Run it before the step that uploads the packages, so the .json files are
# uploaded along with them.
set -e

dir=$1

if [ -z "$dir" ] || [ ! -d "$dir" ]; then
    echo "Usage: $0 <packages-dir>" >&2
    exit 1
fi

commit=${GITHUB_SHA:-$(git rev-parse HEAD)}
built_at=$(date -u +%Y-%m-%dT%H:%M:%SZ)

for f in "$dir"/webcamoid-portable-linux-daily-master-x64.AppImage \
         "$dir"/webcamoid-portable-linux-daily-master-arm64.AppImage \
         "$dir"/webcamoid-installer-windows-daily-master-win64.exe \
         "$dir"/webcamoid-installer-mac-daily-master.run \
         "$dir"/webcamoid-android-daily-master.apk; do
    [ -f "$f" ] || continue
    printf '{"commit":"%s","built_at":"%s"}\n' "$commit" "$built_at" > "$f.json"
    echo "Wrote $f.json"
done
