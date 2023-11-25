package com.example.embedded_smarthome;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.app.TimePickerDialog;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.GravityEnum;
import com.afollestad.materialdialogs.MaterialDialog;
import com.example.myloadingbutton.MyLoadingButton;
import com.github.angads25.toggle.interfaces.OnToggledListener;
import com.github.angads25.toggle.model.ToggleableView;
import com.github.angads25.toggle.widget.LabeledSwitch;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

import java.nio.charset.Charset;
import java.util.Locale;

import pl.pawelkleczkowski.customgauge.CustomGauge;

import android.widget.TimePicker;

public class MainActivity extends AppCompatActivity {
    MQTTHelper mqttHelper;
    TextView txtTemp, txtHumi;
    LabeledSwitch btnTimer;
    CustomGauge temp;
    CustomGauge humi;
    MyLoadingButton myLoadingButton1, myLoadingButton2;
    boolean isLED_open = false;
    boolean isDoor_open = false;
    boolean isAlert_5 = false, isAlert_6 = false, isAlert_7 = false;
    MaterialDialog.Builder builder;
    MaterialDialog dialog;
    MaterialDialog.Builder builder_init;
    MaterialDialog dialog_init;
    Button timeButton;
    int hour, minute = 0;
    Handler handler;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        timeButton = findViewById(R.id.timeButton);
        temp = findViewById(R.id.gauge1);
        temp.setValue(30);
        humi = findViewById(R.id.gauge2);
        humi.setValue(70);
        txtTemp = findViewById(R.id.textTemperature);
        txtHumi = findViewById(R.id.textHuminity);
        btnTimer = findViewById(R.id.btnTimer);

