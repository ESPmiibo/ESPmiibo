
/*This sketch is meant to run on an esp8266.
  It is based on Wifiibo project found here: https://github.com/Xerxes3rd/Wifiibo
  The Wifiibo project is a port of the amiitool project found here: https://github.com/socram8888/amiitool

  Necessary parts:
    ESP8266 (I tested this with and recommend using a nodeMCU)
    MFRC522 or PN532 NFC reader (I only tested this with the MFRC522)

  Recommended parts:
    HC05 or similar Serial -> Bluetooth adapter (This will allow you to control the cloner from your phone or pc via bluetooth)

  If you decide to alter anything, feel free! My only warning is to make sure you use yield() judiciously within loops, the watchdog barks easily.
*/


////////////////////////
////USER DEFINITIONS////
////////////////////////

/* Uncomment for the reader you are using */
//#define USE_PN532 true                        //In the future I would like to do card emulation with this
#define USE_MFRC522 true

//Change this to the rate of your serial console
const int baudrate = 9600;

#define DEBUG_USER_VALUE      true                //True for debug on, false for debug off

//Change these to the username and password for your wifi
const char* ssid =            "Ridley";           //Change this to the name of your 2.4GHz wifi, 5GHz not supported on esp8266
const char* password =        "metroids7";        //Change this to your wifi password

//Change this to what you want the ESPmiibo access point to be called
const char* hostName =        "ESPmiibo";         //Name of the wifi access point

//Username and password for online file manager
const char* http_username =   "admin";            //Not actually used right now
const char* http_password =   "password";         //Gonna be naughty and leave them as default

////////////////////////
/// END  DEFINITIONS ///
////////////////////////

#include <SerialCommand.h>

//Amiibo Handling Libraries
#define FS_NO_GLOBALS //allow spiffs to coexist with SD card, define BEFORE including FS.h, not yet needed but will be fixed
#include <string.h>
#include <Arduino.h>
#include <amiibo.h>
#include <amiitool.h>
#include <drbg.h>
#include <keygen.h>

//Wifi
#include <ESP8266WiFi.h>

//Filesystem and webserver

#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <ESP8266mDNS.h>

//NFC LIBRARIES
#ifdef USE_PN532
#include <Adafruit_PN532Ex.h>   //I'm only using the mfrc522
Adafruit_PN532Ex pn532(D4);
#endif

#ifdef USE_MFRC522
#include <MFRC522Ex.h>
MFRC522Ex mfrc522(D1, D2);
#endif

#define KEYFILENAMESTRING           "/key_retail.bin"
//Key File
const char* keyfilename =           KEYFILENAMESTRING;                                //needed for atool

//Homepage in c array format
#include "main_htm_gz.h"              

//ATOOL INSTANCTIATION
amiitool atool((char*)keyfilename);             //instantiate amitool, passing the path to the encryption key

//GLOBALS
volatile bool triggerReadNFC    = false;            //used to trigger a read, set to true then call handleNFC(), will be reset by function
volatile bool triggerWriteNFC   = false;            //used to trigger a write, set to true then call handleNFC() will be reset by function
volatile bool WiFiButtonPushed  = false;            //used to track if the wifi flash button has been pushed.
volatile bool consoleToDebug    = false;            //used to enable spew of console to debug
volatile bool enableDebug       = DEBUG_USER_VALUE; //used to enable debug spew, change this in user settings to desired value (T/F)
volatile bool backFlag          = false;            //used to track if the 'back' command was used
String inputString = "";                            // a String to hold incoming data

typedef struct cmdStruct
{
  String p;              //path to file to operate on
  String t;              //target file
  int n;                 //Tracking int, also used as a bool for 'list'
};
struct cmdStruct cmd;

//Menuing and Version 
#include "definitions.h"

// SKETCH BEGIN
SerialCommand CMD;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

