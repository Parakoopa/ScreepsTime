var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

var keys = require('message_keys');
// console.log(JSON.stringify(keys, null, 2));

require('./gcolor');
var pako = require('./pako.js');

var xToken;

var info = {};
var lastInfo = {};

var getPrefix = function() {
  var protocol = "http";
  var serverUrl = localStorage.getItem('server') || "screeps.com";
  if (serverUrl == "screeps.com") {
    // TODO: Should propably allow HTTPS for custom servers as well.
    protocol = "https";
  }
  return protocol+'://'+serverUrl+'/api/';
};

var officialServer = function() {
  return !localStorage.getItem('server') || localStorage.getItem('server') == "screeps.com";
};

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

/* Convenience routine for making authorized requests to screeps. Needs to
 * replace X-Token if a new one was supplied in a response.
 * Credit to Dormando for this. https://github.com/screepers/pcreeps/blob/master/src/js/app.js
 */
var xhrScreepsRequest = function (url, type, callback, data, basicAuth) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var newToken = this.getResponseHeader('X-Token');
    if (newToken) xToken = newToken;
    
    try { var reply = JSON.parse(this.responseText); } catch(e) { var reply = this.responseText; }
    callback(reply);
  };
  if (basicAuth) {
    xhr.open(type, url);
    xhr.setRequestHeader("Authorization", "Basic " + btoa(basicAuth.user + ":" + basicAuth.password));
  } else {
    xhr.open(type, url);
  }
  if (type == "POST" && data) xhr.setRequestHeader('Content-Type', 'application/json; charset=utf-8');
  if (xToken) {
    xhr.setRequestHeader('X-Username', xToken);
    xhr.setRequestHeader('X-Token', xToken);
  }
  if (data) {
      xhr.send(JSON.stringify(data));
  } else {
      xhr.send();
  }
};

var doScreepsLogin = function(callback) {
  var emailAuth = null, basicAuth = null;
  if (officialServer()) {
    emailAuth = { "email": localStorage.getItem('email'), "password": localStorage.getItem('password') };
    console.log("Login to official Screeps server via Email");
  } else {
    basicAuth = { "user" : localStorage.getItem('email'), "password": localStorage.getItem('password') };
    console.log("Login to custom Screeps server via Basic Auth");
  }
  xhrScreepsRequest(getPrefix() + "auth/signin", 'POST', function(res) {
    // console.log("LOGIN RESPONSE:");  
    // console.log(JSON.stringify(res, null, 2));
    console.log("Login to Screeps API was " + (res.token ? "successful." : "a catastrophic failure."));
    xToken = res.token;
    callback(res.token ? true : false);
  }, emailAuth, basicAuth);
}

var doScreepsUnreadMessage = function(callback) {
	xhrScreepsRequest(getPrefix() + "user/messages/unread-count", 'GET', function(res) {
		info.unreadMessages = (res.count ? res.count : 0);
		console.log("Unread Message: " + info.unreadMessages)
		callback();
	});
}

/*
var doScreepsAuthMe = function(callback) {
	xhrScreepsRequest(getPrefix() + "auth/me", 'GET', function(res) {
		info.email = res.email;
		info.username = res.username;
		info.maxCpu = res.cpu;
		info.gclProgress = res.gcl;
		info.credits = res.credits;
		info.money = res.money;
		info.subscriptionTokens = res.subscriptionTokens;
		console.log("AuthMe: " + info.email);
		callback();
	});
}
*/

var decodeBase64 = function(s) {
    var e={},i,b=0,c,x,l=0,a,r='',w=String.fromCharCode,L=s.length;
    var A="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for(i=0;i<64;i++){e[A.charAt(i)]=i;}
    for(x=0;x<L;x++){
        c=e[s.charAt(x)];b=(b<<6)+c;l+=6;
        while(l>=8){((a=(b>>>(l-=8))&0xff)||(x<(L-2)))&&(r+=w(a));}
    }
    return r;
};

