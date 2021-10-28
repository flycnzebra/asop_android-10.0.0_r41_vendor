package com.android.server.zebra;

import static android.view.Display.DEFAULT_DISPLAY;
import static android.view.Display.INVALID_DISPLAY;

import android.content.Context;
import android.hardware.input.InputManager;
import android.hardware.zebra.V1_0.IZebra;
import android.hardware.zebra.V1_0.IZebraCallback;
import android.os.Bundle;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.zebra.FlyLog;
import android.zebra.IZebraService;
import android.zebra.ZebraListener;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * @hide ClassName: ZebraService
 * Description:
 * Author: FlyZebra
 * Email:flycnzebra@gmail.com
 * Date: 20-1-8 下午5:42
 */
public class ZebraService extends IZebraService.Stub {
    private Context mContext;
    private static RemoteCallbackList<ZebraListener> mZebraListeners = new RemoteCallbackList<>();
    private final Object mLock = new Object();

    private Bundle sensorBundle;
    private Bundle gpsBundle;
    private Bundle cellBundle;
    private Bundle wifiBundle;
    private IZebra hidlZebra;

    public ZebraService(Context context) {
        mContext = context;
        try {
            hidlZebra = IZebra.getService();
            hidlZebra.registerCallback(new ZebraHidlCallBack());
        } catch (Exception e) {
            FlyLog.e(e.toString());
        }
    }

    @Override
    public void registerListener(ZebraListener ZebraListener) throws RemoteException {
        mZebraListeners.register(ZebraListener);
    }

    @Override
    public void unRegisterListener(ZebraListener ZebraListener) throws RemoteException {
        mZebraListeners.unregister(ZebraListener);
    }

    @Override
    public void upSensorData(Bundle bundle) throws RemoteException {
        sensorBundle = bundle;
        notifySensorChange(sensorBundle);
    }

    @Override
    public Bundle getSensorData() throws RemoteException {
        return sensorBundle;
    }

    @Override
    public void upGpsData(Bundle bundle) throws RemoteException {
        gpsBundle = bundle;
        notifyGpsChange(gpsBundle);
    }

    @Override
    public Bundle getGpsData() throws RemoteException {
        return gpsBundle;
    }

    @Override
    public void upCellData(Bundle bundle) throws RemoteException {
        cellBundle = bundle;
        notifyCellChange(cellBundle);
    }

    @Override
    public Bundle getCellData() throws RemoteException {
        return cellBundle;
    }

    @Override
    public void upWifiData(Bundle bundle) throws RemoteException {
        wifiBundle = bundle;
        notifyWifiChange(wifiBundle);
    }

    @Override
    public Bundle getWifiData() throws RemoteException {
        return wifiBundle;
    }

    @Override
    public List<String> getWhiteList() throws RemoteException {
        return null;
    }

    @Override
    public boolean addWhiteProcess(String packName) throws RemoteException {
        return false;
    }

    @Override
    public boolean delWhiteProcess(String packName) throws RemoteException {
        return false;
    }

