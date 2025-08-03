/* Webcamoid, camera capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

package org.webcamoid.webcamoidutils;

import java.lang.String;
import android.media.MediaScannerConnection;
import android.net.Uri;

public class WebcamoidUtils implements MediaScannerConnection.OnScanCompletedListener
{
    private long m_userPtr = 0;

    public WebcamoidUtils(long userPtr)
    {
        m_userPtr = userPtr;
    }

    // MediaScannerConnection.OnScanCompletedListener

    @Override
    public void onScanCompleted(String path, Uri uri)
    {
        scanCompleted(m_userPtr, path, uri);
    }

    private static native void scanCompleted(long userPtr, String path, Uri uri);
}
