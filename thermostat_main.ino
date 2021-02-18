#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Update.h>

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "";
const char* password = "";

#define DHTPIN 27     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22     // DHT 22 (AM2302)

DHT dht(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Default Threshold Temperature Value

char lastTemperature[] = "00.0";
char lastTemperature1[] = "00.0";
char lastHumidity[] = "00.0";
char lastHumidity1[] = "00.0";
char inputTemperatureTh[] = "72.0";

char inputSensor = '1';//1: main sensor; 2: secondary sensor
char inputMode = '0';//0:off; 1:cool; 2:heat
char inputFan = '0';//0: auto 1:on
char timeString[31];
char stringWeather[] = "LOVING FAMILY";
unsigned long timeLastReceive = 0;
unsigned long timeLastTimeUpdate = 0;
unsigned long timeLastTimeString = 0;
unsigned long timeWeather = 0;
unsigned long currentMillis = 0;
//bool secondarySensorPresent = false;
bool displayOn = true;
bool controlEnable = true;
// used for median filter
float temperature1 = 0;
float temperature2 = 0;
float temperature3 = 0;
//used for controlling
float temperature = 0;
float humidity = 0;
float temperatureTh = 72;

float temperatureThMain = 72;
float temperatureThSec = 72;
float temperatureMain = 0;
float temperatureSec = 0;
float humidityMain = 0;
float humiditySec = 0;

byte controlMode = 0; //current mode of the system, 0=off;10=cool, paused; 11=cool, active;20=heat,paused;21=heat,active
byte numZeroReadingMain = 0;
byte numZeroReadingSec = 0;
byte autoSensorSwitched = 0; // code switched sensor? 0=no, 1=switched from main to secendary, 2=from secondary to main
byte autoModeSwitched = 0; // code switched mode? 0=no, 1=cold to off, 2=heat to off

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 3600;

// GPIOs of touch pins
const int touchSensorPin = 13;
const int touchFanPin = 12;
const int touchOffPin = 14;
const int touchPlusPin = 33;
const int touchMinusPin = 32;
const int touchHeatPin = 15;
const int touchCoolPin = 4;

// GPIO where the relays are connected to
const int heatCool = 18;
const int onOff = 19;
const int fan = 23;

// previous touch button values
int touchPlus = 200;
int touchMinus = 200;
int touchFan = 200;
int touchOff = 200;
int touchSensor = 200;
int touchHeat = 200;
int touchCool = 200;
int touchTh = 90;

// current touch values
int touchPlusCurrent = 200;
int touchMinusCurrent = 200;
int touchFanCurrent = 200;
int touchOffCurrent = 200;
int touchSensorCurrent = 200;
int touchHeatCurrent = 200;
int touchCoolCurrent = 200;



// dim display
unsigned long timeDimDisplay = 0;
unsigned long timeUpdateDisplay = 0;

// Interval between sensor readings. Learn more about ESP32 timers: https://RandomNerdTutorials.com/esp32-pir-motion-sensor-interrupts-timers/
unsigned long timeLastSensorRead = 0;     // last time of temperature control
//const long interval = 5000;  // interval for temperature control
unsigned long timeLastControl = 0;
float temperatureDelta = 0.3;

typedef struct struct_message {
  int id;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

struct_message incomingReadings;

/*
// HTML GUI web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Thermostat</title>
  <link rel="icon" href="https://hoffmanrefrigeration.com/wp-content/uploads/2019/06/thermostat-icon.png">
  <style>
     table, th, td {
       border: 3px solid black;
       border-collapse: collapse;
       text-align:center;
       font-size:6vw;
     }
     p, a {
         font-size:6vw;
     }
     label, input {
     font-size:6vw;
     }
     table.center {
       margin-left: auto;
       margin-right: auto;
     }
     input[type=radio] {
         border: 0px;
         width: 0.8em;
         height: 0.8em;
     }
  </style>
</head>
<body>
  <p style="text-align:center;">LOVING FAMILY</p>
  <p style="color:red;text-align:center;">&#9829 &#9829 &#9829</p>
  <table class="center">
    <tr>
      <td colspan="4" id="time">Wed 02/10/21 10:12:13</td>
    </tr>
    <tr>
      <td style="background-color:%COLOR1%" id="t1">00.0</td>
      <td colspan='2' rowspan='2' style="font-size:10vw" id="t_set">00.0</td>
      <td id="h1">00.0</td>
    </tr>
    <tr>
      <td style="background-color:%COLOR2%" id="t2">00.0</td>
      <td id="h2">00.0</td>
    </tr>
    <tr>
      <td id="fan">FAN</td>
      <td colspan='2'>&#9829 &#10014 &#9829</td>
      <td id="mode">MODE</td>
    </tr>
  </table>
  <form action="/get">
    <p>Temperature Set:</p><input name="threshold_input" step="0.1" type="number"><br>
    <p>Fan:</p><input id="on" name="fan" type="radio" value='1'> <label for="on">On</label><br>
    <input id="auto" name="fan" type="radio" value='0'> <label for="auto">Auto</label><br>
    <p>Sensor:</p><input id="main" name="sensor" type="radio" value='1'> <label for="main">Main</label><br>
    <input id="secondary" name="sensor" type="radio" value='2'> <label for="secondary">Secondary</label><br>
    <p>Mode:</p><input id="off" name="mode" type="radio" value='0'> <label for="off">Off</label><br>
    <input id="cool" name="mode" type="radio" value='1'> <label for="cool">Cool</label><br>
    <input id="heat" name="mode" type="radio" value='2'> <label for="heat">Heat</label><br>
    <input type="submit" value="Submit">
  </form><br>
  <p style="text-align:center">
    <a href="/">Refresh</a> 
  </p>
  <script>
    if (!!window.EventSource) {
     var source = new EventSource('/events');
     
     source.addEventListener('open', function(e) {
      console.log("Events Connected");
     }, false);
     source.addEventListener('error', function(e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
      }
     }, false);
     
     source.addEventListener('message', function(e) {
      console.log("message", e.data);
     }, false);
     
     source.addEventListener('time', function(e) {
      console.log("time", e.data);
      document.getElementById("time").innerHTML = e.data;
     }, false);
     
     source.addEventListener('sensor', function(e) {
      console.log("sensor", e.data);
      if(e.data == '1'){
        document.getElementById("t1").style.background = "#6FD81F";
      document.getElementById("t2").style.background = "#FFFFFF";
      }
      if(e.data == '2'){
        document.getElementById("t2").style.background = "#6FD81F";
        document.getElementById("t1").style.background = "#FFFFFF";
      }
     }, false);
     
     source.addEventListener('t1', function(e) {
      console.log("t1", e.data);
      document.getElementById("t1").innerHTML = e.data;
     }, false);
     
     source.addEventListener('t2', function(e) {
      console.log("t2", e.data);
      document.getElementById("t2").innerHTML = e.data;
     }, false);
     source.addEventListener('h1', function(e) {
      console.log("h1", e.data);
      document.getElementById("h1").innerHTML = e.data;
     }, false);
     source.addEventListener('h2', function(e) {
      console.log("h2", e.data);
      document.getElementById("h2").innerHTML = e.data;
     }, false);
     source.addEventListener('t_set', function(e) {
     console.log("t_set", e.data);
     document.getElementById("t_set").innerHTML = e.data;
     }, false);
     source.addEventListener('fan', function(e) {
      console.log("fan", e.data);
      document.getElementById("fan").innerHTML = e.data;
     }, false);
     source.addEventListener('mode', function(e) {
      console.log("mode", e.data);
      document.getElementById("mode").innerHTML = e.data;
     }, false);
     
    }
</script>
</body>
</html>)rawliteral";

// redirect to homepage 
const char redirect_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <meta http-equiv="refresh" content="0; URL=/" />
</head>
  <body>
  </body>
</html>)rawliteral";
*/

// mordern websocket gui page
const char websocket_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>   
    <title>Thermostat</title>
    <link rel="icon" href="https://hoffmanrefrigeration.com/wp-content/uploads/2019/06/thermostat-icon.png">
  <style> 
    body {
      background-color: #CCCCCC;
    }
    #thermostat, #thermostat1 {
      width: 65vmin;
      height: 65vmin;
      margin: 0 auto;
      margin-bottom: 1em;
      -webkit-tap-highlight-color: rgba(0, 0, 0, 0);
    }
    .dial {
      -webkit-user-select: none;
       -moz-user-select: none;
        -ms-user-select: none;
          user-select: none;
    }
  
    .dial.away .dial__lbl--target {
      visibility: hidden;
    }
    .dial.away .dial__lbl--target--half {
      visibility: hidden;
    }
    .dial.away .dial__lbl--away {
      opacity: 1;
    }
    .dial .dial__shape {
      transition: fill 0.5s;
    }

    .dial__editableIndicator {
      fill: white;
      fill-rule: evenodd;
      opacity: 0;
      transition: opacity 0.5s;
    }
    .dial--edit .dial__editableIndicator {
      opacity: 1;
    }
    .dial--state--off .dial__shape {
      fill: #222;
    }
    .dial--state--heating .dial__shape {
      fill: #E36304;
    }
    .dial--state--cooling .dial__shape {
      fill: #007AF1;
    }
    .dial__ticks path {
      fill: rgba(255, 255, 255, 0.3);
    }
    .dial__ticks path.active {
      fill: rgba(255, 255, 255, 0.8);
    }
    .dial text {
      fill: white;
      text-anchor: middle;
      font-family: Helvetica, sans-serif;
      alignment-baseline: central;
    }
    .dial__lbl--target {
      font-size: 120px;
      font-weight: bold;
    }
    .dial__lbl--target--half {
      font-size: 40px;
      font-weight: bold;
      opacity: 0;
      transition: opacity 0.1s;
    }
    .dial__lbl--target--half.shown {
      opacity: 1;
      transition: opacity 0s;
    }
    .dial__lbl--ambient {
      font-size: 22px;
      font-weight: bold;
    }
    .dial__lbl--hum {
      font-size: 22px;
      font-weight: bold;
    }
    .dial__lbl--away {
      font-size: 72px;
      font-weight: bold;
      opacity: 0;
      pointer-events: none;
    }
    @font-face {
      font-family: 'Open Sans';
      font-style: normal;
      font-weight: 300;
      src: url(https://fonts.gstatic.com/s/opensans/v18/mem5YaGs126MiZpBA-UN_r8OUuhs.ttf) format('truetype');
    }
    .btn-group button {
      font-size: 3em;
      background-color: #4CAF50; /* Green background */
      border: 1px solid green; /* Green border */
      color: white; /* White text */
      padding: 10px 24px; /* Some padding */
      margin-bottom: 0.5em;
      cursor: pointer; /* Pointer/hand icon */
      float: left; /* Float the buttons side by side */
    }

    /* Clear floats (clearfix hack) */
    .btn-group:after {
      content: "";
      clear: both;
      display: table;
    }

    .btn-group button:not(:last-child) {
      border-right: none; /* Prevent double borders */
    }

    /* Add a background color on hover */
    .btn-group button:hover {
      background-color: #3e8e41;
    }
  </style>
</head>
<body>
<div id='container'>
  <div id='thermostat'></div>
  <div id='thermostat1'></div>
  <div class="btn-group" style="width:100%">
      <button id="btn_cool" style="width:33.3%">&#10052 &#10052</button>
      <button id="btn_off" style="width:33.3%">&#10060</button>
      <button id="btn_heat" style="width:33.3%">&#128293 &#128293</button>
  </div>
  <div class="btn-group" style="width:100%">
      <button id="btn_sensor_main" style="width:50%">&#127777 1</button>
      <button id="btn_sensor_sec" style="width:50%">&#127777 2</button>
  </div>
  <div class="btn-group" style="width:100%">
      <button id="btn_fan_on" style="width:50%">&#128168 &#128168</button>
      <button id="btn_fan_auto" style="width:50%">&#128168 A</button>
  </div>
</div>

<script>
  var thermostatDial = (function() {
     // Utility functions
    // Create an element with proper SVG namespace, optionally setting its attributes and appending it to another element
    function createSVGElement(tag, attributes, appendTo) {
        var element = document.createElementNS('http://www.w3.org/2000/svg', tag);
        attr(element, attributes);
        if (appendTo) {
            appendTo.appendChild(element);
        }
        return element;
    }

    // Set attributes for an element
    function attr(element, attrs) {
        for (var i in attrs) {
            element.setAttribute(i, attrs[i]);
        }
    }

    // Rotate a cartesian point about given origin by X degrees
    function rotatePoint(point, angle, origin) {
        var radians = angle * Math.PI / 180;
        var x = point[0] - origin[0];
        var y = point[1] - origin[1];
        var x1 = x * Math.cos(radians) - y * Math.sin(radians) + origin[0];
        var y1 = x * Math.sin(radians) + y * Math.cos(radians) + origin[1];
        return [x1, y1];
    }

    // Rotate an array of cartesian points about a given origin by X degrees
    function rotatePoints(points, angle, origin) {
        return points.map(function(point) {
            return rotatePoint(point, angle, origin);
        });
    }

    // Given an array of points, return an SVG path string representing the shape they define
    function pointsToPath(points) {
        return points.map(function(point, iPoint) {
            return (iPoint > 0 ? 'L' : 'M') + point[0] + ' ' + point[1];
        }).join(' ') + 'Z';
    }

    function circleToPath(cx, cy, r) {
        return [
            "M", cx, ",", cy,
            "m", 0 - r, ",", 0,
            "a", r, ",", r, 0, 1, ",", 0, r * 2, ",", 0,
            "a", r, ",", r, 0, 1, ",", 0, 0 - r * 2, ",", 0,
            "z"
        ].join(' ').replace(/\s,\s/g, ",");
    }

    function donutPath(cx, cy, rOuter, rInner) {
        return circleToPath(cx, cy, rOuter) + " " + circleToPath(cx, cy, rInner);
    }

    // Restrict a number to a min + max range
    function restrictToRange(val, min, max) {
        if (val < min) return min;
        if (val > max) return max;
        return val;
    }
    // Round a number 
    function roundHalf(num) {
        return Math.round(num * 10) / 10;
    }

    function setClass(el, className, state) {
        el.classList[state ? 'add' : 'remove'](className);
    }
     // The "MEAT"
    return function(targetElement, options) {
        var self = this;
         // Options
        options = options || {};
        options = {
            diameter: options.diameter || 400,
            minValue: options.minValue || 65, // Minimum value for target temperature
            maxValue: options.maxValue || 80, // Maximum value for target temperature
            numTicks: options.numTicks || 150, // Number of tick lines to display around the dial
            onSetTargetTemperature: options.onSetTargetTemperature || function() {}, // Function called when new target temperature set by the dial
        };
         // Properties - calculated from options in many cases
        var properties = {
            tickDegrees: 300, 
            rangeValue: options.maxValue - options.minValue,
            radius: options.diameter / 2,
            ticksOuterRadius: options.diameter / 30,
            ticksInnerRadius: options.diameter / 8,
            hvac_states: ['off', 'heating', 'cooling'],
            dragLockAxisDistance: 15,
        }
        properties.lblAmbientPosition = [properties.radius, properties.ticksOuterRadius - (properties.ticksOuterRadius - properties.ticksInnerRadius) / 2]
        properties.offsetDegrees = 180 - (360 - properties.tickDegrees) / 2;
         // Object state
        var state = {
            target_temperature: 72,
            ambient_temperature: 70,
			ambient_humidity: 30,
            hvac_state: properties.hvac_states[0],
            //has_leaf: false,
            away: false
        };
         // Property getter / setters
        Object.defineProperty(this, 'target_temperature', {
            get: function() {
                return state.target_temperature;
            },
            set: function(val) {
                state.target_temperature = restrictTargetTemperature(+val);
                render();
            }
        });
        Object.defineProperty(this, 'ambient_temperature', {
            get: function() {
                return state.ambient_temperature;
            },
            set: function(val) {
                state.ambient_temperature = roundHalf(+val);
                render();
            }
        });
		Object.defineProperty(this, 'ambient_humidity', {
            get: function() {
                return state.ambient_humidity;
            },
            set: function(val) {
                state.ambient_humidity = val;
                render();
            }
        });
        Object.defineProperty(this, 'hvac_state', {
            get: function() {
                return state.hvac_state;
            },
            set: function(val) {
                if (properties.hvac_states.indexOf(val) >= 0) {
                    state.hvac_state = val;
                    render();
                }
            }
        });
     
        Object.defineProperty(this, 'away', {
            get: function() {
                return state.away;
            },
            set: function(val) {
                state.away = !!val;
                render();
            }
        });
         // SVG
        var svg = createSVGElement('svg', {
            width: '100%', 
            height: '100%', 
            viewBox: '0 0 ' + options.diameter + ' ' + options.diameter,
            class: 'dial'
        }, targetElement);
        // CIRCULAR DIAL
        var circle = createSVGElement('circle', {
            cx: properties.radius,
            cy: properties.radius,
            r: properties.radius,
            class: 'dial__shape'
        }, svg);
        // EDITABLE INDICATOR
        var editCircle = createSVGElement('path', {
            d: donutPath(properties.radius, properties.radius, properties.radius - 4, properties.radius - 8),
            class: 'dial__editableIndicator',
        }, svg);
         // Ticks
        var ticks = createSVGElement('g', {
            class: 'dial__ticks'
        }, svg);
        var tickPoints = [
            [properties.radius - 1, properties.ticksOuterRadius],
            [properties.radius + 1, properties.ticksOuterRadius],
            [properties.radius + 1, properties.ticksInnerRadius],
            [properties.radius - 1, properties.ticksInnerRadius]
        ];
        var tickPointsLarge = [
            [properties.radius - 1.5, properties.ticksOuterRadius],
            [properties.radius + 1.5, properties.ticksOuterRadius],
            [properties.radius + 1.5, properties.ticksInnerRadius + 20],
            [properties.radius - 1.5, properties.ticksInnerRadius + 20]
        ];
        var theta = properties.tickDegrees / options.numTicks;
        var tickArray = [];
        for (var iTick = 0; iTick < options.numTicks; iTick++) {
            tickArray.push(createSVGElement('path', {
                d: pointsToPath(tickPoints)
            }, ticks));
        };
         // Labels
        var lblTarget = createSVGElement('text', {
            x: properties.radius,
            y: properties.radius,
            class: 'dial__lbl dial__lbl--target'
        }, svg);
        var lblTarget_text = document.createTextNode('');
        lblTarget.appendChild(lblTarget_text);
        var lblTargetHalf = createSVGElement('text', {
            x: properties.radius + properties.radius / 2.5,
            y: properties.radius - properties.radius / 8,
            class: 'dial__lbl dial__lbl--target--half'
        }, svg);
        var lblTargetHalf_text = document.createTextNode('');
        lblTargetHalf.appendChild(lblTargetHalf_text);
        // humidity
        var lblHum = createSVGElement('text',{          
          x: properties.radius,
          y: properties.radius*1.8,
		  class: 'dial__lbl--hum'
        },svg);
        var lblHum_text = document.createTextNode('');
        lblHum.appendChild(lblHum_text);

        var lblAmbient = createSVGElement('text', {
            class: 'dial__lbl--ambient'
        }, svg);
        var lblAmbient_text = document.createTextNode('');
        lblAmbient.appendChild(lblAmbient_text);
        //
        var lblAway = createSVGElement('text', {
            x: properties.radius,
            y: properties.radius,
            class: 'dial__lbl dial__lbl--away'
        }, svg);
        var lblAway_text = document.createTextNode('AWAY');
        lblAway.appendChild(lblAway_text);
         // RENDER
        function render() {
            renderAway();
            renderHvacState();
            renderTicks();
            renderTargetTemperature();
            renderAmbientTemperature();
            renderHumidity();
            //renderLeaf();
        }
        render();
         // RENDER - ticks
        function renderTicks() {
            var vMin, vMax;
            if (self.away) {
                vMin = self.ambient_temperature;
                vMax = vMin;
            } else {
                vMin = Math.min(self.ambient_temperature, self.target_temperature);
                vMax = Math.max(self.ambient_temperature, self.target_temperature);
            }
            var min = restrictToRange(Math.round((vMin - options.minValue) / properties.rangeValue * options.numTicks), 0, options.numTicks - 1);
            var max = restrictToRange(Math.round((vMax - options.minValue) / properties.rangeValue * options.numTicks), 0, options.numTicks - 1);
            tickArray.forEach(function(tick, iTick) {
                var isLarge = iTick == min || iTick == max;
                var isActive = iTick >= min && iTick <= max;
                attr(tick, {
                    d: pointsToPath(rotatePoints(isLarge ? tickPointsLarge : tickPoints, iTick * theta - properties.offsetDegrees, [properties.radius, properties.radius])),
                    class: isActive ? 'active' : ''
                });
            });
        }
         // RENDER - ambient temperature
        function renderAmbientTemperature() {
            lblAmbient_text.nodeValue = Math.round(self.ambient_temperature * 10) / 10;
            var peggedValue = restrictToRange(self.ambient_temperature, options.minValue, options.maxValue);
            degs = properties.tickDegrees * (peggedValue - options.minValue) / properties.rangeValue - properties.offsetDegrees;
            if (peggedValue > self.target_temperature) {
                degs += 8;
            } else {
                degs -= 8;
            }
            var pos = rotatePoint(properties.lblAmbientPosition, degs, [properties.radius, properties.radius]);
            attr(lblAmbient, {
                x: pos[0],
                y: pos[1]
            });
        }
         //RENDER - ambient humidity
        function renderHumidity() {
            lblHum_text.nodeValue = Math.round(self.ambient_humidity * 10) / 10+'%';
        }
         // RENDER - target temperature
        function renderTargetTemperature() {
            lblTarget_text.nodeValue = Math.floor(self.target_temperature);
            lblTargetHalf_text.nodeValue = Math.round((self.target_temperature - Math.floor(self.target_temperature)) * 10);
            setClass(lblTargetHalf, 'shown', true); /*self.target_temperature%1!=0*/
        }
         // RENDER - HVAC state
        function renderHvacState() {
            Array.prototype.slice.call(svg.classList).forEach(function(c) {
                if (c.match(/^dial--state--/)) {
                    svg.classList.remove(c);
                };
            });
            svg.classList.add('dial--state--' + self.hvac_state);
        }
		//RENDER - away
        function renderAway() {
            svg.classList[self.away ? 'add' : 'remove']('away');
        }
         //Drag to control
        var _drag = {
            inProgress: false,
            startPoint: null,
            startTemperature: 0,
            lockAxis: undefined
        };

        function eventPosition(ev) {
            if (ev.targetTouches && ev.targetTouches.length) {
                return [ev.targetTouches[0].clientX, ev.targetTouches[0].clientY];
            } else {
                return [ev.x, ev.y];
            };
        }

        var startDelay;

        function dragStart(ev) {
            startDelay = setTimeout(function() {
                setClass(svg, 'dial--edit', true);
                _drag.inProgress = true;
                _drag.startPoint = eventPosition(ev);
                _drag.startTemperature = self.target_temperature || options.minValue;
                _drag.lockAxis = undefined;
            }, 100);
        };

        function dragEnd(ev) {
            clearTimeout(startDelay);
            setClass(svg, 'dial--edit', false);
            if (!_drag.inProgress) return;
            _drag.inProgress = false;
            if (self.target_temperature != _drag.startTemperature) {
                if (typeof options.onSetTargetTemperature == 'function') {
                    options.onSetTargetTemperature(self.target_temperature);
                };
            };
        };

        function dragMove(ev) {
            ev.preventDefault();
            if (!_drag.inProgress) return;
            var evPos = eventPosition(ev);
            var dy = _drag.startPoint[1] - evPos[1];
            var dx = evPos[0] - _drag.startPoint[0];
            var dxy;
            if (_drag.lockAxis == 'x') {
                dxy = dx;
            } else if (_drag.lockAxis == 'y') {
                dxy = dy;
            } else if (Math.abs(dy) > properties.dragLockAxisDistance) {
                _drag.lockAxis = 'y';
                dxy = dy;
            } else if (Math.abs(dx) > properties.dragLockAxisDistance) {
                _drag.lockAxis = 'x';
                dxy = dx;
            } else {
                dxy = (Math.abs(dy) > Math.abs(dx)) ? dy : dx;
            };
            var dValue = (dxy * getSizeRatio()) / (options.diameter) * properties.rangeValue / 5;
            self.target_temperature = roundHalf(_drag.startTemperature + dValue);
        }

        svg.addEventListener('mousedown', dragStart);
        svg.addEventListener('touchstart', dragStart);

        svg.addEventListener('mouseup', dragEnd);
        svg.addEventListener('mouseleave', dragEnd);
        svg.addEventListener('touchend', dragEnd);

        svg.addEventListener('mousemove', dragMove);
        svg.addEventListener('touchmove', dragMove);
        //Helper functions
        function restrictTargetTemperature(t) {
            return restrictToRange(roundHalf(t), options.minValue, options.maxValue);
        }
      /*  function angle(point) {
            var dx = point[0] - properties.radius;
            var dy = point[1] - properties.radius;
            var theta = Math.atan(dx / dy) / (Math.PI / 180);
			console.log(dy);
            if (point[0] >= properties.radius && point[1] < properties.radius) {
                theta = 90 - theta - 90;
            } else if (point[0] >= properties.radius && point[1] >= properties.radius) {
                theta = 90 - theta + 90;
            } else if (point[0] < properties.radius && point[1] >= properties.radius) {
                theta = 90 - theta + 90;
            } else if (point[0] < properties.radius && point[1] < properties.radius) {
                theta = 90 - theta + 270;
            }
            return theta;
        };*/
        function getSizeRatio() {
            return options.diameter / targetElement.clientWidth;
        }
    };
})();

/* ==== */
initButton();
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
initWebSocket();

var nest = new thermostatDial(document.getElementById('thermostat'), {
    onSetTargetTemperature: function(v) {
    websocket.send(nest.target_temperature.toFixed(1)+'G');
  console.log('button event '+nest.target_temperature.toFixed(1)+'G');
    }
  
});

var nest1 = new thermostatDial(document.getElementById('thermostat1'), {
    onSetTargetTemperature: function(v) {
    websocket.send(nest1.target_temperature.toFixed(1)+'g');
  console.log('button event '+nest1.target_temperature.toFixed(1)+'g');
    }
});
// websocket stuff
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }

  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 5000);
  }
  function initButton() {
    document.getElementById('btn_cool').addEventListener('click', tg_cool);
  document.getElementById('btn_off').addEventListener('click', tg_off);
  document.getElementById('btn_heat').addEventListener('click', tg_heat);
  document.getElementById('btn_sensor_main').addEventListener('click', tg_sensor_main);
  document.getElementById('btn_sensor_sec').addEventListener('click', tg_sensor_sec);
  document.getElementById('btn_fan_auto').addEventListener('click', tg_fan_auto);
  document.getElementById('btn_fan_on').addEventListener('click', tg_fan_on);
  }
  function tg_cool(){
    websocket.send('c');
  console.log('button event');
  }
  function tg_off(){
    websocket.send('o');
  console.log('button event');
  }
  function tg_heat(){
    websocket.send('h');
  console.log('button event');
  }
  function tg_sensor_main(){
    websocket.send('S');
  console.log('button event');
  }
  function tg_sensor_sec(){
    websocket.send('s');
  console.log('button event');
  }
  function tg_fan_auto(){
    websocket.send('f');
  console.log('button event');
  }
  function tg_fan_on(){
    websocket.send('F');
  console.log('button event');
  }

  function onMessage(event) { 
    //var state;
  var msg;
  var fan;
  var sensor;
  var mode;//0:off,1:cool,2:heat
  var t1;
  var t2;
  var th1;
  var th2;
  var h1;
  var h2;
  msg=event.data;
  console.log(msg);
  fan=msg[0];
  sensor=msg[1];
  mode=msg[2];
  t1=Number(msg.substring(3,7));
  t2=Number(msg.substring(7,11));
  if(msg.substring(11,15)!=="AA.A"){
    th1=Number(msg.substring(11,15));
    th2=Number(msg.substring(15,19));
    nest.target_temperature = th1;
    nest1.target_temperature = th2;
  } 
  
  h1=Number(msg.substring(19,23));
  h2=Number(msg.substring(23,27));
  if(fan=='0'){
    document.getElementById('btn_fan_auto').style.backgroundColor = "#214f1f";
    document.getElementById('btn_fan_on').style.backgroundColor = "#4CAF50";
  }
    if(fan=='1'){
    document.getElementById('btn_fan_on').style.backgroundColor = "#214f1f";
    document.getElementById('btn_fan_auto').style.backgroundColor = "#4CAF50";
  }
  if(sensor=='1'){
    document.getElementById('btn_sensor_main').style.backgroundColor = "#214f1f";
    document.getElementById('btn_sensor_sec').style.backgroundColor = "#4CAF50";
    nest1.hvac_state = "off";
    switch(mode){
      case '0':
        nest.hvac_state = "off";
        break;
      case '1':
        nest.hvac_state = "cooling";
        break;
      case '2':
        nest.hvac_state = "heating";
        break;
    }    
  }
  if(sensor=='2'){
    nest.hvac_state = "off";
    document.getElementById('btn_sensor_sec').style.backgroundColor = "#214f1f";
    document.getElementById('btn_sensor_main').style.backgroundColor = "#4CAF50";
    switch(mode){
      case '0':
        nest1.hvac_state = "off";
        break;
      case '1':
        nest1.hvac_state = "cooling";
        break;
      case '2':
        nest1.hvac_state = "heating";
        break;
    }    
  }
  switch(mode){
    case '0':
      document.getElementById('btn_off').style.backgroundColor = "#214f1f";
      document.getElementById('btn_heat').style.backgroundColor = "#4CAF50";
      document.getElementById('btn_cool').style.backgroundColor = "#4CAF50";
      break;
    case '1':
      document.getElementById('btn_cool').style.backgroundColor = "#214f1f";
      document.getElementById('btn_heat').style.backgroundColor = "#4CAF50";
      document.getElementById('btn_off').style.backgroundColor = "#4CAF50";
      break;
    case '2':
      document.getElementById('btn_heat').style.backgroundColor = "#214f1f";
      document.getElementById('btn_off').style.backgroundColor = "#4CAF50";
      document.getElementById('btn_cool').style.backgroundColor = "#4CAF50";
      break;
  }
  nest.ambient_temperature = t1;
  nest1.ambient_temperature = t2;
  nest.ambient_humidity = h1;
  nest1.ambient_humidity = h2;
  }