    private void notifySensorChange(final Bundle bundle) {
        synchronized (mLock) {
            final int N = mZebraListeners.beginBroadcast();
            for (int i = 0; i < N; i++) {
                try {
                    mZebraListeners.getBroadcastItem(i).notifySensorChange(bundle);
                } catch (RemoteException e) {
                    FlyLog.e(e.toString());
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            }
            mZebraListeners.finishBroadcast();
        }
    }

    private void notifyGpsChange(final Bundle bundle) {
        synchronized (mLock) {
            final int N = mZebraListeners.beginBroadcast();
            for (int i = 0; i < N; i++) {
                try {
                    mZebraListeners.getBroadcastItem(i).notifyGpsChange(bundle);
                } catch (RemoteException e) {
                    FlyLog.e(e.toString());
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            }
            mZebraListeners.finishBroadcast();
        }
    }

    private void notifyCellChange(final Bundle bundle) {
        synchronized (mLock) {
            final int N = mZebraListeners.beginBroadcast();
            for (int i = 0; i < N; i++) {
                try {
                    mZebraListeners.getBroadcastItem(i).notifyCellChange(bundle);
                } catch (RemoteException e) {
                    FlyLog.e(e.toString());
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            }
            mZebraListeners.finishBroadcast();
        }
    }

    private void notifyWifiChange(final Bundle bundle) {
        synchronized (mLock) {
            final int N = mZebraListeners.beginBroadcast();
            for (int i = 0; i < N; i++) {
                try {
                    mZebraListeners.getBroadcastItem(i).notifyWifiChange(bundle);
                } catch (RemoteException e) {
                    FlyLog.e(e.toString());
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            }
            mZebraListeners.finishBroadcast();
        }
    }

    class ZebraHidlCallBack extends IZebraCallback.Stub implements Runnable {
        private ByteBuffer eventBuf = ByteBuffer.allocate(1024);
        private final Object mEventLock = new Object();
        byte head[] = new byte[16];
        byte tEvent[] = new byte[10];
        byte kEvent[] = new byte[2];
        final float DEFAULT_SIZE = 1.0f;
        final int DEFAULT_META_STATE = 0;
        final float DEFAULT_PRECISION_X = 1.0f;
        final float DEFAULT_PRECISION_Y = 1.0f;
        final int DEFAULT_EDGE_FLAGS = 0;

        ZebraHidlCallBack() {
            new Thread(this).start();
        }

        @Override
        public void run() {
            while (true) {
                try {
                    synchronized (mEventLock) {
                        if (eventBuf.position() == 0) {
                            mEventLock.wait();
                        } else {
                            eventBuf.flip();
                            eventBuf.get(head);
                            if (head[2] == (byte) 0x03 && head[3] == (byte) 0x01) {
                                eventBuf.get(tEvent);
                                int x = (tEvent[2] & 0xFF) << 8 | (tEvent[3] & 0xFF);
                                int y = (tEvent[4] & 0xFF) << 8 | (tEvent[5] & 0xFF);
                                int w = (tEvent[6] & 0xFF) << 8 | (tEvent[7] & 0xFF);
                                int h = (tEvent[8] & 0xFF) << 8 | (tEvent[9] & 0xFF);
                                float fx = x * 1080 / w;
                                float fy = y * 2160 / h;
                                int action = MotionEvent.ACTION_UP;
                                float pressure = 0.0f;
                                switch ((tEvent[1] & 0xFF)) {
                                    case 0x00:
                                        action = MotionEvent.ACTION_DOWN;
                                        pressure = 1.0f;
                                        break;
                                    case 0x01:
                                        action = MotionEvent.ACTION_MOVE;
                                        pressure = 1.0f;
                                        break;
                                    case 0x02:
                                        action = MotionEvent.ACTION_UP;
                                        pressure = 0.0f;
                                        break;
                                }
                                sendTouchEvent(action, fx, fy, pressure);
                                FlyLog.e("sendTouchEvent action=%d, fx=%f, fy=%f, pressure=%f", action, fx, fy, pressure);
                            } else if (head[2] == (byte) 0x03 && head[3] == (byte) 0x02) {
                                eventBuf.get(kEvent);
                                int keycode = kEvent[1] & 0x00FF;
                                if (keycode == 158) keycode = 4;
                                FlyLog.e("sendKeyEvent keycode=%d", keycode);
                                sendKeyEvent(keycode);
                            }
                            eventBuf.compact();
                        }
                    }
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            }
        }

        @Override
        public void notifyEvent(ArrayList<Byte> event) throws RemoteException {
            synchronized (mEventLock) {
                if (eventBuf.position() > 1024) {
                    eventBuf.clear();
                }
                byte data[] = new byte[event.size()];
                for (int i = 0; i < event.size(); i++) {
                    data[i] = event.get(i);
                }
                eventBuf.put(data, 0, event.size());
                mEventLock.notify();
            }
        }

        private void sendKeyEvent(int keyCode) {
            final long now = SystemClock.uptimeMillis();
            KeyEvent key1 = new KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 0);
            InputManager.getInstance().injectInputEvent(key1, InputManager.INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
            KeyEvent key2 = new KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0);
            InputManager.getInstance().injectInputEvent(key2, InputManager.INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
        }

        private void sendTouchEvent(int action, float x, float y, float pressure) {
            final long now = SystemClock.uptimeMillis();
            MotionEvent event = MotionEvent.obtain(now, now, action, x, y, pressure, DEFAULT_SIZE,
                    DEFAULT_META_STATE, DEFAULT_PRECISION_X, DEFAULT_PRECISION_Y,
                    InputDevice.SOURCE_TOUCHSCREEN, DEFAULT_EDGE_FLAGS);
            event.setSource(InputDevice.SOURCE_TOUCHSCREEN);
            InputManager.getInstance().injectInputEvent(event, InputManager.INJECT_INPUT_EVENT_MODE_WAIT_FOR_FINISH);
        }
    }
}
