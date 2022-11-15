package org.tensorflow.lite.examples.detection;

/*
 * Copyright 2019 The TensorFlow Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import android.annotation.SuppressLint;
import android.app.Fragment;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Matrix;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.Size;
import android.util.SparseIntArray;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;

import com.hsj.camera.IFrameCallback;
import com.hsj.camera.UsbCamera;

import org.tensorflow.lite.examples.detection.R;
import org.tensorflow.lite.examples.detection.customview.AutoFitTextureView;
import org.tensorflow.lite.examples.detection.env.ImageUtils;
import org.tensorflow.lite.examples.detection.env.Logger;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;

@SuppressLint("ValidFragment")
public class LegacyCameraConnectionFragment extends Fragment {
  private static final Logger LOGGER = new Logger();
  /** Conversion from screen rotation to JPEG orientation. */
  private static final SparseIntArray ORIENTATIONS = new SparseIntArray();

  static {
    ORIENTATIONS.append(Surface.ROTATION_0, 90);
    ORIENTATIONS.append(Surface.ROTATION_90, 0);
    ORIENTATIONS.append(Surface.ROTATION_180, 270);
    ORIENTATIONS.append(Surface.ROTATION_270, 180);
  }
Context context;
  //TODO Config Usb Camera vid, pid
  private int vid, pid;
  // Frame width, height
  public static int width, height;

  private UsbCamera camera;
  private Camera.PreviewCallback imageListener;
  private Size desiredSize;
  /** The layout identifier to inflate for this Fragment. */
  private int layout;
  /** An {@link AutoFitTextureView} for camera preview. */
  //public AutoFitTextureView textureView; for auto correction of width and height
  public TextureView textureView;

  /**
   * {@link TextureView.SurfaceTextureListener} handles several lifecycle events on a {@link
   * TextureView}.
   */
  private final TextureView.SurfaceTextureListener surfaceTextureListener =
          new TextureView.SurfaceTextureListener() {
            @Override
            public void onSurfaceTextureAvailable(
                    final SurfaceTexture texture, final int width, final int height) {
              surface = new Surface(texture);

              ///manually taken from another app
              vid=6408;
              pid=8977;
              context=CameraActivity.getInstance().getContext();
              startCamera();
            }

            @Override
            public void onSurfaceTextureSizeChanged(
                    final SurfaceTexture texture, final int width, final int height) {}

            @Override
            public boolean onSurfaceTextureDestroyed(final SurfaceTexture texture) {
              return true;
            }

            @Override
            public void onSurfaceTextureUpdated(final SurfaceTexture texture) {}
          };
  /** An additional thread for running tasks that shouldn't block the UI. */
  private HandlerThread backgroundThread;

  public LegacyCameraConnectionFragment(
          final Camera.PreviewCallback imageListener, final int layout, final Size desiredSize) {
    this.imageListener = imageListener;
    this.layout = layout;
    this.desiredSize = desiredSize;
  }
View rootview;
  @Override
  public View onCreateView(
          final LayoutInflater inflater, final ViewGroup container, final Bundle savedInstanceState) {
    rootview=inflater.inflate(layout, container, false);
    textureView = (TextureView) rootview.findViewById(R.id.textureView2);
    //refer orginal project for clarification.

    return rootview;
  }

  @Override
  public void onViewCreated(final View view, final Bundle savedInstanceState) {
    textureView = (TextureView) rootview.findViewById(R.id.textureView2);
    //textureView=(AutoFitTextureView)view.findViewById(R.id.texture);for autofit function
  }


  @Override
  public void onActivityCreated(final Bundle savedInstanceState) {
    super.onActivityCreated(savedInstanceState);
  }

  @Override
  public void onResume() {
    super.onResume();
    startBackgroundThread();
    // When the screen is turned off and turned back on, the SurfaceTexture is already
    // available, and "onSurfaceTextureAvailable" will not be called. In that case, we can open
    // a camera and start preview from here (otherwise, we wait until the surface is ready in
    // the SurfaceTextureListener).

    if (textureView.isAvailable()) {
      System.out.println("");
      //startCamera();
    } else {
      textureView.setSurfaceTextureListener(surfaceTextureListener);
    }
  }

  @Override
  public void onPause() {
    stopCamera();
    stopBackgroundThread();
    super.onPause();
  }

  /** Starts a background thread and its {@link Handler}. */
  private void startBackgroundThread() {
    backgroundThread = new HandlerThread("CameraBackground");
    backgroundThread.start();
  }

  /** Stops the background thread and its {@link Handler}. */
  private void stopBackgroundThread() {
    backgroundThread.quitSafely();
    try {
      backgroundThread.join();
      backgroundThread = null;
    } catch (final InterruptedException e) {
      LOGGER.e(e, "Exception!");
    }
  }

