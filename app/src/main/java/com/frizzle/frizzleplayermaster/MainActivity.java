package com.frizzle.frizzleplayermaster;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import com.frizzle.frizzleplayermaster.player.FrizzlePlayer;

import java.io.File;

public class MainActivity extends AppCompatActivity implements SeekBar.OnSeekBarChangeListener {

    // Used to load the 'native-lib' library on application startup.

    private SurfaceView surfaceView;
    private SeekBar seekBar;
    private FrizzlePlayer frizzlePlayer;
    private boolean isTouchSeekBar;
    private boolean isSeeking;
    private int progress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager
                .LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_main);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        seekBar = findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(this);
        Button btnPlay = findViewById(R.id.btn_play);
        Button btnStop = findViewById(R.id.btn_stop);
        btnPlay.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                play();
            }
        });
        checkPermission();
        frizzlePlayer = new FrizzlePlayer();
        frizzlePlayer.setSurfaceView(surfaceView);
        File file = new File(Environment.getExternalStorageDirectory(), "input.mp4");
        frizzlePlayer.setDataSource(file.getAbsolutePath());
        frizzlePlayer.setOnPrepareListener(new FrizzlePlayer.OnPrepareListener() {
            @Override
            public void onPrepared() {
                frizzlePlayer.start();
                int duration = frizzlePlayer.getDuration();
                if (duration<=0){
                    return;
                }
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        seekBar.setVisibility(View.VISIBLE);
                    }
                });
            }
        });
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                isTouchSeekBar=true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                isTouchSeekBar=false;
                isSeeking = true;
                progress = frizzlePlayer.getDuration() * seekBar.getProgress()/100;
                frizzlePlayer.seek(progress);
            }
        });

        frizzlePlayer.setOnProgressListener(new FrizzlePlayer.OnProgressListener() {
            @Override
            public void onProgress(final int progress2) {
                if (!isTouchSeekBar){
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            int duration = frizzlePlayer.getDuration();
                            if (duration!=0){
                                if (isSeeking){
                                    isSeeking=false;
                                    return;
                                }
                                seekBar.setProgress(progress2*100/duration);
                            }
                        }
                    });
                }
            }
        });
        btnStop.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                stop();
            }
        });
    }

    public void checkPermission() {
        boolean isGranted = true;
        if (android.os.Build.VERSION.SDK_INT >= 23) {
            if (this.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                //如果没有写sd卡权限
                isGranted = false;
            }
            if (this.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                isGranted = false;
            }
            Log.i("cbs","isGranted == "+isGranted);
            if (!isGranted) {
                this.requestPermissions(
                        new String[]{Manifest.permission.ACCESS_COARSE_LOCATION, Manifest.permission
                                .ACCESS_FINE_LOCATION,
                                Manifest.permission.READ_EXTERNAL_STORAGE,
                                Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        102);
            }
        }

    }



    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {

    }

    private void play() {
        frizzlePlayer.prepare();
    }

    @Override
    public void onPointerCaptureChanged(boolean hasCapture) {

    }


    /**
     * 停止播放
     */
    private void stop() {
        frizzlePlayer.stop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (frizzlePlayer!=null){
            frizzlePlayer.release();
        }
    }
}
