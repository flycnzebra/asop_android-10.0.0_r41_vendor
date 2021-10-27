package com.android.server.zebra;

import android.app.ActivityManager;
import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.RemoteException;
import android.zebra.IZebraProcessService;

import java.util.ArrayList;
import java.util.List;

/**
 * @hide ClassName: ZebraProcessService
 * Description:
 * Author: FlyZebra
 * Email:flycnzebra@gmail.com
 * Date: 21-3-24 下午5:42
 */
public class ZebraProcessService extends IZebraProcessService.Stub implements Runnable {
    private Context mContext;
    private ActivityManager am;
    private static final Object mLock = new Object();
    private static List<String> whiteList = new ArrayList<>();
    public static ZebraProcessService ocProcessService;
    private static final HandlerThread mTaskThread = new HandlerThread("ZebraProcess");

    static {
        mTaskThread.start();
    }

    private static final Handler mTaskHandler = new Handler(mTaskThread.getLooper());
    private static int FIRST_WORK_TIME = 5000;
    private static int ONCE_WORK_TIME = 60000;

    public static ZebraProcessService getInstance() {
        return ocProcessService;
    }

    public ZebraProcessService(Context context) {
        mContext = context;
        ocProcessService = this;
        am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        mTaskHandler.postDelayed(this, FIRST_WORK_TIME);
    }

    @Override
    public void run() {
        synchronized (mLock) {
            for (String str : whiteList) {
                //am.forceStopPackage(str);
            }
        }
        mTaskHandler.postDelayed(this, ONCE_WORK_TIME);
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

    @Override
    public void notifyProcessStatus(String packName, int statu) throws RemoteException {

    }
}