//==================================================================================================

  private byte[] data;
  private Surface surface;
  private UsbReceiver usbReceiver;
  private UsbDeviceConnection udc;
  private List<UsbCamera.SupportInfo> supportInfos = new ArrayList<>();
  private final String ACTION_USB = "com.hsj.camera.sample.USB_PERMISSION." + hashCode();




  private void openCamera(UsbManager usbManager, UsbDevice device) {
    if (camera == null) camera = new UsbCamera();
    UsbDeviceConnection connection = usbManager.openDevice(device);
    if (connection != null) {
      this.udc = connection;
      int status = camera.open(connection.getFileDescriptor());
      if (UsbCamera.STATUS_OK == status) {
        supportInfos.clear();
        status = camera.getSupportInfo(supportInfos);
        if (UsbCamera.STATUS_OK == status && !supportInfos.isEmpty()) {
          UsbCamera.SupportInfo info = supportInfos.get(0);


          info.setFormatCallback(UsbCamera.PIXEL_FORMAT.PIXEL_FORMAT_NV21);
          width = info.width;
          height = info.height;

          data = new byte[ImageUtils.getYUVByteSize(info.height,info.width)];


         // textureView.setAspectRatio(info.height,info.width);only for autofittexture

          status = camera.setSupportInfo(info);
          if (UsbCamera.STATUS_OK == status) {
            camera.setFrameCallback(frameCallback);

            camera.setPreview(surface);








            status = camera.start();

            if (UsbCamera.STATUS_OK != status) {
              showToast("start failed: " + status);
            }
          } else {
            showToast("setSupportInfo failed: " + status);
          }
        } else {
          showToast("getSupportInfo failed: " + status);
        }
      } else {
        showToast("openCamera failed: " + status);
      }
    } else {
      showToast("openCamera failed.");
    }
  }


  private void startCamera() {

    if (usbReceiver == null) {
      IntentFilter filter = new IntentFilter(ACTION_USB);
     context.registerReceiver(usbReceiver = new UsbReceiver(), filter);
    }
    final UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
    Collection<UsbDevice> devices = usbManager.getDeviceList().values();
    UsbDevice usbDevice = null;

    for (UsbDevice device : devices){
      if (vid == device.getVendorId() && pid == device.getProductId()){
        usbDevice = device;
        break;
      }
    }
    if (usbDevice == null){
      showToast("Please connect uvc camera or config vid and pid.");
    } else if (usbManager.hasPermission(usbDevice)) {
      openCamera(usbManager, usbDevice);
    } else {
      Intent intent = new Intent(ACTION_USB);
      PendingIntent pi = PendingIntent.getBroadcast(context, 0, intent, 0);
      usbManager.requestPermission(usbDevice, pi);
    }
  }

  protected void stopCamera() {
    if (this.camera != null) {
      this.camera.stop();
      this.camera.close();
      this.camera.destroy();
      this.camera = null;
    }
    if (this.udc != null) {
      this.udc.close();
      this.udc = null;
    }
    this.supportInfos.clear();
  }

  private final IFrameCallback frameCallback = new IFrameCallback() {
    @Override
    public void onFrame(ByteBuffer frame) {
      frame.clear();
      frame.get(data);
      imageListener.onPreviewFrame(data, null);
    }
  };

  private final class UsbReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
      if (ACTION_USB.equals(intent.getAction())) {

        UsbDevice device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if (device != null && intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
          openCamera((UsbManager) context.getSystemService(Context.USB_SERVICE), device);
          camera.start();


        } else {
          showToast("Usb Permission had been denied.");
        }
      }
    }
  }

  private void showToast(String msg) {
    Toast.makeText(context, msg, Toast.LENGTH_SHORT).show();
  }

//==================================================================================================

}