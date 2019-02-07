/*
  .._____ ______ _____  _____          _            __  _____  ______ ____  _    _  _____
  ./ ____|  ____|  __ \|_   _|   /\   | |          / / |  __ \|  ____|  _ \| |  | |/ ____|
  | (___ | |__  | |__) | | |    /  \  | |         / /  | |  | | |__  | |_) | |  | | |  __
  .\___ \|  __| |  _  /  | |   / /\ \ | |        / /   | |  | |  __| |  _ <| |  | | | |_ |
  .____) | |____| | \ \ _| |_ / ____ \| |____   / /    | |__| | |____| |_) | |__| | |__| |
  |_____/|______|_|  \_\_____/_/    \_\______| /_/     |_____/|______|____/ \____/ \_____|
*/
String inputString = "";            // a String to hold incoming data

//Custom serial event for program control. Stop program till input, similar to cin
String serialEvent() {
  while (!Serial.available()) {
    yieldEvent();
  }
  const bool inputComplete = false; //never goes true
  while (!inputComplete) {
    // get the new byte:
    if (Serial.available()) {
      char inChar = (char)Serial.read();
      if (inChar == '\n') {
        serialSeparator();
        debug("Received via Serial: " + inputString);
        serialSeparator();
        String returnString = inputString;
        inputString = "";

        returnString = serialInterrupt(returnString);                 //modifies the input to allow text commands, enables command to restart

        return returnString;
      } else {
        inputString += inChar;
      }
    }
    yieldEvent();
  }
}

//ONLY CALL THINGS HERE IF YOU ARE OK WITH ABORTING PROCESSES
//DEFINITIONS IN "definitions.h"
//You can use this to modify the serial string.
String serialInterrupt(String checkString) {
  if (checkString == RESTART_STRING) {
    ESP.restart();
  } else if (checkString == String(BACK_STRING) || checkString == String(UP_STRING)) {
    return String(BACK_BUTTON);
  }
  return checkString;
}


//Clean consistent separator
void serialSeparator() {
  Serial.println(F("\n_______________________________\n"));
}


//debug output function, currently integrating post-humously
void debug(String debugString) {
  if (enableDebug) {
    Serial.println("(DEBUG):" + debugString);
  }
}

//String
void console(String consoleOut) {
  Serial.print(consoleOut);
  if (consoleToDebug) {
    debug("Serial Out: " + consoleOut);
  }
}

//String w/ \n
void consoleln(String consoleOut) {
  Serial.println(consoleOut);
  if (consoleToDebug) {
    debug("Serial Line: " + consoleOut);
  }
}

//int w/ \n
void consoleln(int consoleOut) {
  Serial.println(consoleOut);
  if (consoleToDebug) {
    debug("Serial Line: " + consoleOut);
  }
}


//The program has been written to allow anything to act as a button. Add sections for different buttons
void waitForButton() {            //stops program until button is pushed
  WiFiButtonPushed = false;       //Debounce if it got pushed before the fucntion was called
  consoleln(F("Place tag on reader and push FLASH / READ button"));

  bool buttonPushed = false;
  while (!buttonPushed) {  //wait for button to be pushed

        server.handleClient();        //updates client (also useful cause it checks for the wifi button being pushed)

    if (digitalRead(BUTTON)) {
      buttonPushed = true;
    }

    if (WiFiButtonPushed) {
      buttonPushed = true;
    }

    yieldEvent();

  }

  WiFiButtonPushed = false;
}
