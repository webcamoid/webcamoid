/* Webcamoid, webcam capture application.
 * Copyright (C) 2019  Gonzalo Exequiel Pedone
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
import android.view.SurfaceHolder;

public class AkAndroidCameraCallbacks implements Camera.PreviewCallback,
                                                 Camera.ShutterCallback,
                                                 Camera.PictureCallback,
                                                 SurfaceHolder.Callback
{
    private long m_userPtr = 0;
    private byte[] m_lastPreviewBuffer = null;
    private int m_index = 0;

    private AkAndroidCameraCallbacks(long userPtr)
    {
        m_userPtr = userPtr;
    }

    public void resetPictureIndex()
    {
        m_index = 0;
    }

    // Camera.PreviewCallback

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

    // Camera.ShutterCallback

    @Override
    public void onShutter()
    {
        shutterActivated(m_userPtr);
    }

    // Camera.PictureCallback

    @Override
    public void onPictureTaken(byte[] data, Camera camera)
    {
        if (data != null)
            pictureTaken(m_userPtr, m_index, data);

        m_index++;
    }

    // SurfaceHolder.Callback

    @Override
    public void surfaceChanged(SurfaceHolder holder,
                               int format,
                               int width,
                               int height)
    {
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder)
    {
        notifySurfaceCreated(m_userPtr);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder)
    {
        notifySurfaceDestroyed(m_userPtr);
    }

    private static native void previewFrameReady(long userPtr, byte[] data);
    private static native void notifySurfaceCreated(long userPtr);
    private static native void notifySurfaceDestroyed(long userPtr);
    private static native void shutterActivated(long userPtr);
    private static native void pictureTaken(long userPtr,
                                            int index,
                                            byte[] data);
}
