/**************************************************************************************************
  Filename:       zbLighitngMain.java
  Revised:        $$
  Revision:       $$

  Description:    ZigBee Lighting Main UI

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

import android.app.Activity;
import android.app.ProgressDialog;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.app.AlertDialog;

import java.util.ArrayList;
import java.util.List; 
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.TimeUnit;


import com.lightingcontroller.Zigbee.ZigbeeAssistant;
import com.lightingcontroller.Zigbee.ZigbeeDevice;
import com.lightingcontroller.Zigbee.ZigbeeGroup;
import com.lightingcontroller.ColourPicker;

public class zbLighitngMain extends Activity {
    /** Called when the activity is first created. */	
 
	
	public static ColourPicker colourPicker;
	private static ZigbeeGroup currGroup;	
	private static ZigbeeDevice currDevice;
	private Spinner deviceSpinner;
	private ArrayAdapter<String> deviceSpinnerAdapter;
	private int currentDeviceSpinnerSelection=0;	
	
	ProgressDialog bar;	
	
    @Override
     public void onCreate(Bundle savedInstanceState) {
        
    	super.onCreate(savedInstanceState);

    	DisplayMetrics metrics = new DisplayMetrics();
		WindowManager wm = (WindowManager) this.getSystemService(Context.WINDOW_SERVICE);
		wm.getDefaultDisplay().getMetrics(metrics);		
		
    	if(metrics.widthPixels > metrics.heightPixels)
    	{
    		setContentView(R.layout.zblightingmain);  
    	}
    	else
    	{
    		setContentView(R.layout.zblightingmainportraite);  		
    	}
		    	
        setContentView(R.layout.zblightingmain);
        
        colourPicker = (ColourPicker)findViewById(R.id.colourPick);	

        SeekBar levelControl = (SeekBar)findViewById(R.id.seekBarLevel);
        levelControl.setMax(0xFF);
        
        levelControl.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

		   @Override
		   public void onStopTrackingTouch(SeekBar arg0) {
		    // TODO Auto-generated method stub
	
		   }
	
		   @Override
		   public void onStartTrackingTouch(SeekBar arg0) {
		    // TODO Auto-generated method stub
	
		   }

		   @Override
		   public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
			   sendLevelChange((byte)arg1);
		   }
        });

		
		List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();
        List<String> devNameList = new ArrayList<String>();
        for (int i = 0 ; i < devList.size() ; i++)
        {
        	devNameList.add(devList.get(i).Name);        
        }                 
    	deviceSpinner = (Spinner) findViewById(R.id.deviceSpinner);
    	deviceSpinnerAdapter = new ArrayAdapter<String>(this,
        		android.R.layout.simple_spinner_item, devNameList);
    	deviceSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    	deviceSpinner.setAdapter(deviceSpinnerAdapter);
        
        addItemsOnDeviceSpinner();
        addListenerOnDeviceSpinnerItemSelection();  	   
    }    

    // add items into spinner dynamically
    public void addItemsOnDeviceSpinner() {  
    	//add devices
        List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();
        List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups(); 
        deviceSpinnerAdapter.clear();
        for (int i = 0 ; i < devList.size() ; i++)
        {             	
        	deviceSpinnerAdapter.add(devList.get(i).Name);
        }
        
        //add groups
        for (int i = 0 ; i < groupList.size() ; i++)
        {        	
        	deviceSpinnerAdapter.add("group: " + groupList.get(i).getGroupName());
        }        
    }
   
    public void addListenerOnDeviceSpinnerItemSelection() {    	
    	deviceSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
    	    @Override
    	    public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
    	    	List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();
    	    	List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups(); 	        	    	
    	    	currentDeviceSpinnerSelection = position;

    	    	if(currentDeviceSpinnerSelection < devList.size() )
    	    	{
    	    		currDevice = devList.get(currentDeviceSpinnerSelection);
    	    		
    	    		currDevice.clearCurrentStateUpdated();
    	    		currDevice.clearCurrentLevelUpdated();
    	    		currDevice.clearCurrentHueUpdated();
    	    		currDevice.clearCurrentSatUpdated();    	    		
    	    		ZigbeeAssistant.getDeviceState(currDevice);    	    		
    	    		ZigbeeAssistant.getDeviceLevel(currDevice);
    	    		ZigbeeAssistant.getDeviceHue(currDevice);
    	    		ZigbeeAssistant.getDeviceSat(currDevice);
    	    		//new waitRspTask().execute("Device Select");	
    	    		
    	    		currGroup = null;
    	    	}
    	    	else
    	    	{
    	    		currDevice = null;
    	    		currGroup = groupList.get(currentDeviceSpinnerSelection - devList.size());
    	    	}    	    	   	    	
      	  }
      	 
      	  @Override
      	  public void onNothingSelected(AdapterView<?> arg0) {
      	  }
    	});
    } 
	
    class waitRspTask extends AsyncTask<String , Integer, Void>
    {
    	private boolean rspSuccess;
    	String param;
        @Override
        protected void onPreExecute()
        {
            bar = new ProgressDialog(zbLighitngMain.this);
            bar.setMessage("Finding Device");
            bar.setIndeterminate(true);
            bar.show();
        } 
        @Override
        protected Void doInBackground(String... params) 
        {
        	param = params[0];
        	
        	try { TimeUnit.MILLISECONDS.sleep(100); } catch (InterruptedException e) {e.printStackTrace();}
        	
            for(int i=0;i<100;i++)
            {	
            	if( currDevice.getCurrentStateUpdated() &&
                		currDevice.getCurrentLevelUpdated() &&
                		currDevice.getCurrentHueUpdated() &&
                		currDevice.getCurrentSatUpdated() )
            	{
            		colourPicker.upDateColorPreivew( currDevice.getCurrentHue(), currDevice.getCurrentSat() );
            		//todo change level bar and represent level and state on the color preview
            		rspSuccess = true;            		
            		return null;
            	}
            	
            	try { TimeUnit.MILLISECONDS.sleep(10); } catch (InterruptedException e) {e.printStackTrace();}            	
            	
	            if( ((i % 30) == 0) && (i > 0) )
	            {
	            	if( !currDevice.getCurrentStateUpdated() )
	            	{
	            		ZigbeeAssistant.getDeviceState(currDevice);
	            	}
	            	if(	!currDevice.getCurrentLevelUpdated() )
	            	{
	            		ZigbeeAssistant.getDeviceLevel(currDevice);
	            	}
	            	if(	!currDevice.getCurrentHueUpdated() )
	            	{	            	
	            		ZigbeeAssistant.getDeviceHue(currDevice);
	            	}
	            	if(	!currDevice.getCurrentSatUpdated() )
	            	{	            	
	            		ZigbeeAssistant.getDeviceSat(currDevice);
	            	}	            	            	
	            }
            	            
            }
            	
        	if( currDevice.getCurrentStateUpdated() ||
            		currDevice.getCurrentLevelUpdated() ||
            		currDevice.getCurrentHueUpdated() ||
            		currDevice.getCurrentSatUpdated() )
        	{
        		//we got a response but not all HSV value, maybe network is very busy,
        		//try to handle this gracefully by setting preview to last known values.
        		colourPicker.upDateColorPreivew( currDevice.getCurrentHue(), currDevice.getCurrentSat() );
        		//todo change level bar and represent level and state on the color preview
        		rspSuccess = true;            		
        		return null;
        	}
        	else
        	{
        		//We did not get any response, maybe the device is off the network
        		rspSuccess = false;    
        		return null;
        	}
        }
        @Override
        protected void onPostExecute(Void result) 
        {
            bar.dismiss();
            
            if (rspSuccess == false)
        	{
    	    	AlertDialog show = new AlertDialog.Builder(zbLighitngMain.this)
    			.setTitle(param)
    			.setMessage("Device " + zbLighitngMain.currDevice.Name + " Not Responding.")
    			.setPositiveButton("OK",             
    			new DialogInterface.OnClickListener()
    			{			
    				public void onClick(DialogInterface dialoginterface,int i){				    	
    				}		
    			})	    	    		
    			.show();
        	}
            
        }
    }    

    protected void onResume() {
        super.onResume();
        
		List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();
        List<String> devNameList = new ArrayList<String>();
        for (int i = 0 ; i < devList.size() ; i++)
        {
        	devNameList.add(devList.get(i).Name);        
        }                 
    	deviceSpinner = (Spinner) findViewById(R.id.deviceSpinner);
    	deviceSpinnerAdapter = new ArrayAdapter<String>(this,
        		android.R.layout.simple_spinner_item, devNameList);
    	deviceSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    	deviceSpinner.setAdapter(deviceSpinnerAdapter);
        
        addItemsOnDeviceSpinner();
        addListenerOnDeviceSpinnerItemSelection();
       
    } 
    
    @Override       
    public void onBackPressed() {    
    	new AlertDialog.Builder(this)
		.setTitle("Close")
		.setMessage("Are you sure you want to exit?")
		.setPositiveButton("OK",          
		new DialogInterface.OnClickListener()
		{			
			public void onClick(DialogInterface dialoginterface,int i){				    				    	
                zbLighitngMain.this.finish();					    					    
			}		
		})	    	    		
		.setNegativeButton("Cancel", null)
		.show();	    	    	
    	
        return;
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.layout.optionmenu, menu);
        return true;
    }  
    
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
    	super.onConfigurationChanged(newConfig);

    	if(newConfig.orientation == Configuration.ORIENTATION_PORTRAIT)
    	{
    		setContentView(R.layout.zblightingmain);  
    	}
    	else
    	{
    		setContentView(R.layout.zblightingmainportraite);  		
    	}

		List<ZigbeeDevice> devList = ZigbeeAssistant.getSwitchable();
        List<String> devNameList = new ArrayList<String>();
        for (int i = 0 ; i < devList.size() ; i++)
        {
        	devNameList.add(devList.get(i).Name);        
        }                 
    	deviceSpinner = (Spinner) findViewById(R.id.deviceSpinner);
    	deviceSpinnerAdapter = new ArrayAdapter<String>(zbLighitngMain.this,
        		android.R.layout.simple_spinner_item, devNameList);
    	deviceSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    	deviceSpinner.setAdapter(deviceSpinnerAdapter);
        
        addItemsOnDeviceSpinner();
        addListenerOnDeviceSpinnerItemSelection();    	

    }    
    
    public static ZigbeeDevice getCurrentDevice()
    {
    	return currDevice;    	
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.optionMenuGroups: 
            	startActivity(new Intent(zbLighitngMain.this, groupSelect.class));
                break;                 
            case R.id.optionMenuScenes: 
            	startActivity(new Intent(zbLighitngMain.this, sceneSelect.class));
                break;                
            case R.id.optionMenuBinding:
            	startActivity(new Intent(zbLighitngMain.this, bindSelect.class));
                break;
            case R.id.optionMenuListDevices:
            {
            	AlertDialog alert;
        		String devicesText = ZigbeeAssistant.getInfoString();  
        		AlertDialog.Builder builder = new AlertDialog.Builder(this);
        		builder.setTitle("Zigbee Devices")
        			   .setMessage(devicesText)
        		       .setCancelable(false)
        		       .setNegativeButton("Okay", new DialogInterface.OnClickListener() {
        		           public void onClick(DialogInterface dialog, int id) {
        		        	   dialog.cancel();
        		           }
        		       });
        		alert = builder.create();
        		alert.show();
            }            	
                break;
        }
        return true;
    }
 
    public void deviceChangeNameButton(View view)
    { 
    	String Title = "Set Name";
    	String Msg = "Please Enter the name of the device";
     	final EditText t = new EditText(this);

    	new AlertDialog.Builder(this)
		.setTitle(Title)
		.setMessage(Msg)
		.setView(t)
		.setNegativeButton("CANCEL", null)
		.setPositiveButton("OK",             
		new DialogInterface.OnClickListener()
		{			
			public void onClick(DialogInterface dialoginterface,int i){	
		        ZigbeeAssistant.setDeviceName( currDevice.Name, t.getText().toString());		
		        
		        deviceSpinnerAdapter.clear();
		        List<ZigbeeDevice> tList = ZigbeeAssistant.getDevices();
		        for (int j = 0 ; j < tList.size() ; j++)
		        {
		        	if (tList.get(j).hasColourable || tList.get(j).hasSwitchable || tList.get(j).hasDimmable)
		        	{        	
		        		deviceSpinnerAdapter.add(tList.get(j).Name);
		        	}
		        }    	
		    	
		    	
  			}		
		})
		.show();     	    		
    }    	
    
    public void nextPageButton(View view) {  
    	startActivity(new Intent(zbLighitngMain.this, sceneSelect.class));    	
	}     
	
	public void onCmdButton(View view) {
		if(currDevice != null)
			ZigbeeAssistant.setDeviceState(currDevice, true);
		else if (currGroup != null)
		{
            ZigbeeAssistant.setDeviceState(currGroup, true);
		}
    }
   

    public void offCmdButton(View view) {
		if(currDevice != null)
		{
			ZigbeeAssistant.setDeviceState(currDevice, false);
		}
		else if (currGroup != null)
		{
            ZigbeeAssistant.setDeviceState(currGroup, false);
		}
    }

    public void tlCmdButton(View view) {

    }                    

    public void sendHueSatChange(byte hue, byte sat)
    {    	
		if(currDevice != null)    
		{
			ZigbeeAssistant.setDeviceHueSat(currDevice,hue, sat);
		}
		else if (currGroup != null)
		{
            ZigbeeAssistant.setDeviceHueSat(currGroup,hue, sat);
		}
    }
    
    public void sendLevelChange(byte level)
    {
		if(currDevice != null)    	
		{
			ZigbeeAssistant.setDeviceLevel(currDevice, (char) level);
		}
		else if (currGroup != null)
		{
            ZigbeeAssistant.setDeviceLevel(currGroup, (char) level);
		}    	
    }	
}