module.exports = [
	{ 
    	"type": "heading", 
    	"defaultValue": "Screeps Time 1.8",
		  "size": 1,
	},
  {
    "type": "text",
    "defaultValue": "This watchface connects to your screeps account through your phone. Your email/password will only be saved locally."
  },
  {
    "type": "section",
    "items": [
			{
				"type": "heading",
				"defaultValue": "Screeps Account"
			},  
			{
				"type": "input",
				"messageKey": "CONFIG_SERVER",
				"label": "Server (+ Port if private server)",
				"defaultValue": "screeps.com",
        "attributes": {
          "type": "text",
          "limit": "128"
        }
			},  
			{
				"type": "input",
				"messageKey": "CONFIG_EMAIL",
				"label": "Email Address (Username for private servers)",
				"defaultValue": "",
        "attributes": {
          "type": "text",
          "limit": "128"
        }
			},      
			{
				"type": "input",
				"messageKey": "CONFIG_PASSWORD",
				"label": "Password",
				"defaultValue": "",
        "attributes": {
          "type": "password",
          "limit": "64"
        }
			},      
			{
				"type": "select",
				"messageKey": "CONFIG_POLL_SCREEPS",
				"defaultValue": "1",
				"label": "Poll API Every",
				"options": [
					{ 
						"label": "1 Minute",
						"value": "1" 
					},
					{ 
						"label": "5 minutes",
						"value": "5" 
					},
					{ 
						"label": "10 minutes",
						"value": "10" 
					},
					{ 
						"label": "15 minutes",
						"value": "15" 
					},
					{ 
						"label": "30 minutes",
						"value": "30" 
					},
					{ 
						"label": "1 Hour",
						"value": "60" 
					}
				]
			},   
      {
        "type": "text",
        "defaultValue": "Reducing poll rate will help save battery life."
      }      
    ]
  },         
	{
		"type": "section",
		"items": [
			{
				"type": "heading",
				"defaultValue": "Screeps - Unread Mail"
			},
      {
        "type": "text",
        "defaultValue": "If this is enabled, the specified rail data will be replaced with a 'New Messages' notification when you have unread messages."
      },          
      {
				"type": "toggle",
				"messageKey": "CONFIG_MAIL_SHOW",
				"label": "Show New Messages",
				"defaultValue": false,
			},      
			{
				"type": "select",
				"messageKey": "CONFIG_MAIL_RAIL",
				"defaultValue": "0",
				"label": "Notification Rail",
				"options": [
					{ 
						"label": "Top-most",
						"value": "0" 
					},
					{ 
						"label": "Second Top",
						"value": "1" 
					},
					{ 
						"label": "Second Bottom",
						"value": "2" 
					},
					{ 
						"label": "Bottom-most",
						"value": "3" 
					}
				]
			},
		]
 	},      
	{
		"type": "section",
		"items": [
			{
				"type": "heading",
				"defaultValue": "Battery Gauge"
			},
      {
        "type": "text",
        "defaultValue": "If this is enabled, a small sidebar will displayed to the right of the time. It colors changes as it drops and blinks when near empty."
      },          
      {
				"type": "toggle",
				"messageKey": "CONFIG_BATTERY_MAIN",
				"label": "Show Battery Sidebar",
				"defaultValue": true,
			},        
      {
        "type": "text",
        "defaultValue": "If this is enabled, the specified rail data will be replaced with a battery progress bar. If on the same rail of as messages, messages will take priority."
      },          
			{
				"type": "select",
				"messageKey": "CONFIG_BATTERY_THRESHOLD",
				"defaultValue": "0",
				"label": "Only display below",
				"options": [
					{ "label": "Never Show", "value": "0" },
					{ "label": "10%", "value": "10" },
					{ "label": "20%", "value": "20" },
					{ "label": "30%", "value": "30" },
					{ "label": "40%", "value": "40" },
					{ "label": "50%", "value": "50" },
					{ "label": "60%", "value": "60" },
					{ "label": "70%", "value": "70" },
					{ "label": "80%", "value": "80" },
					{ "label": "90%", "value": "90" },
					{ "label": "Always Display", "value": "100" },  
				]
			},  
			{
				"type": "select",
				"messageKey": "CONFIG_BATTERY_RAIL",
				"defaultValue": "3",
				"label": "Notification Rail",
				"options": [
					{ 
						"label": "Top-most",
						"value": "0"
					},
					{ 
						"label": "Second Top",
						"value": "1"
					},
					{ 
						"label": "Second Bottom",
						"value": "2"
					},
					{ 
						"label": "Bottom-most",
						"value": "3"
					}
				]
			},
		]
 	},        
  {
    "type": "section",
    "items": [
			{
				"type": "heading",
				"defaultValue": "Weather"
			},  
      {
        "type": "text",
        "defaultValue": "Weather provided by OpenWeatherMap.org - You can register an API key for free."
      },          
      {
				"type": "toggle",
				"messageKey": "CONFIG_USE_WEATHER",
				"label": "Display Weather",
				"defaultValue": false,
			},      
      {
				"type": "toggle",
				"messageKey": "CONFIG_USE_FAHRENHEIT",
				"label": "Use Fahrenheit",
				"defaultValue": true,
			},      
			{
				"type": "select",
				"messageKey": "CONFIG_POLL_WEATHER",
				"defaultValue": "30",
				"label": "Poll API Every",
				"options": [
					{ 
						"label": "1 Minute",
						"value": "1" 
					},
					{ 
						"label": "5 minutes",
						"value": "5" 
					},
					{ 
						"label": "10 minutes",
						"value": "10" 
					},
					{ 
						"label": "15 minutes",
						"value": "15" 
					},
					{ 
						"label": "30 minutes",
						"value": "30" 
					},
					{ 
						"label": "1 Hour",
						"value": "60" 
					}
				]
			},    
			{
				"type": "input",
				"messageKey": "CONFIG_WEATHER_API_KEY",
				"label": "OpenWeatherMap API Key",
				"defaultValue": "",
        "attributes": {
          "type": "text",
          "limit": "128"
        }
			},      
			{
				"type": "input",
				"messageKey": "CONFIG_WEATHER_CITYID",
				"label": "City ID (Optional, use GPS if blank)",
				"defaultValue": "",
        "attributes": {
          "type": "text",
          "limit": "64"
        }
			},      
    ]
  },      
	{
		"type": "section",
		"items": [
			{
				"type": "heading",
				"defaultValue": "Bluetooth"
			},
      {
				"type": "toggle",
				"messageKey": "CONFIG_BLUETOOTH_MAIN",
				"label": "Show Bluetooth Indicator",
				"defaultValue": true,
			},         
      {
				"type": "toggle",
				"messageKey": "CONFIG_WAKE_ON_CONNECT",
				"label": "Wake on Reconnect",
				"defaultValue": true,
			},      
			{
				"type": "select",
				"messageKey": "CONFIG_BLUETOOTH_RECONNECT",
				"defaultValue": "3",
				"label": "Reconnect Vibration",
				"options": [
					{ 
						"label": "None",
						"value": "0" 
					},
					{ 
						"label": "Short",
						"value": "1" 
					},
					{ 
						"label": "Long",
						"value": "2" 
					},
					{ 
						"label": "Double",
						"value": "3" 
					}
				]
			},
      {
				"type": "toggle",
				"messageKey": "CONFIG_WAKE_ON_DISCONNECT",
				"label": "Wake on Disconnect",
				"defaultValue": true,
			},        
			{
				"type": "select",
				"messageKey": "CONFIG_BLUETOOTH_DISCONNECT",
				"defaultValue": "2",
				"label": "Disconnect Vibration",
				"options": [
					{ 
						"label": "None",
						"value": "0" 
					},
					{ 
						"label": "Short",
						"value": "1" 
					},
					{ 
						"label": "Long",
						"value": "2" 
					},
					{ 
						"label": "Double",
						"value": "3" 
					}
				]
			},
		]
 	},
	{
		"type": "submit",
		"defaultValue": "Save Settings"
	}
];

