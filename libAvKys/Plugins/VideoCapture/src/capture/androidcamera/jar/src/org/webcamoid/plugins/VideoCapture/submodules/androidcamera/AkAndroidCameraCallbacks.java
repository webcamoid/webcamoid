/* Webcamoid, camera capture application.
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

import java.util.List;
import java.util.ArrayList;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureFailure;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.media.Image;
import android.media.ImageReader;
import android.view.Surface;

public class AkAndroidCameraCallbacks implements ImageReader.OnImageAvailableListener
{
    private long m_userPtr = 0;
    private CaptureSessionCallback m_captureSessionCB;
    private CaptureSessionStateCallback m_captureSessionStateCB;
    private DeviceStateCallback m_deviceStateCB;

    private AkAndroidCameraCallbacks(long userPtr)
    {
        m_userPtr = userPtr;
        m_captureSessionCB = new CaptureSessionCallback();
        m_captureSessionStateCB = new CaptureSessionStateCallback();
        m_deviceStateCB = new DeviceStateCallback();
    }

    public CaptureSessionCallback captureSessionCB()
    {
        return m_captureSessionCB;
    }

    public CaptureSessionStateCallback captureSessionStateCB()
    {
        return m_captureSessionStateCB;
    }

    public DeviceStateCallback deviceStateCB()
    {
        return m_deviceStateCB;
    }

    public List<Surface> createSurfaceList()
    {
        return new ArrayList<Surface>();
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

    public class CaptureSessionCallback extends CameraCaptureSession.CaptureCallback
    {
        @Override
        public void onCaptureCompleted(CameraCaptureSession session,
                                       CaptureRequest request,
                                       TotalCaptureResult result)
        {
        }

        @Override
        public void onCaptureFailed(CameraCaptureSession session,
                                    CaptureRequest request,
                                    CaptureFailure failure)
        {
        }

        @Override
        public void onCaptureSequenceAborted(CameraCaptureSession session,
                                             int sequenceId)
        {
        }

        @Override
        public void onCaptureStarted(CameraCaptureSession session,
                                     CaptureRequest request,
                                     long timestamp,
                                     long frameNumber)
        {
        }

        @Override
        public void onCaptureSequenceCompleted(CameraCaptureSession session,
                                               int sequenceId,
                                               long frameNumber)
        {
        }

        @Override
        public void onCaptureProgressed(CameraCaptureSession session,
                                        CaptureRequest request,
                                        CaptureResult partialResult)
        {
        }
    }

    public class CaptureSessionStateCallback extends CameraCaptureSession.StateCallback
    {
        @Override
        public void onConfigured(CameraCaptureSession session)
        {
            sessionConfigured(m_userPtr, session);
        }

        @Override
        public void onConfigureFailed(CameraCaptureSession session)
        {
            sessionConfigureFailed(m_userPtr, session);
        }
    }

    public class DeviceStateCallback extends CameraDevice.StateCallback
    {
        @Override
        public void onOpened(CameraDevice camera)
        {
            cameraOpened(m_userPtr, camera);
        }

        @Override
        public void onDisconnected(CameraDevice camera)
        {
            cameraDisconnected(m_userPtr, camera);
        }

        @Override
        public void onError(CameraDevice camera, int error)
        {
            cameraFailed(m_userPtr, camera, error);
        }
    }

    private static native void sessionConfigured(long userPtr,
                                                 CameraCaptureSession session);
    private static native void sessionConfigureFailed(long userPtr,
                                                      CameraCaptureSession session);
    private static native void cameraOpened(long userPtr,
                                            CameraDevice camera);
    private static native void cameraDisconnected(long userPtr,
                                                  CameraDevice camera);
    private static native void cameraFailed(long userPtr,
                                            CameraDevice camera,
                                            int error);
    private static native void imageAvailable(long userPtr, Image image);
}