</script>    
</body>
</html>)rawliteral";

// OTA update page
const char* otaUpdate =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form>"
  "<div id='prg'>progress: 0%</div>"
  "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Page does not exist");
}

AsyncWebServer server(80);
// Create an Event Source on /events
//AsyncEventSource events("/events");
AsyncWebSocket ws("/ws");
//String msg;

void notifyClients(char* msg) {
  ws.textAll(msg);
}

//void StringWidth4(char* output, float value){
  //dtostrf(value,4,1,output);
//}

void buildMsg(char* msg, bool sendAllParameters){
	
	msg[0]=inputFan;	
	msg[1]=inputSensor;
	msg[2]=inputMode; 
  msg[3]='\0';
  char string4[]="00.0";
  if(sendAllParameters){
    dtostrf(temperatureMain,4,1,string4);
    strcat(msg, string4);
    dtostrf(temperatureSec,4,1,string4);
    strcat(msg, string4);
    dtostrf(temperatureThMain,4,1,string4);
    strcat(msg, string4);
    dtostrf(temperatureThSec,4,1,string4);
    strcat(msg, string4);
    dtostrf(humidityMain,4,1,string4);
    strcat(msg, string4);
    dtostrf(humiditySec,4,1,string4);
    strcat(msg, string4);
    }
  else if(~sendAllParameters){
    dtostrf(temperatureMain,4,1,string4);
    strcat(msg, string4);
    dtostrf(temperatureSec,4,1,string4);
    strcat(msg, string4);
    strcat(msg, "AA.AAA.A");
    dtostrf(humidityMain,4,1,string4);
    strcat(msg, string4);
    dtostrf(humiditySec,4,1,string4);
    strcat(msg, string4);
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    controlEnable = false;
    // Serial.printf((char*)data);
    if (strcmp((char*)data, "c") == 0) {
      inputMode = '1';      
    }
	else if (strcmp((char*)data, "h") == 0) {
      inputMode = '2';      
    }
	else if (strcmp((char*)data, "o") == 0) {
      inputMode = '0';      
    }
	else if (strcmp((char*)data, "f") == 0) {
      inputFan = '0';      
    }
	else if (strcmp((char*)data, "F") == 0) {
      inputFan = '1';      
    }
	else if (strcmp((char*)data, "S") == 0) {
		inputSensor = '1';  
		temperatureTh = temperatureThMain;
		dtostrf(temperatureTh,4,1,inputTemperatureTh);	  
    }
	else if (strcmp((char*)data, "s") == 0) {
    inputSensor = '2';  
		temperatureTh = temperatureThSec;
		dtostrf(temperatureTh,4,1,inputTemperatureTh);
    }
	else if (strncmp((char*)(data+4), "g", 1) == 0) {
	//	Serial.printf("00000");
		inputSensor = '2';
  //  Serial.printf("11111");
		temperatureThSec = (float)atof((char*)data);
   // Serial.printf("22222");
		temperatureTh = temperatureThSec;
  // Serial.printf("33333");
		dtostrf(temperatureTh,4,1,inputTemperatureTh);
  //  Serial.printf("44444");
    }
	else if (strncmp((char*)(data+4), "G", 1) == 0) {
		
		inputSensor = '1';
		temperatureThMain = (float)atof((char*)data);
		temperatureTh = temperatureThMain;
		dtostrf(temperatureTh,4,1,inputTemperatureTh);;
    }
  else{
     // Serial.printf("unknown message");
    }
  }
  char msg[28];
  buildMsg(msg,true);
  notifyClients(msg);
  controlEnable = true;
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  
  switch (type) {
    case WS_EVT_CONNECT:
     // Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      
      char msg[28];
      buildMsg(msg,true);
      notifyClients(msg);
      break;
    case WS_EVT_DISCONNECT:
      //Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}



float readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  //float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(t)) {
    // Serial.println("Failed to read from DHT sensor!");
    return 0;
  }
  else {
    // Serial.println(t);
    return t-2;
  }
}