void setup() {
  Serial.begin(baudrate);
  debug("\n\nSerial started at: " + String(baudrate));
  consoleln("Starting...");

  //Start onboard file system
  if (SPIFFS.begin()) {
    debug(F("Spiffs Running"));
  } else {
    debug(F("Spiffs Disabled"));
  }

#ifdef USE_PN532
  //Check to see if the nfc device is connected
  if (atool.initNFC(&pn532)) {
    debug(F("PN532 Found"));
  } else {
    debug(F("PN532 Not Found"));
  }
#endif

#ifdef USE_MFRC522
  //Check to see if the nfc device is connected
  if (atool.initNFC(&mfrc522)) {
    debug(F("MFRC522 Found"));
  } else {
    debug(F("MFRC522 Not Found"));
  }
#endif

  wifiSetup();
  webpageSetup();
  getFileList("/", false);
  checkRetailKey();
  addCommands();
  consoleln(F("Setup Complete."));

  //Spew program info
  consoleln(PROGRAM_DESCRIPTION);
  console("ESPmiibo Version: " + String(VERSION));
  console("\n\n");

}

void loop() {
  yieldEvent();
}


void yieldEvent() {
  CMD.readSerial();     // Process serial commands
  yield();
}


/*
  void deleteFile(String filename) {
  if (SPIFFS.remove(filename))
  sendStatusCharArray("amiibo deleted.");
  else
  sendStatusCharArray("Failed to delete amiibo.");
  }

  void handleRootRequest(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text / html", index_htm_gz, index_htm_gz_len);
  response->addHeader("Content - Encoding", "gzip");
  request->send(response);
  }


  void handleFaviconRequest(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "image / x - icon", favicon_ico_gz, favicon_ico_gz_len);
  response->addHeader("Content - Encoding", "gzip");
  request->send(response);
  }
*/



/*
  .__  __          _____ _   _    ______ _    _ _   _  _____ _______ _____ ____  _   _  _____
  |  \/  |   /\   |_   _| \ | |  |  ____| |  | | \ | |/ ____|__   __|_   _/ __ \| \ | |/ ____|
  | \  / |  /  \    | | |  \| |  | |__  | |  | |  \| | |       | |    | || |  | |  \| | (___
  | |\/| | / /\ \   | | | . ` |  |  __| | |  | | . ` | |       | |    | || |  | | . ` |\___ \
  | |  | |/ ____ \ _| |_| |\  |  | |    | |__| | |\  | |____   | |   _| || |__| | |\  |____) |
  |_|  |_/_/    \_\_____|_| \_|  |_|     \____/|_| \_|\_____|  |_|  |_____\____/|_| \_|_____/
*/


void mainFunctionDumpAmiibo() {
  waitForButton();
  triggerReadNFC = true;
  handleNFC();
  console(F("\nEncrypted Data: \n"));
  ESP.wdtDisable(); //Dump takes too long, triggers wdt, we have six seconds before the hardware wdt resets here.
  atool.printData(atool.original, NTAG215_SIZE, 4, false, false);
  ESP.wdtEnable(1000); //reenable wdt, the value passed does nothing.
  consoleln(F("Dump Complete\n\n"));
  //Original dump method, used library method, but it can't be dumped directly.
  //dump_amiibo_array(atool.original, NTAG_SIZE);
  /*
    Serial.println(F("\nDecrypted Data: \n"));
    atool.printData(atool.modified, NFC3D_AMIIBO_SIZE, 4, false, false);
  */
}

void mainFunctionSaveAmiibo(String saveName) {
  consoleln("Amiibo will be saved as: " + saveName + ".bin");
  consoleln(F("Scan Amiibo"));
  bool amiiboRead = false;                           //bool to track if the amiibo has been saved successfully
  while (!amiiboRead) {                              //until we save successfully
    waitForButton();
    triggerReadNFC = true;
    if (handleNFC()) {                                  //if card read successfully
      amiiboRead = true;
      debug(F("Card read successfully"));
    } else {
      consoleln(F("Card read failed.\nRetrying...\n"));
    }
    yieldEvent();
  }
  saveAmiibo(saveName + F(".bin"));               //Save amiibo
}

