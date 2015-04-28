var resume = "resume";
var pause = "pause";
var celsius = "celsius";
var fahrenheit = "fahrenheit";
var light = "light";
var timer = "timer";
var statc = "statc";
var statf = "statf";
var polling = "polling";

/* Listener is triggered when buttons is pressed or clicked */
Pebble.addEventListener("appmessage",
  function(e) {
    if (e.payload) {
      if (e.payload.resume) {
        sendToServer(resume);
      } else if (e.payload.pause) {
        sendToServer(pause);
      } else if (e.payload.celsius) {
        sendToServer(celsius);
      } else if (e.payload.fahrenheit) {
        sendToServer(fahrenheit);
      } else if (e.payload.light) {
        sendToServer(light);
      } else if (e.payload.timer) {
        sendToServer(timer);
      } else if (e.payload.statc) {
        sendToServer(statc);
      } else if (e.payload.statf) {
        sendToServer(statf);
      } else if (e.payload.polling) {
        sendToServer(polling);
      }
      }
});

/* send request to server and get reply and send to the phone */
function sendToServer(part) {

	var req = new XMLHttpRequest();
	var ipAddress = "158.130.104.2"; // Hard coded IP address
	var port = "3001"; // Same port specified as argument to server
	var url = "http://" + ipAddress + ":" + port + "/";
	var method = "GET";
	var async = true;
  
  req.onerror = function() {
     Pebble.sendAppMessage( {
      "0": "Connection Failed."
    });
  };
  
	req.onload = function(e) {
                var msg = "no response";
                
                console.log(req.responseText);
                var response = JSON.parse(req.responseText);
                console.log(response);
                if (response) {
                    if (response.name) {
                        msg = response.name;
                    }
                    else msg = "no message has been received!";
                }
                if (req.readyState == 4 && req.status == 200) {
                   console.log(msg);
                   Pebble.sendAppMessage({ "0": msg });
                   
                } else {
                   Pebble.sendAppMessage({ "0": "Cannot connect to server."});
                }      
	};
  req.open(method, url + part, async);
//   req.timeout = 3000;
  req.send(null);
}


