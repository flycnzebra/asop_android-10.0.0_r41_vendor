package com.android.server.zebra;

import android.content.Context;
import android.content.Intent;
import android.zebra.FlyLog;
import android.zebra.IZebraService;
import android.zebra.ZebraListener;
import android.zebra.ZebraManager;
import android.os.Build;
import android.os.Bundle;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.Binder;
import android.os.UserHandle;
import android.os.IBinder;
import android.provider.Settings;
import android.util.ArrayMap;

/**
 * @hide ClassName: ZebraService
 * Description:
 * Author: FlyZebra
 * Email:flycnzebra@gmail.com
 * Date: 20-1-8 下午5:42
 */
public class ZebraService extends IZebraService.Stub {
    private static final String TAG = "ZebraService";
    private Context mContext;
    private ArrayMap<IBinder, Integer> mRecords = new ArrayMap<IBinder, Integer>();
    private RemoteCallbackList<ZebraListener> mZebraListeners =
        new RemoteCallbackList<ZebraListener>() {
            @Override
            public void onCallbackDied(ZebraListener callback) {
                synchronized(mRecords) {
                    mRecords.remove(callback.asBinder());
                }
            }
        };
    private Bundle sensorBundle;
    private Bundle gpsBundle;
    private Bundle cellBundle;
    private Bundle wifiBundle;
    private Bundle phonebookBundle;
    private Bundle webcamBundle;
    private Bundle smsBundle;

    public ZebraService(Context context) {
        mContext = context;
    }

    @Override
    public void registerListener(ZebraListener zebraListener, int type) throws RemoteException {
        if (type <= 0) throw new RemoteException(TAG + " Error type: " + type);

        synchronized(mRecords) {
            IBinder registerIBinder = zebraListener.asBinder();
            if (mRecords.put(registerIBinder, type) == null) {
                mZebraListeners.register(zebraListener);
            }
        }
    }

    @Override
    public void unregisterListener(ZebraListener zebraListener) throws RemoteException {
        synchronized(mRecords) {
            mZebraListeners.unregister(zebraListener);
            mRecords.remove(zebraListener.asBinder());
        }
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
    public void upPhonebookData(Bundle bundle) throws RemoteException {
        phonebookBundle = bundle;
        notifyPhonebookChange(phonebookBundle);
    }

    @Override
    public Bundle getPhonebookData() throws RemoteException {
        return phonebookBundle;
    }

    @Override
    public void upWebcamData(Bundle bundle) throws RemoteException {
        webcamBundle = bundle;
        notifyWebcamChange(webcamBundle);
    }

    @Override
    public Bundle getWebcamData() throws RemoteException {
        return webcamBundle;
    }

    @Override
    public void upSmsData(Bundle bundle) throws RemoteException {
        smsBundle = bundle;
        notifySmsChange(smsBundle);
    }

    @Override
    public Bundle getSmsData() throws RemoteException {
        return smsBundle;
    }

    private void notifySensorChange(final Bundle bundle) {
        synchronized (mRecords) {
            mZebraListeners.broadcast(listener -> {
                try {
                    Integer recordType = mRecords.get(listener.asBinder());
                    if (recordType != null &&
                            ((recordType.intValue() & ZebraManager.LISTEN_TYPE_SENSOR) > 0)) {
                        listener.notifySensorChange(bundle);
                    }
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            });
        }
    }

    private void notifyGpsChange(final Bundle bundle) {
        synchronized (mRecords) {
            mZebraListeners.broadcast(listener -> {
                try {
                    Integer recordType = mRecords.get(listener.asBinder());
                    if (recordType != null &&
                            ((recordType.intValue() & ZebraManager.LISTEN_TYPE_GPS) > 0)) {
                        listener.notifyGpsChange(bundle);
                    }
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            });
        }
    }

    private void notifyCellChange(final Bundle bundle) {
        synchronized (mRecords) {
            mZebraListeners.broadcast(listener -> {
                try {
                    Integer recordType = mRecords.get(listener.asBinder());
                    if (recordType != null &&
                            ((recordType.intValue() & ZebraManager.LISTEN_TYPE_CELL) > 0)) {
                        listener.notifyCellChange(bundle);
                    }
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            });
        }
    }

    private void notifyWifiChange(final Bundle bundle) {
        synchronized (mRecords) {
            mZebraListeners.broadcast(listener -> {
                try {
                    Integer recordType = mRecords.get(listener.asBinder());
                    if (recordType != null &&
                            ((recordType.intValue() & ZebraManager.LISTEN_TYPE_WIFI) > 0)) {
                        listener.notifyWifiChange(bundle);
                    }
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            });
        }
    }

    private void notifyPhonebookChange(final Bundle bundle) {
        synchronized (mRecords) {
            mZebraListeners.broadcast(listener -> {
                try {
                    Integer recordType = mRecords.get(listener.asBinder());
                    if (recordType != null &&
                            ((recordType.intValue() & ZebraManager.LISTEN_TYPE_PHONEBOOK) > 0)) {
                        listener.notifyPhonebookChange(bundle);
                    }
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            });
        }
    }

    private void notifyWebcamChange(final Bundle bundle) {
        synchronized (mRecords) {
            mZebraListeners.broadcast(listener -> {
                try {
                    Integer recordType = mRecords.get(listener.asBinder());
                    if (recordType != null &&
                            ((recordType.intValue() & ZebraManager.LISTEN_TYPE_WEBCAM) > 0)) {
                        listener.notifyWebcamChange(bundle);
                    }
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            });
        }
    }

    private void notifySmsChange(final Bundle bundle) {
        synchronized (mRecords) {
            mZebraListeners.broadcast(listener -> {
                try {
                    Integer recordType = mRecords.get(listener.asBinder());
                    if (recordType != null &&
                            ((recordType.intValue() & ZebraManager.LISTEN_TYPE_SMS) > 0)) {
                        listener.notifySmsChange(bundle);
                    }
                } catch (Exception e) {
                    FlyLog.e(e.toString());
                }
            });
        }
    }

    @Override
    public void setAirplaneModeOn(boolean enabling) {
    }

    @Override
    public boolean usbDhcpWifi(boolean opening) {
        return false;
    }

    @Override
    public void usbPcWifi(boolean opening, String ipAddress, String gateway) {
    }
}