float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  if (isnan(h)) {
    // Serial.println("Failed to read from DHT sensor!");
    return 0;
  }
  else {
    // Serial.println(h);
    return h;
  }
}

// callback function that will be executed when data from sensor is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  // Copies the sender mac address to a string
  char macStr[18];
  // Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // Serial.println(macStr);
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  timeLastReceive = millis();
  //board["id"] = incomingReadings.id;
  //board["temperature"] = incomingReadings.temp;
  //board["humidity"] = incomingReadings.hum;
  //board["readingId"] = String(incomingReadings.readingId);
  //String jsonString = JSON.stringify(board);
  //events.send(jsonString.c_str(), "new_readings", millis());
  if (incomingReadings.temp < 0.001) {
    if (numZeroReadingSec < 6) {
      numZeroReadingSec++;
    } else {
      temperatureSec = 0;
      humiditySec = 0;
      strcpy(lastTemperature1,"00.0");
      strcpy(lastHumidity1,"00.0");
    }

  } else {
    numZeroReadingSec = 0;
    temperatureSec = incomingReadings.temp-2; // looks like dht22 reading is 2 F higher than real temperature
    humiditySec = incomingReadings.hum;
    dtostrf(temperatureSec,4,1, lastTemperature1);
    dtostrf(humiditySec, 4, 1, lastHumidity1);
  }



  // Serial.printf("Board ID %u: %u bytes\n", incomingReadings.id, len);
  //Serial.printf("remote t: %4.2f \n", incomingReadings.temp);
  // Serial.printf("h value: %4.2f \n", incomingReadings.hum);
  // Serial.printf("readingID value: %d \n", incomingReadings.readingId);
  // Serial.println();
}

