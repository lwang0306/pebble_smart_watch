var resume = "resume";
var pause = "pause";
var celsius = "celsius";
var fahrenheit = "fahrenheit";

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
      } 
    }
});

// function sendToServer(part) {
//   var req = new XMLHttpRequest();
// 	var ipAddress = "158.130.109.148"; // Hard coded IP address
// 	var port = "3001"; // Same port specified as argument to server
// 	var url = "http://" + ipAddress + ":" + port + "/";
// 	var method = "GET";
// 	var async = true;
//   console.log(url + part);
//   Pebble.sendAppMessage({ "0": part});
// }

function sendToServer(part) {

	var req = new XMLHttpRequest();
	var ipAddress = "158.130.110.25"; // Hard coded IP address
	var port = "3002"; // Same port specified as argument to server
	var url = "http://" + ipAddress + ":" + port + "/";
	var method = "GET";
	var async = true;
  
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
                   Pebble.sendAppMessage({ "0": msg });
                } else {
                   Pebble.sendAppMessage({ "0": "Cannot connect to server."});
                }      
	};
  req.open(method, url + part, async);
  req.send(null);
}