//WILL NEED TO BE FIXED IN THE LONG RUN
//CURRENTLY DOESN'T PRINT TO A TARGET LOCATION, USES OLD DUMP METHODS
//WILL HAVE TO MODIFY OLD METHODS TO WORK WITH YIELD CALLS
void mainFunctionDumpFile(String dumpFile) {
  if (SPIFFS.exists(dumpFile)) {                             //check if file exists
    consoleln("\nDumping file: " + dumpFile);
    fs::File f = SPIFFS.open(dumpFile, "r");                      //open the file
    if (atool.loadFileSPIFFS(&f, false)) {                          //if file successfully loaded
      ESP.wdtDisable(); //Dump takes too long, triggers wdt, we have six seconds before the hardware wdt resets here.
      atool.printData(atool.original, NTAG215_SIZE, 4, false, false);
      ESP.wdtEnable(1000); //reenable wdt, the value passed does nothing.
    }
  } else {
    consoleln(F("File does not exist"));
  }
}

void mainFunctionDeleteFile(String fileToDelete) {
  if (SPIFFS.exists(fileToDelete)) {
    if (SPIFFS.remove(fileToDelete)) {                  //Delete file
      consoleln(F("File deleted successfully"));   //If successful
    } else {
      consoleln(F("File deletion unsuccessful\n"));  //If unsuccessful
    }
  } else {
    consoleln(F("File does not exist\n"));
  }
}

void mainFunctionRenameFile(String fileToRename, String newFileName) {
  console("Renaming " + fileToRename + " to: " + newFileName);
  if (SPIFFS.rename(fileToRename, newFileName)) {
    consoleln(F("Rename successful!"));
  } else {
    consoleln(F("Rename unsuccessful."));
  }

}

void mainFunctionWriteFile(String fileString, int numberOfClones) {
  consoleln("Writing " + fileString + " " + String(numberOfClones) + " time(s)");
  if (SPIFFS.exists(fileString)) {                                  //make sure file exists
    consoleln("\nLoading file: " + fileString);
    fs::File f = SPIFFS.open(fileString, "r");                        //open the file

    if (atool.loadFileSPIFFS(&f, false)) {                          //if file successfully loaded
      mainFunctionWriteHelper(numberOfClones);
      debug("Exiting amiibo writing function.");
      consoleln("Finished writing amiibos");
    } else {
      consoleln(F("Couldn't open file. \n Exiting... \n"));
    }
  } else {
    consoleln(F("File does not exist. \n Exiting... "));
  }
}

void mainFunctionCloneAmiibo(int numberOfClones) {
  consoleln(F("Scan source Amiibo."));
  waitForButton();
  bool readSuccessful = false;
  bool writeSuccessful = false;

  //Read the amiibo, get it into ram
  while (!readSuccessful) {                                        //repeat until the read is successful
    triggerReadNFC = true;                                  //set flag to read
    if (handleNFC()) {
      readSuccessful = true;
      consoleln(F("Amiibo read successful"));
    }
  }

  //pass off to generic mass writer
  mainFunctionWriteHelper(numberOfClones);

  debug("Exiting amiibo cloner function.");
  consoleln("Finished cloning amiibos");
}

//must have amiibo object loaded beforre calling this, whether by file or scan
void mainFunctionWriteHelper(int numberToWrite) {
  debug("Writing " + String(numberToWrite) + " tags");
  int numberOfClonesMadeSoFar = 0;
  while (numberOfClonesMadeSoFar < numberToWrite) {
    consoleln("Remaining tags to write: " + String(numberToWrite - numberOfClonesMadeSoFar));
    consoleln(F("Scan blank tag."));
    bool writeSuccessful = false;
    while (!writeSuccessful) {
      waitForButton();
      triggerWriteNFC = true;
      if (handleNFC()) {
        writeSuccessful = true;
        consoleln(F("Tag written successfully\n\n"));
        numberOfClonesMadeSoFar++;
      } else {
        consoleln(F("Write unsuccessful\nRetrying..."));
      }
    }
    yieldEvent();
  }
}

//Flips debug mode,
bool toggleDebug() {
  enableDebug = !enableDebug;

  if (enableDebug) {
    debug("Debug output enabled.");
    consoleln("Debug output enabled.");
  } else {
    debug("Debug output disabled.");
    consoleln("Debug output disabled.");
  }
}

