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

import java.nio.ByteBuffer;
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
        Image image = imageReader.acquireLatestImage();
        Image.Plane[] planes = image.getPlanes();
        byte[] data = new byte[0];

        for (Image.Plane plane: planes) {
            ByteBuffer buffer = plane.getBuffer();
            buffer.rewind();
            byte[] newData = new byte[buffer.remaining()];
            buffer.get(newData);
            byte[] concatData = new byte[data.length + newData.length];
            System.arraycopy(data, 0, concatData, 0, data.length);
            System.arraycopy(newData, 0, concatData, data.length, newData.length);
            data = concatData;
        }

        int format = image.getFormat();
        int width = image.getWidth();
        int height = image.getHeight();
        long timestampNs = image.getTimestamp();
        image.close();

        if (data.length < 1)
            return;

        imageAvailable(m_userPtr,
                       format,
                       width,
                       height,
                       timestampNs,
                       data);
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

    private static native void imageAvailable(long userPtr,
                                              int format,
                                              int width,
                                              int height,
                                              long timestampNs,
                                              byte[] data);
    private static native void captureStopped(long userPtr);
}