        handler = new Handler(Looper.getMainLooper());
        btnTimer.setOnToggledListener(new OnToggledListener() {
            @Override
            public void onSwitched(ToggleableView toggleableView, boolean isOn) {

                if (isOn == true){
                    long theTime = 1000*(hour*3600 + minute*60) + 500;
                    Log.d("TEST", "thetime: " + theTime);
                    handler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            if(isLED_open){
                                process_led();
                                btnTimer.setOn(false);
                            }
                        }
                    }, theTime);
                } else {
                    handler.removeCallbacksAndMessages(null);
                    Log.d("TEST", "remove ");
                }
            }
        });

        builder = new MaterialDialog.Builder(this)
                .title("CẢNH BÁO")
                .titleColor(Color.RED)
                .titleGravity(GravityEnum.CENTER)
                .contentGravity(GravityEnum.CENTER)
                .content("R.string.content")
                .contentColor(Color.BLACK)
                .positiveColor(Color.BLUE)
                .negativeColor(Color.RED)
                .positiveText("XÁC NHẬN");


        dialog = builder.build();

        builder_init = new MaterialDialog.Builder(this)
                .title("THÔNG BÁO")
                .titleColor(Color.BLUE)
                .titleGravity(GravityEnum.CENTER)
                .contentGravity(GravityEnum.CENTER)
                .content("Kết nối server thành công!")
                .contentColor(Color.BLACK)
                .positiveColor(Color.BLUE)
                .positiveText("XÁC NHẬN");


        dialog_init = builder_init.build();


        myLoadingButton1 = findViewById(R.id.my_loading_button1);
        myLoadingButton1.setButtonLabel("MỞ ĐÈN");
        myLoadingButton1.setMyButtonClickListener(new MyLoadingButton.MyLoadingButtonClick() {
            @Override
            public void onMyLoadingButtonClick() {
                process_led();
            }
        });


        myLoadingButton2 = findViewById(R.id.my_loading_button2);
        myLoadingButton2.setButtonLabel("MỞ CỬA");
        myLoadingButton2.setMyButtonClickListener(new MyLoadingButton.MyLoadingButtonClick() {
            @Override
            public void onMyLoadingButtonClick() {
                process_door();
            }
        });

        startMQTT();
    }

    public void popTimePicker(View view)
    {
        TimePickerDialog.OnTimeSetListener onTimeSetListener = new TimePickerDialog.OnTimeSetListener()
        {
            @Override
            public void onTimeSet(TimePicker timePicker, int selectedHour, int selectedMinute)
            {
                hour = selectedHour;
                minute = selectedMinute;
                timeButton.setText(String.format(Locale.getDefault(), "%02d:%02d",hour, minute));
                Log.d("TEST", "Hour: " + hour + " Min: " + minute);
            }
        };

        TimePickerDialog timePickerDialog = new TimePickerDialog(this, /*style,*/ onTimeSetListener, hour, minute, true);

        timePickerDialog.setTitle("Select Time");
        timePickerDialog.show();
    }

    public void process_led(){
        myLoadingButton1.showLoadingButton();
        if (isLED_open) {
            sendDataMQTT("sonnguyen19/feeds/nutnhan1", "0");
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    if (isLED_open){
                        myLoadingButton1.showErrorButton();
                        Log.d("TEST", "isLED true" + isLED_open);
                    }
                }
            }, 3000);
        }
        else{
            sendDataMQTT("sonnguyen19/feeds/nutnhan1", "1");
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    if (!isLED_open){
                        myLoadingButton1.showErrorButton();
                        Log.d("TEST", "isLED false" + isLED_open);
                    }
                }
            }, 3000);
        }
    }

    public void process_door(){
        myLoadingButton2.showLoadingButton();
        if (isDoor_open) {
            sendDataMQTT("sonnguyen19/feeds/nutnhan2", "0");
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    if (isDoor_open){
                        myLoadingButton2.showErrorButton();
                    }
                }
            }, 7500);
        }
        else{
            sendDataMQTT("sonnguyen19/feeds/nutnhan2", "1");
            final Handler handler = new Handler(Looper.getMainLooper());
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    if (!isDoor_open){
                        myLoadingButton2.showErrorButton();
                    }
                }
            }, 7500);
        }
    }
    public void sendDataMQTT(String topic, String value){
        MqttMessage msg = new MqttMessage();
        msg.setId(1234);
        msg.setQos(0);
        msg.setRetained(false);

        byte[] b = value.getBytes(Charset.forName("UTF-8"));
        msg.setPayload(b);

        try {
            mqttHelper.mqttAndroidClient.publish(topic, msg);
        }catch (MqttException e){
        }
    }
    public void startMQTT() {
        mqttHelper = new MQTTHelper(this);
        mqttHelper.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectComplete(boolean reconnect, String serverURI) {
                dialog_init.show();
            }

            @Override
            public void connectionLost(Throwable cause) {

            }

            @Override
            public void messageArrived(String topic, MqttMessage message) throws Exception {
                Log.d("TEST", topic + "send: " + message.toString());
                if(topic.contains("cambien1")){
                    temp.setValue(Integer.parseInt(message.toString()));
                    txtTemp.setText(message.toString() + "°C");
                }else if(topic.contains("cambien2")){
                    humi.setValue(Integer.parseInt(message.toString()));
                    txtHumi.setText((message.toString() + "%"));
                }else if (topic.contains("canhbao")){
                    if(message.toString().equals("5")) {
                        builder.content("Phát hiện nhiệt độ cao!")
                                .negativeText(isDoor_open? "": "MỞ CỬA")
                                .onPositive(new MaterialDialog.SingleButtonCallback() {
                                    @Override
                                    public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
                                        isAlert_5 = false;
                                        Log.d("TEST", "isAlert_popup: " + isAlert_5);
                                    }
                                })
                                .onNegative(new MaterialDialog.SingleButtonCallback() {
                                    @Override
                                    public void onClick(MaterialDialog dialog, DialogAction which) {
                                        // TODO
                                        isDoor_open = false;
                                        process_door();
                                    }
                                });
                        dialog = builder.build();
                        if(!isAlert_5) {
                            isAlert_5 = true;
                            dialog.show();

                        }

                    }
                    else if(message.toString().equals("6")) {
                        builder.content("Phát hiện vật cản!")
                                .negativeText("")
                                .onPositive(new MaterialDialog.SingleButtonCallback() {
                                    @Override
                                    public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
                                        isAlert_6 = false;
                                        Log.d("TEST", "isAlert_popup: " + isAlert_6);
                                    }
                                });
                        dialog = builder.build();
                        if(!isAlert_6) {
                            isAlert_6 = true;
                            dialog.show();
                        }

                    }
                    else if(message.toString().equals("7")) {
                        builder.content("Phát hiện trời mưa!")
                                .onPositive(new MaterialDialog.SingleButtonCallback() {
                                    @Override
                                    public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
                                        isAlert_5 = false;
                                        Log.d("TEST", "isAlert_popup: " + isAlert_7);
                                    }
                                })
                                .negativeText(isDoor_open? "ĐÓNG CỬA": "")
                                .onNegative(new MaterialDialog.SingleButtonCallback() {
                                    @Override
                                    public void onClick(MaterialDialog dialog, DialogAction which) {
                                        // TODO
                                        isDoor_open = true;
                                        process_door();
                                    }
                                });
                        dialog = builder.build();
                        if(!isAlert_7) {

                            isAlert_7 = true;
                            dialog.show();
                        }
                    }
                }else if (topic.contains("res")){
                    if(message.toString().equals("11")) {
                        isLED_open = true;
                        myLoadingButton1.setButtonLabel("TẮT ĐÈN");
                        myLoadingButton1.showNormalButton();
                    }
                    else if (message.toString().equals("10")) {
                        isLED_open = false;
                        myLoadingButton1.setButtonLabel("MỞ ĐÈN");
                        myLoadingButton1.showNormalButton();
                    }
                    if(message.toString().equals("21")) {
                        isDoor_open = true;
                        myLoadingButton2.setButtonLabel("ĐÓNG CỬA");
                        myLoadingButton2.showNormalButton();
                    }
                    else if (message.toString().equals("20")) {
                        isDoor_open = false;
                        myLoadingButton2.setButtonLabel("MỞ CỬA");
                        myLoadingButton2.showNormalButton();
                    }
                }
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {

            }
        });
    }
}