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

    class ZebraHidlCallBack extends IZebraCallback.Stub {

        private void sendKeyEvent(int keyCode) {
            final long now = SystemClock.uptimeMillis();
            KeyEvent key1 =new KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 0);
            InputManager.getInstance().injectInputEvent(key1, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
            KeyEvent key2 = new KeyEvent(now, now,KeyEvent.ACTION_UP, keyCode, 0);
            InputManager.getInstance().injectInputEvent(key2, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
        }

        private void sendTouchEvent(int action, float x, float y, float presure) {
            final long now = SystemClock.uptimeMillis();
            MotionEvent touch = MotionEvent.obtain(now, now, action, x, y, 1.0f, 1.0f, 0, 1.0f, presure, InputDevice.SOURCE_TOUCHSCREEN, 0);
            InputManager.getInstance().injectInputEvent(touch, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
        }

        @Override
        public void notifyEvent(ArrayList<Byte> event) throws RemoteException {
            try {
                if (event.get(2) == (byte) 0x03 && event.get(3) == (byte) 0x01) {
                    int x = (event.get(18)&0xFF)<<8|(event.get(19)&0xFF);
                    int y = (event.get(20)&0xFF)<<8|(event.get(21)&0xFF);;
                    int w = (event.get(22)&0xFF)<<8|(event.get(23)&0xFF);;
                    int h = (event.get(24)&0xFF)<<8|(event.get(25)&0xFF);;
                    x = x * 1080 / w;
                    y = y * 2160 / h;
                    int action = (event.get(17)&0xFF)==0x02 ? KeyEvent.ACTION_UP : KeyEvent.ACTION_DOWN;
                    sendTouchEvent(action,x, y, 1.0f);
                } else if (event.get(2) == (byte) 0x03 && event.get(3) == (byte) 0x02) {
                    sendKeyEvent((int) event.get(17));
                }
            }catch (Exception e){
                FlyLog.e(e.toString());
            }
        }
    }
}
