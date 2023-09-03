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

package org.webcamoid.plugins.DesktopCapture.submodules.androidscreen;

import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;

public class AkAndroidScreenCallbacks implements ImageReader.OnImageAvailableListener
{
    private long m_userPtr = 0;

    public AkAndroidScreenCallbacks(long userPtr)
    {
        m_userPtr = userPtr;
    }

    public MediaProjectionCallback mediaProjectionCallback()
    {
        return new MediaProjectionCallback();
    }

    // ImageReader.OnImageAvailableListener

    @Override
    public void onImageAvailable(ImageReader imageReader)
    {
        try {
            Image image = imageReader.acquireLatestImage();
            imageAvailable(m_userPtr, image);
            image.close();
        } catch (Exception e) {
        }
    }

    // MediaProjection.Callback

    private class MediaProjectionCallback extends MediaProjection.Callback
    {
        @Override
        public void onStop()
        {
            captureStopped(m_userPtr);
        }
    }

    private static native void imageAvailable(long userPtr, Image image);
    private static native void captureStopped(long userPtr);
}