void mainFunctionRestart() {
  debug("\nRestarting... \n");
  console("\nRestarting...\n");
  ESP.restart();
}


/*.._____ ______ _____  _____          _            __  _____  ______ ____  _    _  _____
  ./ ____|  ____|  __ \|_   _|   /\   | |          / / |  __ \|  ____|  _ \| |  | |/ ____|
  | (___ | |__  | |__) | | |    /  \  | |         / /  | |  | | |__  | |_) | |  | | |  __
  .\___ \|  __| |  _  /  | |   / /\ \ | |        / /   | |  | |  __| |  _ <| |  | | | |_ |
  .____) | |____| | \ \ _| |_ / ____ \| |____   / /    | |__| | |____| |_) | |__| | |__| |
  |_____/|______|_|  \_\_____/_/    \_\______| /_/     |_____/|______|____/ \____/ \_____|


*/

//Output functions
//debug output function, currently integrating post-humously
bool debug(String debugString) {
  if (enableDebug) {
    Serial.println("(DEBUG):" + debugString);
    return true;
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

    if (digitalRead(BUTTON)) {
      buttonPushed = true;
      debug(F("Physical button pushed."));
    }

    if (WiFiButtonPushed) {
      buttonPushed = true;
      debug(F("Web button pushed."));
    }

    yieldEvent();                      //feed watchdog

  }

  WiFiButtonPushed = false;
}



/*
  ..._____ __  __ _____     _______ ____  _   _  _____  ____  _      ______
  ../ ____|  \/  |  __ \   / / ____/ __ \| \ | |/ ____|/ __ \| |    |  ____|
  .| |    | \  / | |  | | / / |   | |  | |  \| | (___ | |  | | |    | |__
  .| |    | |\/| | |  | |/ /| |   | |  | | . ` |\___ \| |  | | |    |  __|
  .| |____| |  | | |__| / / | |___| |__| | |\  |____) | |__| | |____| |____
  ..\_____|_|  |_|_____/_/   \_____\____/|_| \_|_____/ \____/|______|______|
*/

void addCommands() {
  CMD.addCommand(DUMP_AMIIBO, commandDumpa);
  CMD.addCommand(CLONE_AMIIBO, commandClone);
  CMD.addCommand(SAVE_AMIIBO, commandSave);
  CMD.addCommand(DUMP_FILE, commandDumpf);
  CMD.addCommand(DELETE_FILE, commandDelete);
  CMD.addCommand(RENAME_FILE, commandRename);
  CMD.addCommand(WRITE_FILE, commandWritef);
  CMD.addCommand(PRINT_FILE_LIST, commandPrintList);
  CMD.addCommand(TOGGLE_DEBUG, commandDebug);
  CMD.addCommand(RESTART_ESP, commandRestart);
  CMD.addCommand(HELP, commandHelp);
}

void parseCommand() {
  debug(F("parsing"));

  char *arg;
  for (int tokens = MAXTOKENS; tokens > 0; tokens--) {
    arg = CMD.next();   //get token
    if (arg != NULL) {
      debug("Token: " + String(arg));
      if (String(arg) == pToken) {
        cmd.p = String(CMD.next());
        if (cmd.p != NULL) {
          debug("Parsed pToken as " + String(cmd.p));
        } else {
          debug("Empty pToken");
        }

      } else if (String(arg) == tToken) {
        cmd.t = String(CMD.next());

        if (cmd.t != NULL) {
          debug("Parsed tToken as " + String(cmd.t));
        } else {
          debug("Empty tToken");
        }

      } else if (String(arg) == nToken) {
        cmd.n = atoi(CMD.next());

        if (cmd.n != NULL) {
          debug("Parsed nToken as " + String(cmd.n));
        } else {
          debug("Empty nToken");
        }

      } else if (String(arg) == aToken) {
        String tempString = String(CMD.next());
        if (tempString != NULL) {
          if (tempString == TRUESTRING) {
            debug("Parsed aToken as true");
            cmd.n = true;
          } else if (tempString == FALSESTRING) {
            debug("Parsed aToken as false");
            cmd.n = false;
          } else {
            debug("Failed to parse aToken, defaulting false");
            cmd.n = false;
          }
        }
      }
    }
  }
}