function GColor(base) {
  var c = parseInt(base.replace(/^#/, ''), 16);
  if ( isNaN(c) ) return null;
  return c;
}

var doScreepsMemory = function(callback) {
	xhrScreepsRequest(getPrefix() + "user/memory?path=pebble", 'GET', function(res) {
		if ( !res.ok ) {
			callback(true);
			return;
		}
    
    try {
  		var unpacked = decodeBase64(res.data.substring(3));
  		var charData = unpacked.split('').map(function(x) { return x.charCodeAt(0); });			
  		var binData = new Uint8Array(charData);
  		var data        = pako.inflate(binData);
  		var strData     = String.fromCharCode.apply(null, new Uint16Array(data));
  		
  		var mem = JSON.parse(strData);
  		console.log("Memory block downloaded and decoded from Screeps API");
      // console.log(JSON.stringify(mem, null, 2));
    } catch(e) {
      console.log("Error occured downloading Memory block from Screeps API");
      return callback(true);
    }       
    
    if ( !mem[0] && !mem[1] && !mem[2] && !mem[3] ) return callback(true);
    
    var r0 = mem[0] || {};
    var r1 = mem[1] || {};
    var r2 = mem[2] || {};
    var r3 = mem[3] || {};
    info.vibrate = mem['vibrate'] || 0;
        
    info.text = [r0.text || "", r1.text || "", r2.text || "", r3.text || ""];
    info.progress = [r0.progress || 0, r1.progress || 0, r2.progress || 0, r3.progress || 0];
    info.textColor = [r0.textColor || "#000000", r1.textColor || "#000000", r2.textColor || "#000000", r3.textColor || "#000000"];
    info.textSecondColor = [r0.textSecondColor || "#000000", r1.textSecondColor || "#000000", r2.textSecondColor || "#000000", r3.textSecondColor || "#000000"];
    info.overColor = [r0.bgColor || "#000000", r1.bgColor || "#000000", r2.bgColor || "#000000", r3.bgColor || "#000000"];
    info.underColor = [r0.bgSecondColor || "#000000", r1.bgSecondColor || "#000000", r2.bgSecondColor || "#000000", r3.bgSecondColor || "#000000"];
    info.blink = [r0.blink || false, r1.blink || false, r2.blink || false, r3.blink || false];
    info.bold = [r0.bold || false, r1.bold || false, r2.bold || false, r3.bold || false];
    
    for ( var i = 0; i < 4; i++ ) {
      if ( info.progress[i] !== null ) info.progress[i] = Math.round(info.progress[i]);
      if ( info.textColor[i] ) info.textColor[i] = GColor(info.textColor[i]);
      if ( info.textSecondColor[i] ) info.textSecondColor[i] = GColor(info.textSecondColor[i]);
      if ( info.overColor[i] ) info.overColor[i] = GColor(info.overColor[i]);
      if ( info.underColor[i] ) info.underColor[i] = GColor(info.underColor[i]);
    }
    
		callback();
	});
}

function dispatchScreepsInfo() {  
  var dict = {};
  if ( lastInfo.unreadMessages != info.unreadMessages ) {
    defaultLastInfo(true);
    dict[keys["SCREEPS_MAIL"]] = info.unreadMessages;
  }

  // Dispatch rows of data.
  for ( var i = 0; i < 4; i++ ) {
    if ( lastInfo.text[i] != info.text[i] ) dict[keys["SCREEPS_TEXT"] + i] = info.text[i];
    if ( lastInfo.progress[i] != info.progress[i] ) dict[keys["SCREEPS_PROGRESS"] + i] = info.progress[i];
    if ( lastInfo.textColor[i] != info.textColor[i] ) dict[keys["SCREEPS_TEXT_COLOR"] + i] = info.textColor[i];
    if ( lastInfo.textSecondColor[i] != info.textSecondColor[i] ) dict[keys["SCREEPS_TEXT2_COLOR"] + i] = info.textSecondColor[i];
    if ( lastInfo.underColor[i] != info.underColor[i] ) dict[keys["SCREEPS_UNDER_COLOR"] + i] = info.underColor[i];
    if ( lastInfo.overColor[i] != info.overColor[i] ) dict[keys["SCREEPS_OVER_COLOR"] + i] = info.overColor[i];
    if ( lastInfo.blink[i] != info.blink[i] ) dict[keys["SCREEPS_BLINK"] + i] = info.blink[i];
    if ( lastInfo.bold[i] != info.bold[i] ) dict[keys["SCREEPS_BOLD"] + i] = info.bold[i];
  }
  if ( lastInfo.vibrate != info.vibrate ) dict[keys["SCREEPS_VIBRATE"]] = info.vibrate;
  
  lastInfo = info;
  localStorage.setItem('last_info', JSON.stringify(lastInfo));
  
  // console.log(JSON.stringify(lastInfo, null, 2));
  // console.log(JSON.stringify(info, null, 2));
  // console.log(JSON.stringify(dict, null, 2));
  
  if ( Object.keys(dict).length > 0 ) {
    console.log("Sending screeps information to watch...");
    Pebble.sendAppMessage( dict, function() { 
      console.log("dispatchScreepsInfo: Sent to watch successfully.");
    }, function() {
      console.log("dispatchScreepsInfo: Failed to send to watch.");
    });
  } else {
    console.log("No data changed, nothing to send to watch...");
  }
}

function getScreepsData() {
  var email = localStorage.getItem('email');
  var password = localStorage.getItem('password');
  
  if ( !email || !password ) {
    var dict = {};
    dict[keys["ALERT_MISSING_CONFIG"]] = 1;
    Pebble.sendAppMessage( dict, function() { 
      console.log("getScreepsData: Sent alarm to watch successfully.");
    }, function() {
      console.log("getScreepsData: Failed to send alarm to watch.");
    });    
    return console.log("Unable to getScreepsData(), missing email and/or password.");
  }
  
  info = {};
  doScreepsLogin(function(success) {
    info.loginSuccess = success;
    if ( success ) {
      doScreepsUnreadMessage(function() {
        doScreepsMemory(function(error) {
          if ( !error ) dispatchScreepsInfo();
          if ( error ) sendMissingMemoryNotice();                             
        });
      });
    } else {
      // Authentication failure, send notice to the watch.      
      sendBadScreepsLogin();
    }
  })
}

function sendBadScreepsLogin() {  
  console.log("*** Sending Bad Screeps Login");
  var cBlack = GColor("#000000");
  var cWhite = GColor("#FFFFFF");
  var cRed = GColor("#FF0000");
  
  info = {};
  info.text = ["", "Bad Credentials", "Check Config", "" ];
  info.bold = [true, true, true, true];
  info.textColor = [cBlack, cRed, cRed, cBlack];
  info.underColor = [cBlack, cBlack, cBlack, cBlack];
  info.overColor = [cBlack, cBlack, cBlack, cBlack];
  info.progress = [0, 0, 0, 0];
  info.textSecondColor = [cBlack, cBlack, cBlack, cBlack];
  info.blink = [false, false, false, false];
  
  dispatchScreepsInfo();
}

function sendMissingMemoryNotice() {  
  var cBlack = GColor("#000000");
  var cWhite = GColor("#FFFFFF");
  var cRed = GColor("#FF0000");
  
  info = {};
  info.text = ["Screeps API:", "Unable to find", "'Memory.pebble'", "Please Check Docs" ];
  info.bold = [true, true, true, true];
  info.textColor = [cRed, cRed, cRed, cRed];
  info.underColor = [cBlack, cBlack, cBlack, cBlack];
  info.overColor = [cBlack, cBlack, cBlack, cBlack];
  info.progress = [0, 0, 0, 0];
  info.textSecondColor = [cBlack, cBlack, cBlack, cBlack];
  info.blink = [false, false, false, false];
  
  dispatchScreepsInfo();
}

Pebble.addEventListener('ready', function(e) {
  console.log("Watchface is open!");
  
  getWeather();
  getScreepsData();
})

Pebble.addEventListener('appmessage', function(e) {
  var dict = e.payload;
  console.log("PKit received an appmessage.")
  
  if ( dict["REQUEST_WEATHER"] == 1 ) {
    console.log("Message was a weather request!");
    getWeather();
  } else if ( dict["REQUEST_SCREEPS_API"] == 1 ) {
    console.log("Message sent was a ScreepsAPI request!");
    getScreepsData();
  }
  
  console.log(JSON.stringify(dict, null, 2));
});

Pebble.addEventListener('webviewclosed', function(e) {
  console.log("PKit received webviewclosed");
  if ( e && !e.response ) return;
  
  var dict = clay.getSettings(e.response, false);
  
  console.log(JSON.stringify(dict, null, 2));
  console.log("Storing email in localStorage as '" + dict.CONFIG_EMAIL.value + "'");
  
  localStorage.setItem('server', dict.CONFIG_SERVER.value);
  localStorage.setItem('email', dict.CONFIG_EMAIL.value);
  localStorage.setItem('password', dict.CONFIG_PASSWORD.value);  
  localStorage.setItem('use_weather', dict.CONFIG_USE_WEATHER.value);
  localStorage.setItem('use_fahrenheit', dict.CONFIG_USE_FAHRENHEIT.value);  
  localStorage.setItem('weather_api_key', dict.CONFIG_WEATHER_API_KEY.value);  
  localStorage.setItem('weather_cityid', dict.CONFIG_WEATHER_CITYID.value);  
  
  getWeather();
  getScreepsData();
});

function locationSuccess(pos) {
  var myAPIKey = localStorage.getItem('weather_api_key');
  var myCityID = localStorage.getItem('weather_cityid');
  var useFahrenheit = useFlag('use_fahrenheit');
  
  console.log("Location success, requesting from Openweathermap API...");
  if ( !myAPIKey ) {
    console.log("ERROR: Skipping weather send, no API key.");
    // return Pebble.sendAppMessage({'WEATHER': 'No Weather API Key'});
    return Pebble.sendAppMessage({'WEATHER': 'Screeps.com'});
  }
  
  // Construct URL
  var url = 'http://api.openweathermap.org/data/2.5/weather?id=' + myCityID + '&appid=' + myAPIKey;
  if ( pos ) url = 'http://api.openweathermap.org/data/2.5/weather?lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey;
  
  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', function(responseText) {
      var weatherData = JSON.parse(responseText);
    
      console.log(JSON.stringify(weatherData, null, 2));
      if ( weatherData.cod && weatherData.message ) {
        console.log("Weather problem, sending to watch.");
        var weatherMsg = weatherData.message;
      } else {
        console.log("Weather success, sending to watch.")      
        var c = Math.round(weatherData.main.temp - 273.15);
        var f = Math.round((weatherData.main.temp - 273.15) * 9 / 5 + 32);
        var weatherMsg = weatherData.name + " - " + (weatherData.weather[0].main) + " " + ( useFahrenheit ? f : c) + String.fromCharCode(176);
      }
    
      var dictionary = { 'WEATHER': weatherMsg };
      Pebble.sendAppMessage( dictionary, function() { 
        console.log("locationSuccess: Sent to watch successfully.");
      }, function() {
        console.log("locationSuccess: Failed to send to watch.");
      });
   });
}