// Replaces placeholder with DHT22 values
/*String processor(const String& var) {
  //// Serial.println(var);
  if (var == "T1") {
    return lastTemperature;
  }
  if (var == "T2") {
    return lastTemperature1;
  }
  if (var == "H1") {
    return lastHumidity;
  }
  if (var == "H2") {
    return lastHumidity1;
  }
  else if (var == "T_SET") {
    if (inputSensor == "MAIN") {
      return String(temperatureThMain, 1);
    } else if (inputSensor == "SECONDARY") {
      return String(temperatureThSec, 1);
    } else {
      return "";
    }
  }

  //  else if(var == "SENSOR"){
  //    return inputSensor;
  //  }
  else if (var == "MODE") {
    return inputMode;
  }
  else if (var == "FAN") {
    return inputFan;
  }
  else if (var == "TIME") {
    return timeString;
  }
  //  else if(var == "WEATHER"){
  //  return stringWeather;
  //}
  else if (var == "COLOR1") {
    if (inputSensor == "MAIN") {
      return "#6FD81F";
    }
    else {
      return "#FFFFFF";
    }
  }
  else if (var == "COLOR2") {
    if (inputSensor == "SECONDARY") {
      return "#6FD81F";
    }
    else {
      return "#FFFFFF";
    }
  }
  return String();
}*/


