package com.frizzle.frizzleplayermaster.player;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

/**
 * author: LWJ
 * date: 2020/7/16$
 * description
 */
public class FrizzlePlayer  implements SurfaceHolder.Callback {
    static {
        System.loadLibrary("frizzleplayer");
    }

    //准备完成的监听
    private OnPrepareListener onPrepareListener;
    //错误监听
    private OnErrorListener onErrorListener;
    //进度监听
    private OnProgressListener onProgressListener;

    private String dataSource;
    private native void native_prepare(String dataSource);
    private native void native_start();
    private native void native_set_surface(Surface surface);
    private SurfaceHolder surfaceHolder;
    public void setSurfaceView(SurfaceView surfaceView) {
        if (null != this.surfaceHolder) {
            this.surfaceHolder.removeCallback(this);
        }
        this.surfaceHolder = surfaceView.getHolder();
        this.surfaceHolder.addCallback(this);
    }

    public void setDataSource(String absolutePath) {
        this.dataSource=absolutePath;

    }

    public void prepare() {
        native_prepare(dataSource);
    }

    public void setOnPrepareListener(OnPrepareListener onPrepareListener) {
        this.onPrepareListener = onPrepareListener;
    }

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    public void setOnProgressListener(OnProgressListener onProgressListener) {
        this.onProgressListener = onProgressListener;
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        native_set_surface(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {

    }

    //不断调用
    public void onProgress(int progress) {
        if (null != onProgressListener) {
            onProgressListener.onProgress(progress);
        }

    }
    //准备完毕
    public void onPrepare() {
        if (null != onPrepareListener) {
            onPrepareListener.onPrepared();
        }

    }
    //错误
    public void onError(int errorCode) {
        Log.e("errorCode: ",""+errorCode);
        if (null != onErrorListener) {
            onErrorListener.onError(errorCode);
        }

    }

    public void start() {
        native_start();
    }

    public void stop() {
        native_stop();
    }

    /**
     * 界面销毁调用
     */
    public void release() {
        if (null != surfaceHolder){
            surfaceHolder.removeCallback(this);
        }
        native_release();
    }

    public int getDuration(){
        return native_getDuration();
    }

    public native int native_getDuration();

    public void seek(final int progress) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                native_seek(progress);
            }
        }).start();
    }

    public native void native_seek(int progress);

    public interface OnPrepareListener {
        void onPrepared();
    }

    public interface OnErrorListener {
        void onError(int error);
    }

    public interface OnProgressListener {
        void onProgress(int progress);
    }

    public native void native_stop();
    public native void native_release();
}
