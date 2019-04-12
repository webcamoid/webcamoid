/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

package org.webcamoid.plugins.VideoCapture.submodules.androidcamera;

import android.hardware.Camera;

public class AkCameraPreviewCallback implements Camera.PreviewCallback
{
    private long m_userPtr = 0;
    private byte[] m_lastPreviewBuffer = null;

    private AkCameraPreviewCallback(long userPtr)
    {
        m_userPtr = userPtr;
    }

    @Override
    public void onPreviewFrame(byte[] data, Camera camera)
    {
        // Re-enqueue the last buffer
        if (m_lastPreviewBuffer != null)
            camera.addCallbackBuffer(m_lastPreviewBuffer);

        m_lastPreviewBuffer = data;

        if (data != null)
            previewFrameReady(m_userPtr, data);
    }

    private static native void previewFrameReady(long userPtr, byte[] data);
}