void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    // Serial.println("Setting as a Wi-Fi Station..");
  }
  // Serial.print("Station IP Address: ");
  // Serial.println(WiFi.localIP());
  // Serial.print("Wi-Fi Channel: ");
  // Serial.println(WiFi.channel());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    // Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  pinMode(onOff, OUTPUT);
  digitalWrite(onOff, HIGH);
  pinMode(heatCool, OUTPUT);
  digitalWrite(heatCool, HIGH);
  pinMode(fan, OUTPUT);
  digitalWrite(fan, HIGH);

  // Start the DHT22 sensor
  dht.begin();

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // Serial.println("Failed to obtain time");
  }
  //char timeString[22];
  strftime(timeString, 22, "%a %m/%d/%y %I:%M:%S", &timeinfo);
  //timeString = String(time);

  // Send GUI page to client
 /* server.on("/legacy", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
    timeDimDisplay = millis();
    display.dim(false);
    displayOn = true;
  });
*/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", websocket_html);
    timeDimDisplay = millis();
    display.dim(false);
    displayOn = true;
  });

  // Receive an HTTP GET request at <ESP_IP>/get?threshold_input=<inputMessage>&enable_arm_input=<inputMessage2>
  /*server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    if (request->hasParam("sensor")) {
      inputSensor = request->getParam("sensor")->value();
    }
    if (request->hasParam("threshold_input")) {
      char inputThTemp[10];
      strcpy(inputThTemp, request->getParam("threshold_input")->value());
      if (strlen(inputThTemp)>0) {
        if (inputSensor == '1') {
          temperatureThMain = atof(inputThTemp);
          //   temperatureTh = temperatureThMain;

        } else if (inputSensor == '2') {
          temperatureThSec = atof(inputThTemp);
          //  temperatureTh = temperatureThSec;
        }
      }
    }
    if (request->hasParam("mode")) {
      inputMode = request->getParam("mode")->value();
    }
    if (request->hasParam("fan")) {
      inputFan = request->getParam("fan")->value();
    }

    if (inputSensor == '1') {
      temperature = temperatureMain;
      temperatureTh = temperatureThMain;
      dtostrf (temperatureThMain,4, 1,inputTemperatureTh);
    } else if (inputSensor == "2") {
      temperature = temperatureSec;
      temperatureTh = temperatureThSec;
      dtostrf(temperatureThSec,4,1,inputTemperatureTh);
    }

    // Serial.println(inputTemperatureTh);
    // Serial.println(inputSensor);
    // Serial.println(inputMode);
    request->send_P(200, "text/html", redirect_html);
    timeDimDisplay = millis();
    display.dim(false);
    displayOn = true;
  });*/
  
  // ota update page
  server.on("/otaUpdate", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", otaUpdate);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, [&](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    //Upload handler chunks in data
    if (!index) {
#if defined(ESP8266)
      int cmd = (filename == "filesystem") ? U_FS : U_FLASH;
      Update.runAsync(true);
      size_t fsSize = ((size_t) &_FS_end - (size_t) &_FS_start);
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin((cmd == U_FS) ? fsSize : maxSketchSpace, cmd)) { // Start with max available size
#elif defined(ESP32)
      int cmd = (filename == "filesystem") ? U_SPIFFS : U_FLASH;
      if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) { // Start with max available size
#endif
        Update.printError(Serial);
        return request->send(400, "text/plain", "OTA could not begin");
      }
    }

    // Write chunked data to the free sketch space
    if (len) {
      if (Update.write(data, len) != len) {
        return request->send(400, "text/plain", "OTA could not begin");
      }
    }

    if (final) { // if the final flag is set then this is the last frame of data
      if (!Update.end(true)) { //true to set the size to the current progress
        Update.printError(Serial);
        return request->send(400, "text/plain", "Could not end OTA");
      }
    } else {
      return;
    }
  });
 /* events.onConnect([](AsyncEventSourceClient * client) {
    char buf[5];
    if (client->lastId()) {
     // Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    //client->send("hello!", NULL, millis(), 10000);
    client->send(dtostrf(temperatureTh,4,1,buf),"t_set",millis());
    client->send(dtostrf(temperatureMain,4, 1,buf), "t1", millis());
    client->send(dtostrf(humidityMain, 4,1,buf), "h1", millis());
    client->send(dtostrf(temperatureSec, 4,1,buf), "t2", millis());
    client->send(dtostrf(humiditySec, 4,1,buf), "h2", millis());
    client->send(inputFan,"fan",millis());
    client->send(inputMode, "mode", millis());
    client->send(timeString, "time", millis());
    client->send(inputSensor, "sensor", millis());
    
  });*/
 // server.addHandler(&events);
  server.onNotFound(notFound);
  initWebSocket();
  server.begin();
  //start display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    // Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  delay(2000);
}