void clearStruct() {
  cmd.p = "";
  cmd.t = "";
  cmd.n = 0;
}

void commandDumpa() {
  mainFunctionDumpAmiibo();
  clearStruct();
}
void commandClone() {
  parseCommand();
  mainFunctionCloneAmiibo(cmd.n);                     //clone n amiibos
  clearStruct();
}
void commandSave() {
  parseCommand();
  mainFunctionSaveAmiibo(cmd.p);                      //save scan to p
  clearStruct();
}
void commandDumpf() {
  parseCommand();
  mainFunctionDumpFile(cmd.p);                        //dump file at p
  clearStruct();
}
void commandDelete() {
  parseCommand();
  mainFunctionDeleteFile(cmd.p);                      //delete file at p
  clearStruct();
}
void commandRename() {
  parseCommand();
  mainFunctionRenameFile(cmd.p, cmd.t);     //rename file p to t
  clearStruct();
}
void commandWritef() {
  parseCommand();
  mainFunctionWriteFile(cmd.p, cmd.n);
  clearStruct();
}
void commandPrintList() {
  parseCommand();
  getFileList(cmd.p, cmd.n);
  clearStruct();
}
void commandDebug() {
  parseCommand();
  toggleDebug();
  clearStruct();
}
void commandRestart() {
  parseCommand();
  restartFunction();
  clearStruct();
}
void commandHelp() {
  debug("Printing help menu");
  parseCommand();

  if (cmd.p) {
    int commandLoc = 0;
    for (int i = 0; i < FUNCNUM; i++) {
      if (cmd.p == commandList[i]) {
        commandLoc = i + 1;                           //set it to i+1, will return 0 if nothing is found
      }
    }

    String tabString = "   ";
    if (commandLoc) {                                   //if a valid (non-zero) topic was found
      commandLoc -= 1;                                  //decrement it by one now that error checking is done
      consoleln(commandList[commandLoc]);               //print command
      //consoleln(tabString + funcStrings[commandLoc]); //print friendly name
      consoleln(tabString + funcDesc[commandLoc]);
      consoleln(tabString + "Usage: " + funcUsage[commandLoc] + "\n");

    } else {
      consoleln("Try help -p (command) for more info on a function)");
      consoleln("Commands:");

      for (int i = 0; i < FUNCNUM; i++) {
        consoleln(tabString + commandList[i]);
      }
      consoleln("\n");

    }
    clearStruct();
  }
}

// This gets set as the default handler, and gets called when no other command matches.
void unrecognized(const char *command) {
  debug("Unrecognized command");
}


/*
  ...................._ _ _             _    _                 _ _ _
  ..../\             (_|_) |           | |  | |               | | (_)
  .../  \   _ __ ___  _ _| |__   ___   | |__| | __ _ _ __   __| | |_ _ __   __ _
  ../ /\ \ | '_ ` _ \| | | '_ \ / _ \  |  __  |/ _` | '_ \ / _` | | | '_ \ / _` |
  ./ ____ \| | | | | | | | |_) | (_) | | |  | | (_| | | | | (_| | | | | | | (_| |
  /_/    \_\_| |_| |_|_|_|_.__/ \___/  |_|  |_|\__,_|_| |_|\__,_|_|_|_| |_|\__, |
  Various amiibo and nfc related functions..................................__/ |
  Editing these can damage cards if you are not careful....................|___/
    `                                    '
     ''                                 ''`
    '''''                             ''''''
   ''''''';         ,+++;`          :'''''''
   ;''''''       .@@@@@@@@@#         '''''';
     ,''',      #@@@@@@@@@@@@,       .''':
       `;      #@@@@@@@@@@@@@@;       '`
              @@@@@@@@+#@@@@@@@@
             '@@@@@@.    ;@@@@@@`
  ;;;,.`      @@@@@@.      #@@@@@#       `.,:::
  ;;;;;;:     @@@@@@        @@@@@@      ,::::::
  ;;;;;;:     @@@@@@        @@@@@@      ,::::::
  .`          @@@@@@#       @@@@@@           `.
              :@@@@@@@,  ,. @@@@@@
               .@@@@@@@@@@, @@@@@@
       .::      ;@@@@@@@@@, @@@@@@     ;;.
     ,:::::      .@@@@@@@@, @@@@@@    ;;;;;:
    :::::::.       :@@@@@+  ######   `;;;;;;;
    :::::::                           ;;;;;;:
     :::::                             :;;;;
     ,::.                               .;;:
      :                                   ;
*/

