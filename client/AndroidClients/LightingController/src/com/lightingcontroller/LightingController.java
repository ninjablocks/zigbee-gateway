/**************************************************************************************************
  Filename:       LightingController.java
  Revised:        $$
  Revision:       $$

  Description:    Lighting Controller Main Activity 

  Copyright (C) {2012} Texas Instruments Incorporated - http://www.ti.com/


   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

     Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the
     distribution.

     Neither the name of Texas Instruments Incorporated nor the names of
     its contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
**************************************************************************************************/

package com.lightingcontroller;

import java.util.concurrent.TimeUnit;

import android.app.Activity;
import android.app.ProgressDialog;
import android.os.AsyncTask;
import android.os.Bundle;
import android.widget.EditText;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.app.AlertDialog;

import com.lightingcontroller.Zigbee.ZigbeeAssistant;
import com.lightingcontroller.Zigbee.ZigbeeSrpcClient;

public class LightingController extends Activity{

    private static final String PREFS_NAME = "MyPrefsFile";
    public int selectedDevice = 0;
    ProgressDialog bar;
    
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState); 
        setContentView(R.layout.main);
       
        new ZigbeeAssistant();                
        
        new waitRspTask().execute("Connecting to gateway");        
    }
   		    
    class waitRspTask extends AsyncTask<String , Integer, Void>
    {
    	private boolean rspSuccess = false;
    	String param;
    	String gatewayIpAddr;
    	
        @Override
        protected void onPreExecute()
        {
            bar = new ProgressDialog(LightingController.this);
            bar.setMessage("Finding Gateway");
            bar.setIndeterminate(true);
            bar.show();
        } 
        @Override
        protected Void doInBackground(String... params) 
        {
        	param = params[0];
        	
			//retrieve path setting
			SharedPreferences preferences = getSharedPreferences(PREFS_NAME, 0);
	    	String gatewayIpAddr = preferences.getString("gatewayIpAddr", null);
			
			if(gatewayIpAddr != null)
			{
				ZigbeeSrpcClient.setGatewayIp(gatewayIpAddr);					
				if(ZigbeeSrpcClient.clientConnect() == 0)
				{
			        ZigbeeSrpcClient.getDevices();
			        ZigbeeSrpcClient.discoverGroups();
			        ZigbeeSrpcClient.discoverScenes();
			        //wait for responses
			        try { TimeUnit.MILLISECONDS.sleep(200); } catch (InterruptedException e) {e.printStackTrace();}  
			          
			        startActivity(new Intent(LightingController.this, zbLighitngMain.class));
			          
					rspSuccess = true;
				}
			}
			
			return null;
        }
        @Override
        protected void onPostExecute(Void result) 
        {
            bar.dismiss();
            
            if (rspSuccess == false)
        	{
             	final EditText t = new EditText(LightingController.this);
             	t.setText("192.168.1.111");

            	new AlertDialog.Builder(LightingController.this)
        		.setTitle("Gateway Address")
        		.setMessage("Please Enter the HA Gateway IP Address")
        		.setView(t)
        		.setPositiveButton("OK",             
        		new DialogInterface.OnClickListener()
        		{			
        			public void onClick(DialogInterface dialoginterface,int i){	
        		    	String gatewayIpAddr;
        		    	gatewayIpAddr = t.getText().toString();

        				//Store path setting
        		    	SharedPreferences preferences = getSharedPreferences(PREFS_NAME, 0);
        		    	SharedPreferences.Editor editor = preferences.edit();
        		    	editor.putString("gatewayIpAddr", gatewayIpAddr); // value to store
        		    	editor.commit();        		            				
        		    	
        				new waitRspTask().execute("Connecting to gateway");
          			}		
        		})
        		.setNegativeButton("Cancel", null)
        		.show();     	    		
        	}
            else
            {
            	finish();
            }
            
        }
    }        
}