void loop() {

  currentMillis = millis();

  
  // display update
  if (displayOn && (currentMillis - timeUpdateDisplay > 300)) {
    timeUpdateDisplay = currentMillis;
    display.clearDisplay();
    // draw the lines
    display.drawRect(0, 0, 128, 64, WHITE); // outer boarder
    display.drawRect(0, 0, 128, 16, WHITE); // top banner
    if(inputSensor == '1'){
      display.fillRect(0, 16, 32, 16, WHITE); // temperature 1
      display.drawRect(0, 32, 32, 16, WHITE); // temperature 2
    }
    else if(inputSensor == '2'){
      display.drawRect(0, 16, 32, 16, WHITE); // temperature 1
      display.fillRect(0, 32, 32, 16, WHITE); // temperature 2
    }    
    display.drawRect(0, 16, 32, 16, WHITE); // temperature 1
    display.drawRect(0, 32, 32, 16, WHITE); // temperature 2
    display.drawRect(0, 48, 32, 16, WHITE); // FAN status
    display.drawRect(32, 48, 64, 16, WHITE); // love symbol
    display.drawRect(32, 16, 64, 32, WHITE); // temperature set point
    display.drawRect(96, 16, 32, 16, WHITE); // humidity 1
    display.drawRect(96, 32, 32, 16, WHITE); // humidity 2
    display.drawRect(96, 48, 32, 16, WHITE); // heat cool mode
    // write the text
    display.setTextSize (1);
    display.setTextColor(WHITE);
    display.setCursor(1, 4); //top banner
    display.println(timeString);
    //display.setCursor(2,52); // weather
    //display.println(inputFan);
    display.setCursor(100, 20); // humidity 1
    display.println(lastHumidity);
    display.setCursor(100, 36); // humidity 2
    display.println(lastHumidity1);
    display.setCursor(100, 52); // mode
    if(inputMode=='0'){
      display.println("OFF");
    }
    else if(inputMode=='1'){
      display.println("COOL");
    }
    if(inputMode=='2'){
      display.println("HEAT");
    }
    display.setCursor(4, 52);
    if(inputFan=='0'){
      display.println("AUTO");
    }
    else if(inputFan=='1'){
      display.println("ON");
    }
    display.setCursor(55, 52);
    //uint8 arr[] = { 3, 0134, 3 };
    display.write(3);
    display.setCursor(65, 52);
    display.println("+");
    display.setCursor(75, 52);
    display.write(3);
    //temperature 1 and 2
    if (inputSensor == '1') {
      display.setTextColor(BLACK, WHITE);
      display.setCursor (4, 20);
      display.println(lastTemperature);
      display.setTextColor(WHITE);
      display.setCursor (4, 36);
      display.println(lastTemperature1);
    }
    if (inputSensor == '2') {
      display.setCursor (4, 20);
      display.println(lastTemperature);
      display.setTextColor(BLACK, WHITE);
      display.setCursor (4, 36);
      display.println(lastTemperature1);
      display.setTextColor(WHITE);
    }
    display.setTextSize(2);//temperature setpoint
    display.setCursor(39, 25);
    char buf[5];
    display.println(inputTemperatureTh);
    display.display();
  }

  // end display update

  if (currentMillis - timeDimDisplay > 60000) {
    display.dim(true); //this keeps the display always on
    displayOn = false;
  }

  // touchPins
  touchPlusCurrent = touchRead(touchPlusPin);
  touchMinusCurrent = touchRead(touchMinusPin);
  touchFanCurrent = touchRead(touchFanPin);
  touchOffCurrent = touchRead(touchOffPin);
  touchSensorCurrent = touchRead(touchSensorPin);
  touchHeatCurrent = touchRead(touchHeatPin);
  touchCoolCurrent = touchRead(touchCoolPin);
  //// Serial.println(touchPlusCurrent);
  //// Serial.println(touchMinusCurrent);
  //// Serial.println(touchFanCurrent);
  //// Serial.println(touchModeCurrent);
  //// Serial.println(touchSensorCurrent);
  //delay(1000);
  if (touchPlusCurrent < touchTh && touchPlus > touchTh) {
    // display.dim(false);
    delay(20);
    touchPlusCurrent = touchRead(touchPlusPin);
    if (touchPlusCurrent < touchTh) {
      timeDimDisplay = currentMillis;
      if (displayOn) {
        touchPlus = touchPlusCurrent;
        temperatureTh += 0.2;
        dtostrf(temperatureTh,4, 1,inputTemperatureTh);
      }
      else {
        display.dim(false);
        displayOn = true;
      }
    }

  }
  if (touchPlusCurrent > touchTh && touchPlus < touchTh) {
    delay(20);
    touchPlusCurrent = touchRead(touchPlusPin);
    if (touchPlusCurrent > touchTh) {
      touchPlus = touchPlusCurrent;
    }
  }
  if (touchMinusCurrent < touchTh && touchMinus > touchTh) {
    // display.dim(false);
    delay(20);
    touchMinusCurrent = touchRead(touchMinusPin);
    if (touchMinusCurrent < touchTh) {
      timeDimDisplay = currentMillis;
      if (displayOn) {
        touchMinus = touchMinusCurrent;
        temperatureTh -= 0.2;
        dtostrf(temperatureTh, 4,1,inputTemperatureTh);
      }
      else {
        display.dim(false);
        displayOn = true;
      }
    }
  }
  if (touchMinusCurrent > touchTh && touchMinus < touchTh) {
    delay(20);
    touchMinusCurrent = touchRead(touchMinusPin);
    if (touchMinusCurrent > touchTh) {
      touchMinus = touchMinusCurrent;
    }
  }
  if (touchFanCurrent < touchTh && touchFan > touchTh) {
    delay(20);
    touchFanCurrent = touchRead(touchFanPin);
    if (touchFanCurrent < touchTh) {
      timeDimDisplay = currentMillis;
      if (displayOn) {
        touchFan = touchFanCurrent;
        if (inputFan == '1') {
          inputFan = '0';
        }
        else if (inputFan == '0') {
          inputFan = '1';
        }
      }
      else {
        display.dim(false);
        displayOn = true;
      }
    }
  }
  if (touchFanCurrent > touchTh && touchFan < touchTh) {
    delay(20);
    touchFanCurrent = touchRead(touchFanPin);
    if (touchFanCurrent > touchTh) {
      touchFan = touchFanCurrent;
    }
  }
  if (touchOffCurrent < touchTh && touchOff > touchTh) {
    delay(20);
    touchOffCurrent = touchRead(touchOffPin);
    if (touchOffCurrent < touchTh) {
      timeDimDisplay = currentMillis;
      if (displayOn) {
        touchOff = touchOffCurrent;
        inputMode = '0';
      }
      else {
        display.dim(false);
        displayOn = true;
      }
    }
  }
  if (touchOffCurrent > touchTh && touchOff < touchTh) {
    delay(20);
    touchOffCurrent = touchRead(touchOffPin);
    if (touchOffCurrent > touchTh) {
      touchOff = touchOffCurrent;
    }
  }
  if (touchHeatCurrent < touchTh && touchHeat > touchTh) {
    delay(20);
    touchHeatCurrent = touchRead(touchHeatPin);
    if (touchHeatCurrent < touchTh) {
      timeDimDisplay = currentMillis;
      if (displayOn) {
        touchHeat = touchHeatCurrent;
        inputMode = '2';
      }
      else {
        display.dim(false);
        displayOn = true;
      }
    }
  }
  if (touchHeatCurrent > touchTh && touchHeat < touchTh) {
    delay(20);
    touchHeatCurrent = touchRead(touchHeatPin);
    if (touchHeatCurrent > touchTh) {
      touchHeat = touchHeatCurrent;
    }
  }
  if (touchCoolCurrent < touchTh && touchCool > touchTh) {
    delay(20);
    touchCoolCurrent = touchRead(touchCoolPin);
    if (touchCoolCurrent < touchTh) {
      timeDimDisplay = currentMillis;
      if (displayOn) {
        touchCool = touchCoolCurrent;
        inputMode = '1';
        // Serial.println("User switched to cool mode");
      }
      else {
        display.dim(false);
        displayOn = true;
      }
    }
  }
  if (touchCoolCurrent > touchTh && touchCool < touchTh) {
    delay(20);
    touchCoolCurrent = touchRead(touchCoolPin);
    if (touchCoolCurrent > touchTh) {
      touchCool = touchCoolCurrent;
    }
  }
  if (touchSensorCurrent < touchTh && touchSensor > touchTh) {
    delay(20);
    touchSensorCurrent = touchRead(touchSensorPin);
    if (touchSensorCurrent < touchTh) {
      timeDimDisplay = currentMillis;
      if (displayOn) {
        touchSensor = touchSensorCurrent;
        if (inputSensor == '1') {
          inputSensor = '2';
        }
        else if (inputSensor == '2') {
          inputSensor = '1';
        }
      }
      else {
        display.dim(false);
        displayOn = true;
      }
    }
  }
  if (touchSensorCurrent > touchTh && touchSensor < touchTh) {
    delay(20);
    touchSensorCurrent = touchRead(touchSensorPin);
    if (touchSensorCurrent > touchTh) {
      touchSensor = touchSensorCurrent;
    }
  }


  // update system time every 24 hours
  if (currentMillis - timeLastTimeUpdate > 86400000) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    timeLastTimeUpdate = currentMillis;
  }
  // construct time string every second
  if (currentMillis - timeLastTimeString > 1000) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      // Serial.println("Failed to obtain time");
    }
    //char timeString[22];
    strftime(timeString, 22, "%a %m/%d/%y %I:%M:%S", &timeinfo);
    
  }


  // update system sensor reading
  // reconnect wifi if connection lost
  if (currentMillis - timeLastSensorRead > 5000) {
     //reconnect wifi
     if (WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect();
        WiFi.reconnect();
     }
     // clean websocket clients
     ws.cleanupClients();
     //update sensor readings
	   timeLastSensorRead = currentMillis;
    // Get primary sensor temperature in F, and update values in webpage
    float temperatureCurrent = readDHTTemperature();
    // Serial.print("Read main sensor temperature ");
    // Serial.println(temperatureCurrent);
    temperature1 = temperature2;
    temperature2 = temperature3;
    temperature3 = temperatureCurrent;
    // Serial.print("Filter ");
    // Serial.print(temperature1);
    // Serial.print("-");
    // Serial.print(temperature2);
    // Serial.print("-");
    // Serial.println(temperature3);
    float temperatureMainTemp = temperature3;
    if (temperature1 - temperature2 < 1E-6 && temperature2 - temperature3 < 1E-6) {
      temperatureMainTemp = temperature2;
    }
    if (temperature1 - temperature3 < 1E-6 && temperature3 - temperature2 < 1E-6) {
      temperatureMainTemp = temperature3;
    }
    if (temperature2 - temperature1 < 1E-6 && temperature1 - temperature3 < 1E-6) {
      temperatureMainTemp = temperature1;
    }
    if (temperature2 - temperature3 < 1E-6 && temperature3 - temperature1 < 1E-6) {
      temperatureMainTemp = temperature3;
    }
    if (temperature3 - temperature1 < 1E-6 && temperature1 - temperature2 < 1E-6) {
      temperatureMainTemp = temperature1;
    }
    if (temperature3 - temperature2 < 1E-6 && temperature2 - temperature1 < 1E-6) {
      temperatureMainTemp = temperature2;
    }
    if (temperatureMainTemp < 0.001) {
      if (numZeroReadingMain < 6) {
        numZeroReadingMain++;
      } else {
        temperatureMain = 0;
      }
    } else {
      numZeroReadingMain = 0;
      temperatureMain = temperatureMainTemp;
    }
     dtostrf(temperatureMain,4, 1,lastTemperature);
    // Serial.print(temperatureMain);
    // Serial.println(" F main sensor");
    // Get primary sensor humidity, and update values in webpage
    humidityMain = readDHTHumidity();
    dtostrf(humidityMain,4, 1,lastHumidity);
    // Serial.print(humidityMain);
    // Serial.println("% main sensor");
    // get secondary sensor temp and humidity, and update values in webpage
    if (currentMillis - timeLastReceive > 6000) {
      if (numZeroReadingSec < 6) {
        numZeroReadingSec++;
      } else {
        strcpy(lastTemperature1,"00.0");
        strcpy(lastHumidity1, "00.0");
        temperatureSec = 0;
        humiditySec = 0;
      }
    }
   /* else {
      numZeroReadingSec = 0;
      lastTemperature1 = String(incomingReadings.temp, 1);
      lastHumidity1 = String (incomingReadings.hum, 1);
      // secondarySensorPresent = true;
    } */


    if (inputSensor == '1' && temperatureMain > 0) {
      temperature = temperatureMain;
      humidity = humidityMain;
      //temperatureTh = temperatureThMain;
      // Serial.println("00000 temperatureMain is");
      // Serial.println(temperatureMain);
    } else if (inputSensor == '1' && temperatureMain < 0.001 && temperatureSec > 0.001) {
      temperature = temperatureSec;
      humidity = humiditySec;
      //temperatureTh = temperatureThSec;
      inputSensor = '2';
      autoSensorSwitched = 1;
      // Serial.println("11111");
    } else if (inputSensor == '2' && temperatureSec > 0.001) {
      temperature = temperatureSec;;
      humidity = humiditySec;
      //temperatureTh = temperatureThSec;
      // Serial.println("22222");
    } else if (inputSensor == '2' && temperatureSec < 0.001 && temperatureMain > 0.001) {
      temperature = temperatureMain;
      humidity = humidityMain;
      //temperatureTh = temperatureThMain;
      inputSensor = '1';
      autoSensorSwitched = 2;
      // Serial.println("3333");
    } else if (temperatureSec < 0.001 && temperatureMain < 0.001) {
      temperature = 0;
      humidity = 0;
      if (inputMode == '2') {
        autoModeSwitched = 2;
      } else if (inputMode == '1') {
        autoModeSwitched = 1;
      }
      inputMode = '0';
      //controlMode = 0;
      // Serial.println("4444");
    }

    if (temperatureMain > 0.001) {
      if (autoSensorSwitched == 1) {
        inputSensor = '1';
        autoSensorSwitched = 0;
      }
      if (autoModeSwitched == 1) {
        inputMode = '1';
        autoModeSwitched = 0;
      } else if (autoModeSwitched == 2) {
        inputMode = '2';
        autoModeSwitched = 0;
      }
    }
    if (temperatureSec > 0.001) {
      if (autoSensorSwitched == 2) {
        inputSensor = '2';
        autoSensorSwitched = 0;
      }
      if (autoModeSwitched == 1) {
        inputMode = '1';
        autoModeSwitched = 0;
      } else if (autoModeSwitched == 2) {
        inputMode = '2';
        autoModeSwitched = 0;
      }
    } 

    // events.send("ping",NULL,millis());
   /* events.send(String(temperatureTh,1).c_str(),"t_set",millis());
    events.send(String(temperatureMain, 1).c_str(), "t1", millis());
    events.send(String(humidityMain, 1).c_str(), "h1", millis());
    events.send(String(temperatureSec, 1).c_str(), "t2", millis());
    events.send(String(humiditySec, 1).c_str(), "h2", millis());
    // events.send(inputFan.c_str(),"fan",millis());
    events.send(inputMode.c_str(), "mode", millis());
    events.send(timeString.c_str(), "time", millis());
    events.send(inputSensor.c_str(), "sensor", millis());*/

   /* events.send(dtostrf(temperatureTh,4,1,buf),"t_set",millis());
    events.send(dtostrf(temperatureMain,4, 1,buf), "t1", millis());
    events.send(dtostrf(humidityMain, 4,1,buf), "h1", millis());
    events.send(dtostrf(temperatureSec, 4,1,buf), "t2", millis());
    events.send(dtostrf(humiditySec, 4,1,buf), "h2", millis());
    events.send(inputFan,"fan",millis());
    events.send(inputMode, "mode", millis());
    events.send(timeString, "time", millis());
    events.send(inputSensor, "sensor", millis());*/

    char msg[28];
    buildMsg(msg,false);
    notifyClients(msg);

  }

  // temperature control

  if (currentMillis - timeLastControl > 500 && controlEnable) {
    timeLastControl = currentMillis;
    // Serial.println(temperature);

    if (inputMode == '2') {
      if (temperature < temperatureTh - temperatureDelta && temperature > 0 && controlMode != 21) {
        digitalWrite(onOff, LOW);
        digitalWrite(heatCool, HIGH);
        controlMode = 21;
        // Serial.println("Heating===");
        /* // Serial.println(temperature);
          // Serial.println(temperatureDelta);
          // Serial.println(temperatureTh - temperatureDelta);
          // Serial.println(controlMode);
          // Serial.println("======="); */
      } else if (temperature > temperatureTh && controlMode != 20) {
        digitalWrite(onOff, HIGH);
        controlMode = 20;
        // Serial.println("Heating, paused");
      } else if (temperature <= temperatureTh && temperature >= temperatureTh - temperatureDelta && controlMode != 20 && controlMode != 21) {
        digitalWrite(onOff, HIGH);
        controlMode = 20;
        // Serial.println("Heating, paused");
      } else if (temperature < 0.001 && controlMode != 0) {
        digitalWrite(onOff, HIGH);
        inputMode = '0';
        controlMode = 0;
        // Serial.println("sensor error, system turned off from heat, temperature is");
        // Serial.println(temperature);
      }
    }
    if (inputMode == '1') {
      if (temperature > temperatureTh + temperatureDelta && controlMode != 11) {
        digitalWrite(onOff, LOW);
        digitalWrite(heatCool, LOW);
        controlMode = 11;
        // Serial.println("Cooling");
      } else if (temperature < temperatureTh && temperature > 0 && controlMode != 10) {
        digitalWrite(onOff, HIGH);
        controlMode = 10;
        // Serial.println("Cooling, paused");
      } else if (temperature <= temperatureTh + temperatureDelta && temperature >= temperatureTh && controlMode && controlMode != 10 && controlMode != 11) {
        digitalWrite(onOff, HIGH);
        controlMode = 10;
        // Serial.println("Cooling, paused");
      } else if (temperature < 0.001 && controlMode != 0) {
        digitalWrite(onOff, HIGH);
        controlMode = 0;
        inputMode = '0';
        // Serial.println("sensor error, system turned off from cool, temperatuer is");
        // Serial.println(temperature);
      }
    }
    if (inputMode == '0' && controlMode != 0) {
      digitalWrite(onOff, HIGH);
      controlMode = 0;
      // Serial.println("System off by user");
    }
    // fan and humidity control
    if (inputFan == '0') {
      //// Serial.println("fan set to auto");
      if (humidity > 60 && controlMode != 11 && controlMode != 21) {
        digitalWrite(fan, LOW);
        // Serial.println("fan turned on to lower humidity");
      } else if (humidity < 55) {
        digitalWrite(fan, HIGH);
        // // Serial.println("fan turned off to keep humidity");
      }
    } else if (inputFan == '1') {
      digitalWrite(fan, LOW);
      //// Serial.println("fan set to on");
    }
  }
}
