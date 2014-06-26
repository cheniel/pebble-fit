Pebble.addEventListener("ready",
  function(e) {
    console.log("JavaScript app ready and running!");
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
  	Pebble.openURL("http://www.google.com");
  }
);