//Intentionally broken, not important for future goals. Futures plans: edit library to not requre these functions, useful but not here
void sendStatusCharArray(char *statusmsg) {}
void updateProgress(int percent) {}
void sendFunctionStatusCode(char *funcName, int code) {}
void sendTagInfo() {}

bool handleNFC() {                    //Heavily edited, removed a lot of unwanted functionality
  if (triggerReadNFC) {
    triggerReadNFC = false;
    console(F("Reading NTAG"));
    bool result = atool.readTag(sendStatusCharArray, updateProgress);

    if (result)
    {
      return true;
      debug("Tag read successful");
    } else {
      consoleln(F("Tag read failed."));
      return false;
    }
  }
  else if (triggerWriteNFC) {
    triggerWriteNFC = false;
    console(F("Writing NTAG"));
    bool result = atool.writeTag(sendStatusCharArray, updateProgress);
    if (result)
    {
      consoleln(F("Tag written."));
      return true;
    } else {
      return false;
    }
  }

  triggerReadNFC = false;
  triggerWriteNFC = false;
}

bool getAmiiboInfo(String filename) {
  int loadResult = 0;
  fs::File f = SPIFFS.open(filename, "r");

  if (f)
  {
    loadResult = atool.loadFileSPIFFS(&f, true);
    f.close();
  }

  if (loadResult >= 0)
  {
    sendTagInfo();
  }

  return loadResult >= 0;
}


bool saveAmiibo(String filename) {      //altered to return true if successful, heavily altered error reporting to be console friendly
  int bytesOut = 0;

  if (filename.length() > SPIFFS_OBJ_NAME_LEN - 1) {
    console(F("Filename too long."));
  }
  else {
    if (SPIFFS.exists(filename)) {
      //code 2
      console(F("File exists already."));
      return false;
    }
    else {
      fs::File f = SPIFFS.open(filename, "w");
      if (!f) {
        //code 3
        console(F("Failed to open file for writing."));
        return false;
      }
      else {
        if ((bytesOut = f.write(atool.original, NTAG215_SIZE)) != NTAG215_SIZE) {                                                     //etc....
          console(F("Failed to write data."));
          return false;
        }
        else {
          if (getAmiiboInfo(filename)) {
            console(F("Amiibo saved!"));
            return true;
          }
          else {
            console(F("Unable to decrypt amiibo."));
            return false;
          }
        }
      }
    }
  }
}


/*__          ________ ____  _____        _____ ______
  \ \        / /  ____|  _ \|  __ \ /\   / ____|  ____|
  .\ \  /\  / /| |__  | |_) | |__) /  \ | |  __| |__
  ..\ \/  \/ / |  __| |  _ <|  ___/ /\ \| | |_ |  __|
  ...\  /\  /  | |____| |_) | |  / ____ \ |__| | |____
  ....\/  \/   |______|____/|_| /_/    \_\_____|______|
  This is still heavily WIP. Will be adding more buttons to main.htm in the future.
  Plans  include full web-based text console and more.
*/

