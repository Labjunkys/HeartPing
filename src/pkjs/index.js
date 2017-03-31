var WebSocket = require('./reconnecting-websocket');

var inbox = new WebSocket("wss://guarded-hollows-38483.herokuapp.com" + "/receive");
var outbox = new WebSocket("wss://guarded-hollows-38483.herokuapp.com" + "/submit");

console.log("hm!");

inbox.onmessage = function(message) {
  var data = JSON.parse(message.data); 
  console.log("Message from server!");
  for(var key in data) {
    var value = data[key];
    console.log(key+":"+value);
  }
  responseFromServer(data);
};

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

var myAPIKey = 'b5f3670745096b0d1a16981758a43420';

function locationSuccess(pos) {
  // We will request the weather here
  // Construct URL
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
      pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      console.log('Temperature is ' + temperature);

      // Conditions
      var conditions = json.weather[0].main;      
      console.log('Conditions are ' + conditions);
      
      
      // Assemble dictionary using our keys
      var dictionary = {
        'TEMPERATURE': temperature,
        'CONDITIONS': conditions
      };
      
      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }      
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');
    

    // Get the initial weather
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    var dict = e.payload;
    console.log('Got message: ' + JSON.stringify(dict));
    if (dict["1"] === parseInt(dict["1"], 10)) {
      console.log(dict["1"]);
      outbox.send(JSON.stringify({ text: dict[1], handle: Pebble.getAccountToken()}));
      console.log("Sent heartbeat to server!");
    }
  
    if (dict["0"] === 0) {
      getWeather();
    }
  }                     
);

function responseFromServer(data) {
  // Assemble dictionary using our keys
  
  if (data.handle == Pebble.getAccountToken()) {
    return; 
  }
  
  var dictionary = {
    'HEARTRATE': parseInt(data.text)
  };
  
  console.log(dictionary);

  // Send to Pebble
  Pebble.sendAppMessage(dictionary,
                        function(e) {
                          console.log('Heartrate sent to Pebble successfully!');
                        },
                        function(e) {
                          console.log('Error sending heartrate to Pebble!');
                        }
                       );
}









