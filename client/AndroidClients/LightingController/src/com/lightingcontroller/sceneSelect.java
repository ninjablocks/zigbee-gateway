/**************************************************************************************************
  Filename:       sceneSelect.java
  Revised:        $$
  Revision:       $$

  Description:    Scene UI

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

import java.util.ArrayList;
import java.util.List;

import com.lightingcontroller.R;
import com.lightingcontroller.Zigbee.ZigbeeAssistant;
import com.lightingcontroller.Zigbee.ZigbeeGroup;
import com.lightingcontroller.Zigbee.ZigbeeScene;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.AdapterView.OnItemSelectedListener;

public class sceneSelect extends Activity {

	private static int currGroup = 0;
	private static Spinner groupSpinner;
	private static ArrayAdapter<String> groupSpinnerAdapter;
	
	private static int currScene;
	private Spinner sceneSpinner;
	private ArrayAdapter<String> sceneSpinnerAdapter;
	
	ProgressDialog bar;
	
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.overridePendingTransition(R.anim.animation_enter_l, R.anim.animation_leave_r);  
                
        setContentView(R.layout.sceneview);        
          
        List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();
        List<String> groupNameList = new ArrayList<String>();
        for (int i = 0 ; i < groupList.size() ; i++)
        {
        	groupNameList.add(groupList.get(i).getGroupName());        
        }                 
    	groupSpinner = (Spinner) findViewById(R.id.groupSpinner);
    	groupSpinnerAdapter = new ArrayAdapter<String>(this,
        		android.R.layout.simple_spinner_item, groupNameList);
    	groupSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        groupSpinner.setAdapter(groupSpinnerAdapter);
        	
        addItemsOnGroupSpinner();
        addListenerOnGroupSpinnerItemSelection();  

        List<ZigbeeScene> sceneList = ZigbeeAssistant.getScenes();
        List<String> sceneNameList = new ArrayList<String>();
        for (int i = 0 ; i < sceneList.size() ; i++)
        {
        	sceneNameList.add(sceneList.get(i).getSceneName());        
        }                 
    	sceneSpinner = (Spinner) findViewById(R.id.sceneSelectSpinner);
    	sceneSpinnerAdapter = new ArrayAdapter<String>(this,
        		android.R.layout.simple_spinner_item, sceneNameList);
    	sceneSpinnerAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
    	sceneSpinner.setAdapter(sceneSpinnerAdapter);        

        addItemsOnSceneSpinner();
        addListenerOnSceneSpinnerItemSelection();      	
    }    

    // add items into spinner dynamically
    public void addItemsOnGroupSpinner() {          
        List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();
        groupSpinnerAdapter.clear();
        for (int i = 0 ; i < groupList.size() ; i++)
        {        	
        	groupSpinnerAdapter.add(groupList.get(i).getGroupName());
        }
    }
   
    public void addListenerOnGroupSpinnerItemSelection() {
    	groupSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
    	    @Override
    	    public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
    	    	currGroup = position;
    	    	currScene = 0;
    	    	//change the scene spinner to reflect the new group 
    	    	addItemsOnSceneSpinner();
      	  }
      	 
      	  @Override
      	  public void onNothingSelected(AdapterView<?> arg0) {
      		// TODO Auto-generated method stub
      		currGroup = -1;  
      	  }
    	});
    }
    
    // add items into spinner dynamically
    public void addItemsOnSceneSpinner() {          
        List<ZigbeeScene> sceneList = ZigbeeAssistant.getScenes();
        List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();
        sceneSpinnerAdapter.clear();
        if(currGroup != -1)
        {	        
	        for (int i = 0 ; i < sceneList.size() ; i++)
	        {    
	        	if(groupList.get(currGroup).getGroupId() == sceneList.get(i).getGroupId())
	        	{
	        		sceneSpinnerAdapter.add(sceneList.get(i).getSceneName());
	        	}
	        }
        }
    }
   
    public void addListenerOnSceneSpinnerItemSelection() {
    	sceneSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
    	    @Override
    	    public void onItemSelected(AdapterView<?> parentView, View selectedItemView, int position, long id) {
    	    	currScene = position;
      	  }
      	 
      	  @Override
      	  public void onNothingSelected(AdapterView<?> arg0) {
      		// TODO Auto-generated method stub
      		currScene = -1;  
      	  }
    	});
    }    


    public void newSceneButton(View view) {

    	final EditText t = new EditText(this);

    	new AlertDialog.Builder(this)
		.setTitle("Create Scene")
		.setMessage("Enter the new scene name")
		.setView(t)
		.setPositiveButton("OK",          
		new DialogInterface.OnClickListener()
		{			
			public void onClick(DialogInterface dialoginterface,int i){	
				List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();				
				ZigbeeAssistant.newScene(t.getText().toString(), groupList.get(currGroup).getGroupId() );							
		    	//set selection to the new group, which will be last as this is a new one						
		    	currScene = sceneSpinnerAdapter.getCount();				    	
		    	new waitRspTask().execute("Create Scene");				    	
		    	sceneSpinner.setSelection(currScene);
		    	//update the spinner
		    	addItemsOnSceneSpinner();					    					    
			}		
		})	    	    		
		.setNegativeButton("Cancel", null)
		.show();	    	
	}
         


    class waitRspTask extends AsyncTask<String , Integer, Void>
    {
    	private boolean rspSuccess;
    	String param;
        @Override
        protected void onPreExecute()
        {
            bar = new ProgressDialog(sceneSelect.this);
            bar.setMessage("Processing..");
            bar.setIndeterminate(true);
            bar.show();
        } 
        @Override
        protected Void doInBackground(String... params) 
        {
        	param = params[0];
        	List<ZigbeeScene> sceneList = ZigbeeAssistant.getScenes();
        	//currGroup = 0;
            for(int i=0;i<10;i++)
            {
            	if(currScene < sceneList.size())
            	{
	            	//check if group response updated the groupId          
	            	if (sceneList.get(currScene).getStatus() == ZigbeeScene.sceneStatusActive)
	            	{
	            		rspSuccess = true;
	            		return null;
	            	}
            	}
	            	
	            try
	            {
	                Thread.sleep(500);
	            }
	            catch(Exception e)
	            {
	                System.out.println(e);
	            }    
            	            
            }
            					
            rspSuccess = false;    
            return null;
        }
        @Override
        protected void onPostExecute(Void result) 
        {
            bar.dismiss();
            
            if (rspSuccess == false)
        	{
    	    	AlertDialog show = new AlertDialog.Builder(sceneSelect.this)
    			.setTitle(param)
    			.setMessage("No response from gateway. " + param + " failed.")
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
        

    public void sceneStoreButton(View view) {
    	
    	if(currGroup != -1)
    	{
	    	AlertDialog show = new AlertDialog.Builder(sceneSelect.this)
			.setTitle("Store Scene to group")
			.setMessage("Add Scene Selected Light to Group " + currGroup)
			.setPositiveButton("OK",             
			new DialogInterface.OnClickListener()
			{			
				public void onClick(DialogInterface dialoginterface,int i){	
					List<ZigbeeScene> sceneList = ZigbeeAssistant.getScenes();
					List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();									
					ZigbeeAssistant.storeScene(sceneList.get(currScene).getSceneName(), groupList.get(currGroup).getGroupId() );						
				}		
			})	    	    		
			.setNegativeButton("Cancel", null)
			.show();	    	    		
    	}
    	else
    	{
	    	AlertDialog show = new AlertDialog.Builder(sceneSelect.this)
			.setTitle("Store Scene to group")
			.setMessage("No scene selected ")
			.setPositiveButton("OK",             
			new DialogInterface.OnClickListener()
			{			
				public void onClick(DialogInterface dialoginterface,int i){				    	
				}		
			})	    	    		
			.show();	    	    		
    	}    		

    }     

    public void sceneRestoreButton(View view) {
		List<ZigbeeScene> sceneList = ZigbeeAssistant.getScenes();	
		List<ZigbeeGroup> groupList = ZigbeeAssistant.getGroups();	
		ZigbeeAssistant.recallScene(sceneList.get(currScene).getSceneName(), groupList.get(currGroup).getGroupId() );
    }  
}

