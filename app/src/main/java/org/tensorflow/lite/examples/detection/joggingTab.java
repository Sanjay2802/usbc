package org.tensorflow.lite.examples.detection;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.os.StrictMode;
import android.view.View;
import android.widget.EditText;

import java.io.IOException;
import java.io.PrintWriter;
import java.net.Socket;

public class joggingTab extends AppCompatActivity {
    Socket s;
    private static joggingTab instance;
    PrintWriter printWriter = null;
    String feedrate=null;
    String stepsize=null;
    String g="G21G91";


    EditText speed;
    EditText step;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_jogging_tab);
        StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
        StrictMode.setThreadPolicy(policy);
        speed=(EditText) findViewById(R.id.feedrate);
        step=(EditText) findViewById(R.id.stepsize);

        try {
             s = new Socket(/*ip address :*/"192.168.1.9",/*port :*/ 9999);
             s.setKeepAlive(true);
             System.out.println(s.isConnected());
            printWriter = new PrintWriter(s.getOutputStream());
        } catch (IOException e) {
            e.printStackTrace();
        }
        setalgo();

        instance = this;
    }
    public static joggingTab getInstance() {
        return instance;
    }


    public void up(View view) {


        printWriter.write(g+"Y"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }

    public void up(){
        printWriter.write(g+"Y"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }
    public void down(View view) {
        printWriter.write(g+"Y-"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }

    public void down(){
        printWriter.write(g+"Y-"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }

    public void left(View view) {
        printWriter.write(g+"X-"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();

    }
    public void left(){
        printWriter.write(g+"X-"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }
    public void right(View view) {
        printWriter.write(g+"X"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }

    public void right(){
        printWriter.write(g+"X"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();



    }
    public void leftup(View view) {
        printWriter.write(g+"X-"+stepsize+"Y"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }
    public void rightup(View view) {
        printWriter.write(g+"X"+stepsize+"Y"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }
    public void leftdown(View view) {
        printWriter.write(g+"X-"+stepsize+"Y-"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }
    public void rightdown(View view) {
        printWriter.write(g+"X"+stepsize+"Y-"+stepsize+"F"+feedrate+"\n");
        printWriter.flush();


    }

    public void sethome(View view) {
        printWriter.write("G10 P0 L20 X0 Y0 Z0"+"\n");
        printWriter.flush();


    }
    public void gohome(View view) {
        printWriter.write("G21G90 G0Z5"+"\n"+"G90 G0 X0 Y0"+"\n"+"G90 G0 Z0"+"\n");
        printWriter.flush();


    }

    public void back(View view) {
        Intent i=new Intent(getApplicationContext(),DetectorActivity.class);
        startActivity(i);



    }


















    public void setalgo() {
        feedrate= String.valueOf(speed.getText());
        stepsize= String.valueOf(step.getText());



    }


}