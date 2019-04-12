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

import android.view.SurfaceHolder;

public class AkSurfaceHolderCallback implements SurfaceHolder.Callback
{
    private long m_userPtr = 0;

    public AkSurfaceHolderCallback(long userPtr)
    {
        m_userPtr = userPtr;
    }

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

    private static native void notifySurfaceCreated(long userPtr);
    private static native void notifySurfaceDestroyed(long userPtr);
}
