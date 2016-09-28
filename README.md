# ScreepsTime
Pebble watchface that displays information from the MMORTS programming game 'Screeps'.

Uses the official API to connect and download the contents of "Memory.pebble", and uses it to drive up to four information rails on the watchface.

[Download now on the Pebble App Store](https://apps.getpebble.com/applications/57ebe9ca8eaf90fc03000090)

## Compatibility
The app store version is currently only built for Basalt (Pebble Time and Time Steel) - It runs on Pebble Time Round, but I had trouble with the emulator and haven't had time to 'clean up' the display to render decently. You're welcome to compile it and deploy to a round Pebble, it just won't look good at the moment.

Incompatible with Aplite due to payload and memory size restrictions. 

# Installing
You can get the latest built version from the Pebble Appstore.

If you wish to compile it yourself, I recommend cloning this project in Cloudpebble.net - It makes the job of compiling/deploying much simplier.

# Using
By itself, this watch displays only the Time and Date. You will need to use the Configuration option in the Pebble App to configure the watchface.

* Configure your Screeps credentials so it can access the memory at Memory.pebble
* If you wish to enable weather, register for a free account at OpenWeatherMap.org and add your API key.

You will need to create a block of memory INSIDE Screeps, to control what gets displayed on your phone. For example:
```
Memory.pebble = {
	vibrate: 0,
	0: { progress: 0, bold: false, blink: false, bgColor: "#666666", bgSecondColor: null, textColor: "#CCCCCC", textSecondColor: null, text: "Rail 1" },
	1: { progress: 0, bold: true, blink: false, bgColor: "#666666", bgSecondColor: null, textColor: "#CCCCCC", textSecondColor: null, text: "Rail 2" },
	2: { progress: 0, bold: true, blink: false, bgColor: "#666666", bgSecondColor: null, textColor: "#CCCCCC", textSecondColor: null, text: "Rail 3" },
	3: { progress: 0, bold: false, blink: false, bgColor: "#666666", bgSecondColor: null, textColor: "#CCCCCC", textSecondColor: null, text: "Rail 4" },
};
```

# Vibrate Option
By setting this value to a number between 1 and 6, your Pebble watch will vibrate when it grabs this payload. If you set it to 0, nothing will occur. The options are as follows:
* 1: Short Pulse (Only once, will not vibrate again until it receives a 'vibrate: 0' payload)
* 2: Long Pulse (Only once, will not vibrate again until it receives a 'vibrate: 0' payload)
* 3: Double Pulse (Only once, will not vibrate again until it receives a 'vibrate: 0' payload)
* 4: Short Pulse (Every time the payload is retrieved, which is every minute)
* 5: Long Pulse (Every time the payload is retrieved, which is every minute)
* 6: Double Pulse (Every time the payload is retrieved, which is every minute)

# Information Rails
Each rail displays information on the watchface. The rails are in order from top to bottom. Rails 0 and 1 are the top-most, rails 2 and 3 are the bottom most. You can configure them a few ways:
```
// Will display a solid white bar, with black text.
{ bgColor: '#FFFFFF', textColor: '#000000', text: 'Sample rail' }

// Same as above, but with bold text.
{ bold: true, bgColor: '#FFFFFF', textColor: '#000000', text: 'Sample rail' }

// Will display a 70% progress bar of light blue over dark blue, with white text.
// The bgColor is the 'progress bar' color, bgSecondaryColor will fill the remainder of the bar. 
{ progress: 70, bgColor: '#0000FF', bgSecondColor: '#000066', textColor: '#FFFFFF', text: "Progress bar' }

// Will display a blinking bar - Colors alternate between primary and secondary every second.
// This bar will blink as black on red, and red on black.
{ blink: true, bgColor: '#000000', bgSecondColor: '#FF0000', textColor: '#FF0000', textSecondColor: '#000000', text: "Blinking warning text" }
```

# Choosing Colors
Please be aware that the Pebble is a little picky about colors as well. Even though they are passed as HTML color codes, there is a limited palette of options. You can find a lot of the valid ones at the following pebble developer site:  
[Pebble Developer - Color Picker](https://developer.pebble.com/guides/tools-and-resources/color-picker/)

# Optional Settings
Additionally, there's two optional settings - New mail and Battery - that you can enable in the application settings within the Pebble App. They both include a 'rail override' - If the criteria is matched, the rail information will be drawn over the specified rail, regardless of what you sent with your payload. These are off by default.

Quick notes:
* New Mail will draw over the Battery indicator, if they're both on the same rail.
* New Mail will only override if you have new unread messages waiting in-game.
* Battery has a threshold - It will only draw if the battery is <= the specified threshold. You can use this to configure it to never show, always show, or simply show when it's starting to get low.

# Reproducing the Sample
For those of you who want a quick demo, or to recreate the view in the app store screenshot, you can use this payload as a test dummy:
```
Memory.pebble = {
	vibrate: 0,
	0: { progress: 0, bold: false, blink: false, bgColor: "#0000FF", bgSecondColor: null, textColor: "#CCCCCC", textSecondColor: null, text: "Screeps AI Running" },
	1: { progress: 30, bold: true, blink: false, bgColor: "#55AA00", bgSecondColor: "#005500", textColor: "#CCCCCC", textSecondColor: null, text: "Job 1: 30%" },
	2: { progress: 60, bold: true, blink: false, bgColor: "#0000FF", bgSecondColor: "#000066", textColor: "#CCCCCC", textSecondColor: null, text: "Avg CPU: 61.3/100" },
	3: { progress: 70, bold: false, blink: false, bgColor: "#0000FF", bgSecondColor: "#000066", textColor: "#CCCCCC", textSecondColor: null, text: "Bucket: 7357" },
};
```

# Questions / Issues
You can submit issues or changes to this repo, or you can contact me on the Screeps slack channel as user 'Camedo'. 
