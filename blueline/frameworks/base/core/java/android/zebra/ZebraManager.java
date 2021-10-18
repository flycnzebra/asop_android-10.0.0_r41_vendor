package android.zebra;

import android.content.Context;
import android.os.Bundle;
import android.os.RemoteException;

import java.util.ArrayList;
import java.util.List;

/**
 * ClassName: ZebraManager
 * Description:
 * Author: FlyZebra
 * Email:flycnzebra@gmail.com
 * Date: 20-1-8 下午5:44
 */

public class ZebraManager {

    public static final String SENSOR_TYPE = "TYPE";
    public static final String SENSOR_X = "X";
    public static final String SENSOR_Y = "Y";
    public static final String SENSOR_Z = "Z";
    public static final String SENSOR_ARRAY = "ARRAY";
    public static final String SENSOR_TIME = "TIME";
    public static final String GPS_LOCATION = "LOCATION";
    public static final String GPS_STARLIST = "STARLIST";
    public static final String CELL_LIST = "CELLLIST";
    public static final String WIFI_LIST = "WIFILIST";
    public static final String WIFI_INFO = "WIFIINFO";
    public static final String PB_ACTION = "ACTION";
    public static final String PB_NAME = "NAME";
    public static final String PB_NUMBER = "NUMBER";
    public static final String PB_SIMCARD = "IN_SIM";
    public static final String SMS_NUMBER = "SMS_NUMBER";
    public static final String SMS_TEXT = "SMS_TEXT";

    public static final int PB_ACTION_SAVE = 0;
    public static final int PB_ACTION_DELETE_ONE = 1;
    public static final int PB_ACTION_DELETE_ALL = 2;

    public static final int LISTEN_TYPE_SENSOR = 0x00000001;
    public static final int LISTEN_TYPE_GPS = 0x00000002;
    public static final int LISTEN_TYPE_CELL = 0x00000004;
    public static final int LISTEN_TYPE_WIFI = 0x00000008;
    public static final int LISTEN_TYPE_PHONEBOOK = 0x00000010;
    public static final int LISTEN_TYPE_WEBCAM = 0x00000020;
    public static final int LISTEN_TYPE_SMS = 0x00000040;

    private int mListenType;
    private IZebraService mService;
    private ZebraListener mZebraListener = new ZebraListener.Stub() {
        @Override
        public void notifySensorChange(Bundle bundle) throws RemoteException {
            synchronized (mSensorLock) {
                for (SensorListener listener : mSensorListeners) {
                    listener.notifySensorChange(bundle);
                }
            }
        }

        @Override
        public void notifyGpsChange(Bundle bundle) throws RemoteException {
            synchronized (mGpsLock) {
                for (GpsListener listener : mGpsListeners) {
                    listener.notifyGpsChange(bundle);
                }
            }
        }

        @Override
        public void notifyCellChange(Bundle bundle) throws RemoteException {
            synchronized (mCellLock) {
                for (CellListener listener : mCellListeners) {
                    listener.notifyCellChange(bundle);
                }
            }
        }

        @Override
        public void notifyWifiChange(Bundle bundle) throws RemoteException {
            synchronized (mWifiLock) {
                for (WifiListener listener : mWifiListeners) {
                    listener.notifyWifiChange(bundle);
                }
            }
        }

        @Override
        public void notifyPhonebookChange(Bundle bundle) throws RemoteException {
            synchronized (mPhonebookLock) {
                for (PhonebookListener listener : mPhonebookListeners) {
                    listener.notifyPhonebookChange(bundle);
                }
            }
        }

        @Override
        public void notifyWebcamChange(Bundle bundle) throws RemoteException {
            synchronized (mWebcamLock) {
                for (WebcamListener listener : mWebcamListeners) {
                    listener.notifyWebcamChange(bundle);
                }
            }
        }

        @Override
        public void notifySmsChange(Bundle bundle) throws RemoteException {
            synchronized (mSmsLock) {
                for (SmsListener listener : mSmsListeners) {
                    listener.notifySmsChange(bundle);
                }
            }
        }
    };

    public ZebraManager(Context context, IZebraService zebraService) {
        mService = zebraService;
    }