void webpageSetup() {
  MDNS.addService("http", "tcp", 80);
  
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  events.onConnect([](AsyncEventSourceClient * client) {
    client->send("hello!", NULL, millis(), 1000);
  });
  server.addHandler(&events);

  server.addHandler(new SPIFFSEditor(http_username, http_password));

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });


  server.serveStatic("/", SPIFFS, "/");
    
  ////////////////////////
  //Webpage flash button//
  ////////////////////////

  server.on("/buttonpushed",  HTTP_POST,    handleButtonPushed);

  
  //server.on("/edit",          HTTP_POST,    handleFileServer);
  

  server.onNotFound([](AsyncWebServerRequest * request) {
    String errorString = "NOT_FOUND: ";
    if (request->method() == HTTP_GET)
      errorString += "GET";
    else if (request->method() == HTTP_POST)
      errorString += "POST";
    else if (request->method() == HTTP_DELETE)
      errorString += "DELETE";
    else if (request->method() == HTTP_PUT)
      errorString += "PUT";
    else if (request->method() == HTTP_PATCH)
      errorString += "PATCH";
    else if (request->method() == HTTP_HEAD)
      errorString += "HEAD";
    else if (request->method() == HTTP_OPTIONS)
      errorString += "OPTIONS";
    else
      errorString += "UNKNOWN";

    debug(errorString);

    if (request->contentLength()) {
      debug("_CONTENT_TYPE: " + String(request->contentType().c_str()));
      debug("_CONTENT_LENGTH: " + String(request->contentLength()));
    }

    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++) {
      AsyncWebHeader* h = request->getHeader(i);
      debug("HEADER: " + String( h->name().c_str()) + " " + String( h->value().c_str()));
    }

    int params = request->params();
    for (i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isFile()) {
        debug("FILE: " + String( p->name().c_str()) + String(p->value().c_str()) + " " + String( p->size()));
      } else if (p->isPost()) {
        debug("POST: " + String( p->name().c_str()) + String(p->value().c_str()) + " " + String( p->size()));
      } else {
        debug("GET: " + String( p->name().c_str()) + String(p->value().c_str()) + " " + String( p->size()));
      }
    }
    
    debug("Sending 404\n\n");
    request->send(404);
  });
  

  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char*)data);
    if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });
  server.begin();
}

void handleRootRequest(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", main_htm, main_htm_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
  debug("Sent main page");
}

void handleButtonPushed(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", main_htm, main_htm_len);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
  debug(F("Button pushed globally"));
  WiFiButtonPushed = true;                            //track globally that the button has been pushed
}


/*__          ________ ____  _____        _____ ______     ____          _____ _  ________ _   _ _____
  \ \        / /  ____|  _ \|  __ \ /\   / ____|  ____|   |  _ \   /\   / ____| |/ /  ____| \ | |  __ \
  .\ \  /\  / /| |__  | |_) | |__) /  \ | |  __| |__      | |_) | /  \ | |    | ' /| |__  |  \| | |  | |
  ..\ \/  \/ / |  __| |  _ <|  ___/ /\ \| | |_ |  __|     |  _ < / /\ \| |    |  < |  __| | . ` | |  | |
  ...\  /\  /  | |____| |_) | |  / ____ \ |__| | |____    | |_) / ____ \ |____| . \| |____| |\  | |__| |
  ....\/  \/   |______|____/|_| /_/    \_\_____|______|   |____/_/    \_\_____|_|\_\______|_| \_|_____/
*/

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    debug("WS connect" + String(server->url()) + String (client->id()));
    client->printf("Hello Client %u :)" + client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    debug("WS disconnect "+ String(server->url()));
  } else if(type == WS_EVT_ERROR){
    debug("WS error");
  } else if(type == WS_EVT_PONG){
    debug("WS pong " + String(server->url()));
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if(info->opcode == WS_TEXT)
        client->text("I got your text message");
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}





/*__          _______ _____  ______ _      ______  _____ _____
  \ \        / /_   _|  __ \|  ____| |    |  ____|/ ____/ ____|
  .\ \  /\  / /  | | | |__) | |__  | |    | |__  | (___| (___
  ..\ \/  \/ /   | | |  _  /|  __| | |    |  __|  \___ \\___ \
  ...\  /\  /   _| |_| | \ \| |____| |____| |____ ____) |___) |
  ....\/  \/   |_____|_|  \_\______|______|______|_____/_____/
  You can change Wifi.mode() to WIFI_AP for access point only or
  WIFI_STA for local address mode.
*/

