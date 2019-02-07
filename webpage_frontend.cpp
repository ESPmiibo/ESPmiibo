/*
  __          ________ ____  _____        _____ ______     
  \ \        / /  ____|  _ \|  __ \ /\   / ____|  ____|  
  .\ \  /\  / /| |__  | |_) | |__) /  \ | |  __| |__      
  ..\ \/  \/ / |  __| |  _ <|  ___/ /\ \| | |_ |  __|    
  ...\  /\  /  | |____| |_) | |  / ____ \ |__| | |____    
  ....\/  \/   |______|____/|_| /_/    \_\_____|______| 
  This is still heavily WIP. Will be adding more buttons to main.htm in the future.
  Plans  include full web-based text console and more.
*/

void webpageSetup() {
  server.on("/upload", HTTP_GET,
  []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/upload", HTTP_POST, []() {                       // if the client posts to the upload page[]() {
    server.send(200);
  },                          // Send status 200 (OK) to tell the client we are ready to receive
  handleFileUpload                                    // Receive and save the file
           );

  //Custom pages
  server.on("/buttonpushed", HTTP_GET, handleButton);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/openfileserver", HTTP_GET, handleEditRedirect);
  //server.on("/quickClone", HTTP_GET, handleQuickClone);
  server.on("/statsForNerds", HTTP_GET, handleNerdStats);

  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });

  server.begin();
}


void handleButton() {
  Serial.println(F("Button pressed in browser."));
  debug(F("button pushed in browser"));
  WiFiButtonPushed = true;                            //track globally that the button has been pushed
  server.sendHeader("Location", "/main.htm", true);  //Redirect to our html web page
  server.send(302, "text/plain", "");
}

void handleRoot() {
  server.sendHeader("Location", "/main.htm", true);  //Redirect to our html web page
  server.send(302, "text/plain", "");
}

void handleEditRedirect() {
  server.sendHeader("Location", "/edit", true);  //Redirect to our html web page
  server.send(302, "text/plain", "");
}

void handleNerdStats() {
  server.sendHeader("Location", "/graphs.htm", true);  //Redirect to our html web page
  server.send(302, "text/plain", "");
}


/* NYI | Future Button
  void handleQuickClone(){
   mainFunctionCloneAmiibo();
   server.sendHeader("Location", "/main.htm", true);  //Redirect to our html web page
   server.send(302, "text/plain", "");
  }

    <form action="/quickClone" method="get" enctype="multipart/form-data" name="push_button">
    <a href="#"><input class="button" type="submit" value="QUICK CLONE">
    </section>
    </form>
*/