function useFlag(flag_name) {
  if ( localStorage.getItem(flag_name) == "False" || localStorage.getItem(flag_name) == "false" ) return false;
  return true;
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  var useWeather = useFlag('use_weather');
  if ( !useWeather ) {
    console.log("PKit is skipping weather, disabled in settings.");
    Pebble.sendAppMessage( { 'WEATHER': 'Screeps.com' }, function() { }, function() { });    
    return;
  }

  console.log("PKit is requesting weather...");
  
  var myCityID = localStorage.getItem('weather_cityid');
  if ( myCityID ) return locationSuccess(null);
  
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

function defaultLastInfo(skipCache) {
  var recoverData = localStorage.getItem('last_info');
  
  if ( recoverData && !skipCache ) {
    try {
      lastInfo = JSON.parse(recoverData);
      console.log("Recovered stored last_info.");
    } catch(e) {
      console.log("Error attempting to recover stored last_info, using default data.");
      defaultLastInfo(true);
    }
  } else {
    console.log("Using default last_info.");
    var cBlack = GColor("#000000");
    var cWhite = GColor("#FFFFFF");
    var cCobaltBlue = GColor("#0055AA");  
    var cRed = GColor("#FF0000");  
    
    lastInfo.unreadMessages = -1;
    lastInfo.vibrate = -1;
    lastInfo.text = ["", "", "", "Connecting Phone"];
    lastInfo.textColor = [null, null, null, null];
    lastInfo.textSecondColor = [null, null, null, null];
    lastInfo.overColor = [null, null, null, null];
    lastInfo.underColor = [null, null, null, null];
    lastInfo.progress = [-1, -1, -1, -1];
    lastInfo.blink = [false, false, false, false];
    lastInfo.bold = [false, false, false, false];
  }
}

// Disable the first-load cache, it's causing problems.
defaultLastInfo(true);