void wifiSetup() {
  //Set up Wifi*

  WiFi.mode(WIFI_STA); 
  WiFi.hostname(hostName);

  WiFi.softAP("ESPmiibo");
  WiFi.begin(ssid, password);
  
  debug("Waiting for wifi connection");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    debug("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }


  /*
  if (WiFi.begin(ssid, password)) {
    debug("Connected to " + WiFi.SSID());
    while (WiFi.status() != WL_CONNECTED) {           //waiting dot counter until ip assigned
      delay(1000);
      debug(".");
    }
    debug("IP address: " + WiFi.localIP() + String("\n"));
  } else {
    debug("Wifi failed to start\n");
  }
  */
}



/* __  __ _____  _____  _____
  |  \/  |_   _|/ ____|/ ____|
  | \  / | | | | (___ | |
  | |\/| | | |  \___ \| |
  | |  | |_| |_ ____) | |____ _
  |_|  |_|_____|_____/ \_____(_)
*/

//Dump byte array as hex values to Serial.
void dump_byte_array(uint8_t * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? F("0") : F(""));
    Serial.print(buffer[i], HEX);
  }
}

//dumps hex file friendly array
void dump_amiibo_array(uint8_t *buffer, byte bufferSize) {
  Serial.print(F("\n\n"));
  byte newLine = 0;
  for (int i = 0; i < bufferSize; i++) {
    if (i % 100 == 0) yield();  // Preemptive yield every 100 byte feed the WDT.
    Serial.print(buffer[i] < 0x10 ? F("0") : F(""));
    Serial.print(buffer[i], HEX);
    if (newLine == 3) {
      newLine = 0;
      Serial.print('\n');
    } else {
      newLine++;
    }
  }
}

void getFileList(String folder, int amiibosOnly) {
  debug(F("Printing file list."));

  if (amiibosOnly) {
    consoleln("Printing amiibo list for folder " + folder);
  } else {
    consoleln("Listing files in folder " + String("'") + folder + String("'"));
  }

  byte listSize = 0;                                                                    //create a byte to count the number of files in the directory
  fs::Dir dir = SPIFFS.openDir(folder);
  while (dir.next()) {
    if (amiibosOnly) {
      if (dir.fileName().endsWith(F(".bin")) || dir.fileName().endsWith(F(".BIN"))) {     //check each to see if it is a bin file
        fs::File f = dir.openFile("r");                                                   //open the file
        if (amiitool::isSPIFFSFilePossiblyAmiibo(&f)) {                                   //check if the file is an amiibo
          consoleln(String(dir.fileName()));                                              //print the name of every amiibo file as it is found
          listSize++;                                                                     //increase our list size
        }
        f.close();                                                                        //close it
      }
    } else {
      fs::File f = dir.openFile("r");                                                     //open the file
      consoleln(String(dir.fileName()));                                             //print the name
      f.close();                                                                          //close it
      listSize++;
    }
  }
  consoleln("Counted: " + String(listSize) + " file(s)");
}

//Checks to see if we have a valid decryption key
bool checkRetailKey() {               
  if (!atool.tryLoadKey()) {
    consoleln(F("Invalid Key"));
  }
  else {
    consoleln(F("Found Key"));
  }
}

//Edit this function based on the restart method of your device
void restartFunction() {
  debug(F("Restarting"));
  consoleln(F("Restarting"));
  ESP.restart();
}


/*
  ._____                                _           _
  |  __ \                              | |         | |
  | |  | | ___ _ __  _ __ ___  ___ __ _| |_ ___  __| |
  | |  | |/ _ \ '_ \| '__/ _ \/ __/ _` | __/ _ \/ _` |
  | |__| |  __/ |_) | | |  __/ (_| (_| | ||  __/ (_| |
  |_____/ \___| .__/|_|  \___|\___\__,_|\__\___|\__,_|
  ............| |Working on removing these.
  ............|_|They add nothing at this point.
  ...............Vestiges from wifiibo
  ...............Might add sd card support back in once I get this working better
*/

bool isSPIFFSFile(String path) {
  return true;
}