    public void upSensorData(Bundle bundle) {
        try {
            if (mService != null) {
                mService.upSensorData(bundle);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public Bundle getSensorData() {
        try {
            if (mService != null) {
                return mService.getSensorData();
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public void upGpsData(Bundle bundle) {
        try {
            if (mService != null) {
                mService.upGpsData(bundle);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public Bundle getGpsData() {
        try {
            if (mService != null) {
                return mService.getGpsData();
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public void upCellData(Bundle bundle) {
        try {
            if (mService != null) {
                mService.upCellData(bundle);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public Bundle getCellData() {
        try {
            if (mService != null) {
                return mService.getCellData();
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public void upWifiData(Bundle bundle) {
        try {
            if (mService != null) {
                mService.upWifiData(bundle);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public Bundle getWifiData() {
        try {
            if (mService != null) {
                return mService.getWifiData();
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public void upPhonebookData(Bundle bundle) {
        try {
            if (mService != null) {
                mService.upPhonebookData(bundle);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public Bundle getPhonebookData() {
        try {
            if (mService != null) {
                return mService.getPhonebookData();
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public void upWebcamData(Bundle bundle) {
        try {
            if (mService != null) {
                mService.upWebcamData(bundle);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public Bundle getWebcamData() {
        try {
            if (mService != null) {
                return mService.getWebcamData();
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public void upSmsData(Bundle bundle) {
        try {
            if (mService != null) {
                mService.upSmsData(bundle);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public Bundle getSmsData() {
        try {
            if (mService != null) {
                return mService.getSmsData();
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    private List<SensorListener> mSensorListeners = new ArrayList<>();
    private final Object mSensorLock = new Object();

    public interface SensorListener {
        void notifySensorChange(Bundle bundle);
    }

    public void addSensorListener(SensorListener sensorListener) {
        registerLisenter(LISTEN_TYPE_SENSOR);
        synchronized (mSensorLock) {
            mSensorListeners.add(sensorListener);
        }
    }

    public void removeSensorListener(SensorListener sensorListener) {
        synchronized (mSensorLock) {
            mSensorListeners.remove(sensorListener);
        }
        unregisterListener(LISTEN_TYPE_SENSOR);
    }

    private List<GpsListener> mGpsListeners = new ArrayList<>();
    private final Object mGpsLock = new Object();

    public interface GpsListener {
        void notifyGpsChange(Bundle bundle);
    }

    public void addGpsListener(GpsListener gpsListener) {
        registerLisenter(LISTEN_TYPE_GPS);
        synchronized (mGpsLock) {
            mGpsListeners.add(gpsListener);
        }
    }

    public void removeGpsListener(GpsListener gpsListener) {
        synchronized (mGpsLock) {
            mGpsListeners.remove(gpsListener);
        }
        unregisterListener(LISTEN_TYPE_GPS);
    }

    private List<CellListener> mCellListeners = new ArrayList<>();
    private final Object mCellLock = new Object();

    public interface CellListener {
        void notifyCellChange(Bundle bundle);
    }

    public void addCellListener(CellListener cellListener) {
        registerLisenter(LISTEN_TYPE_CELL);
        synchronized (mCellLock) {
            mCellListeners.add(cellListener);
        }
    }

    public void removeCellListener(CellListener cellListener) {
        synchronized (mCellLock) {
            mCellListeners.remove(cellListener);
        }
        unregisterListener(LISTEN_TYPE_CELL);
    }

    private List<WifiListener> mWifiListeners = new ArrayList<>();
    private final Object mWifiLock = new Object();

    public interface WifiListener {
        void notifyWifiChange(Bundle bundle);
    }

    public void addWifiListener(WifiListener wifiListener) {
        registerLisenter(LISTEN_TYPE_WIFI);
        synchronized (mWifiLock) {
            mWifiListeners.add(wifiListener);
        }
    }

    public void removeWifiListener(WifiListener wifiListener) {
        synchronized (mWifiLock) {
            mWifiListeners.remove(wifiListener);
        }
        unregisterListener(LISTEN_TYPE_WIFI);
    }

    private List<PhonebookListener> mPhonebookListeners = new ArrayList<>();
    private final Object mPhonebookLock = new Object();

    public interface PhonebookListener {
        void notifyPhonebookChange(Bundle bundle);
    }

    public void addPhonebookListener(PhonebookListener phonebookListener) {
        registerLisenter(LISTEN_TYPE_PHONEBOOK);
        synchronized (mPhonebookLock) {
            mPhonebookListeners.add(phonebookListener);
        }
    }

    public void removePhonebookListener(PhonebookListener phonebookListener) {
        synchronized (mPhonebookLock) {
            mPhonebookListeners.remove(phonebookListener);
        }
        unregisterListener(LISTEN_TYPE_PHONEBOOK);
    }

    private List<WebcamListener> mWebcamListeners = new ArrayList<>();
    private final Object mWebcamLock = new Object();

    public interface WebcamListener {
        void notifyWebcamChange(Bundle bundle);
    }

    public void addWebcamListener(WebcamListener webcamListener) {
        registerLisenter(LISTEN_TYPE_WEBCAM);
        synchronized (mWebcamLock) {
            mWebcamListeners.add(webcamListener);
        }
    }

    public void removeWebcamListener(WebcamListener webcamListener) {
        synchronized (mWebcamLock) {
            mWebcamListeners.remove(webcamListener);
        }
        unregisterListener(LISTEN_TYPE_WEBCAM);
    }

    private List<SmsListener> mSmsListeners = new ArrayList<>();
    private final Object mSmsLock = new Object();

    public interface SmsListener {
        void notifySmsChange(Bundle bundle);
    }

    public void addSmsListener(SmsListener smsListener) {
        registerLisenter(LISTEN_TYPE_SMS);
        synchronized (mSmsLock) {
            mSmsListeners.add(smsListener);
        }
    }

    public void removeSmsListener(SmsListener smsListener) {
        synchronized (mSmsLock) {
            mSmsListeners.remove(smsListener);
        }
        unregisterListener(LISTEN_TYPE_SMS);
    }

    private void registerLisenter(int type) {
        if (mService != null && (mListenType & type) == 0) {
            try {
                mListenType |= type;
                mService.registerListener(mZebraListener, mListenType);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
    }

    private void unregisterListener(int type) {
        if (mService != null && (mListenType & type) != 0) {
            try {
                mListenType &= ~type;
                if (mListenType == 0)
                    mService.unregisterListener(mZebraListener);
                else
                    mService.registerListener(mZebraListener, mListenType);
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
    }

    public void setAirplaneModeOn(boolean enabling) {
        try {
            if (mService != null) {
                mService.setAirplaneModeOn(enabling);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public boolean usbDhcpWifi(boolean opening) {
        try {
            if (mService != null) {
                return mService.usbDhcpWifi(opening);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return false;
    }

    public void usbPcWifi(boolean opening, String ipAddress, String gateway) {
        try {
            if (mService != null) {
                mService.usbPcWifi(opening, ipAddress, gateway);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
}

