/* Webcamoid, webcam capture application.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

import QtQuick 2.7
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3

ColumnLayout {
    id: recRecordBar

    function sort(model, lo, hi)
    {
        if (lo >= hi)
            return;

        var pivot = lo;
        var pivotDesc = model.get(pivot).description.toLowerCase();

        for (var j = pivot + 1; j < hi; j++)
            if (model.get(j).description.toLowerCase() < pivotDesc) {
                model.move(j, lo, 1);
                pivot++;
            }

        sort(model, lo, pivot);
        sort(model, pivot + 1, hi);
    }

    function indexOfFormat(format)
    {
        var lo = 0;
        var mid = lsvRecordingFormatList.model.count >> 1;
        var hi = lsvRecordingFormatList.model.count;

        while (mid !== lo || mid !== hi) {
            if (lsvRecordingFormatList.model.get(mid).format == format)
                return mid;
            else if (lsvRecordingFormatList.model.get(mid).format < format) {
                lo = mid + 1;
                mid = (lo + hi) >> 1;
            } else if (lsvRecordingFormatList.model.get(mid).format > format) {
                hi = mid;
                mid = (lo + hi) >> 1;
            }
        }

        return -1
    }

    function updateRecordingFormatList(availableFormats)
    {
        var recordingFormat = Recording.format;
        lsvRecordingFormatList.model.clear();

        for (var format in availableFormats) {
            lsvRecordingFormatList.model.append({
                format: availableFormats[format],
                description: Recording.formatDescription(availableFormats[format])});
        }

        sort(lsvRecordingFormatList.model, 0, lsvRecordingFormatList.model.count);
        lsvRecordingFormatList.currentIndex = indexOfFormat(recordingFormat);
    }

    Component.onCompleted: updateRecordingFormatList(Recording.availableFormats)

    Connections {
        target: Recording

        onAvailableFormatsChanged: updateRecordingFormatList(availableFormats)
    }

    TextField {
        id: txtSearchFormat
        placeholderText: qsTr("Search format")
        Layout.fillWidth: true
    }
    OptionList {
        id: lsvRecordingFormatList
        filter: txtSearchFormat.text
        textRole: "description"
        Layout.fillWidth: true

        onCurrentIndexChanged: {
            var option = model.get(currentIndex);

            if (option)
                Recording.format = option.format

            txtSearchFormat.text = "";
        }
    }
}
