
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



////USER DEFINITIONS////

//Uncomment for the reader you are using
//#define USE_PN532 true                        //In the future I would like to do card emulation with this
#define USE_MFRC522 true


//Change these to the username and password for your wifi
const char* ssid = "Ridley";                    //Change this to the name of your 2.4GHz wifi
const char* password = "metroids7";             //Change this to your wifi password
const char* hostName = "ESPmiibo";              //Name of the wifi access point
const char* http_username = "admin";            //Not actually used right now
const char* http_password = "password";         //Gonna be naughty and leave them as default

//Change this to the rate of your serial console
const int baudrate = 9600;


////END USER DEFINITIONS////



















//Includes and definitions
#define FS_NO_GLOBALS //allow spiffs to coexist with SD card, define BEFORE including FS.h, not yet needed but will be fixed
#include <string.h>
#include <Arduino.h>
#include <FS.h>
#include <Hash.h>
#include <amiibo.h>
#include <amiitool.h>
#include <drbg.h>
#include <keygen.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include "definitions.h"

//NFC LIBRARIES
#ifdef USE_PN532
#include <Adafruit_PN532Ex.h>   //I'm only using the mfrc522
#endif

#ifdef USE_MFRC522
#include <MFRC522Ex.h>
#endif





//This is janky but it should simplify the file downloading and make it easier in the future. This way I alter the settings but not the function between versions
//For each element of downloadSite specify what you want it to download as
//See function handleDownloadFiles(String sitesToDownload[][], int NumFilesToDownload);

#define NUM_FILES_TO_DOWNLOAD       2
#define KEYFILENAMESTRING           "/key_retail.bin"
#define URLSTRING                   0
#define FILESTRING                  1
//Key File
const String keyfilesite =          "http://share.dmca.gripe/xuIdmgJ01IZ4Fwis.bin";
const String keyfilenamestring =    KEYFILENAMESTRING;                         //needed for automatic renaming of downloaded keyfile
const char* keyfilename =           KEYFILENAMESTRING;                                //needed for atool
//Main file
const String mainfilesite =         "https://share.dmca.gripe/5XVeC8hUM8VAJwdf.htm";  //static serving of main.htm
const String mainfilestring =       "/main.htm";

const String rootFiles[][2] =  {{keyfilesite, keyfilenamestring}, {mainfilesite, mainfilestring}};



//ATOOL INSTANCTIATION
amiitool atool((char*)keyfilename);             //instantiate amitool, passing the path to the encryption key

//MFRC NEED TO FIX PINOUTS
MFRC522Ex mfrc522(D1, D2);

//GLOBAL BOOLEANS
volatile bool triggerReadNFC = false;              //used to trigger a read, set to true then call handleNFC(), will be reset by function
volatile bool triggerWriteNFC = false;             //used to trigger a write, set to true then call handleNFC() will be reset by function
volatile bool WiFiButtonPushed = false;            //used to track if the wifi flash button has been pushed.
volatile bool consoleToDebug = false;              //used to enable spew of console to debug
volatile bool enableDebug    = true;               //used to enable debug spew
// SKETCH BEGIN
ESP8266WebServer server(80);
fs::File fsUploadFile;                             // a File object to temporarily store the received file

//(prototypes)
String getContentType(String filename);      // convert the file extension to the MIME type
bool handleFileRead(String path);         // send the right file to the client (if it exists)
void handleFileUpload();                           // upload a new file to the SPIFFS





/*
  .._____ ______ _______ _    _ _____
  ./ ____|  ____|__   __| |  | |  __ \
  | (___ | |__     | |  | |  | | |__) |
  .\___ \|  __|    | |  | |  | |  ___/
  .____) | |____   | |  | |__| | |
  |_____/|______|  |_|   \____/|_|
*/

void setup() {
  ESP.wdtEnable(1000);     //passed value does nothing, this is a preemptive bug fix since I found soft resets (reset button) do not always reenable the wdt if a function disabled it.
  pinMode(BUTTON, INPUT);   //set up the "flash" button

  Serial.begin(baudrate);
  debug("\n\nSerial started at: " + String(baudrate));
  consoleln("Starting...");

  //Start onboard file system
  if (SPIFFS.begin()) {
    debug(F("Spiffs Running"));
  } else {
    debug(F("Spiffs Disabled"));
  }

  //Check to see if the nfc device is connected
  if (atool.initNFC(&mfrc522)) {
    debug(F("NFC Found"));
  } else {
    debug(F("NFC Not Found"));
  }

  wifiSetup();
  webpageSetup();
  getFileList();
  handleDownloadFiles(rootFiles, NUM_FILES_TO_DOWNLOAD);   //download root
  checkRetailKey();
  consoleln(F("Setup Complete."));

  //Spew program info
  Serial.println(PROGRAM_DESCRIPTION);
  Serial.print(F("ESPmiibo Version: ")); Serial.println(VERSION);

  console("\n\n");
}






/*
  .__  __          _____ _   _     _      ____   ____  _____
  |  \/  |   /\   |_   _| \ | |   | |    / __ \ / __ \|  __ \
  | \  / |  /  \    | | |  \| |   | |   | |  | | |  | | |__) |
  | |\/| | / /\ \   | | | . ` |   | |   | |  | | |  | |  ___/
  | |  | |/ ____ \ _| |_| |\  |   | |___| |__| | |__| | |
  |_|  |_/_/    \_\_____|_| \_|   |______\____/ \____/|_|
*/
void loop() {

  //I UNDERSTAND THIS IS A TERRIBLE USE OF IF STATEMENTS.
  //TERRIBLE.
  //It used to be a switch statement but it wasn't playing well with the esp.
  //That switch statement was the source of 4 hours of headaches.
  //It's else ifs now.

  /*ESP8266 SWITCH HAIKU

    Crashed nowhere near.
    Stack traces were my friend.
    Screw the ESP.

  */

  int chosenFunction = listFunctions(mainFunctionStrings, MAINFUNCTIONSIZE);


  /*  DUMP AMIIBO   */
  if (chosenFunction == DUMP_AMIIBO) {
    mainFunctionDumpAmiibo();

    /*  CLONE AMIIBO  */
  } else if (chosenFunction == CLONE_AMIIBO) {
    mainFunctionCloneAmiibo();

    /*  SAVE AMIIBO  */
  } else if (chosenFunction == SAVE_AMIIBO) {
    mainFunctionSaveAmiibo();

    /*  DUMP FILE    */
  } else if (chosenFunction == DUMP_FILE) {
    mainFunctionDumpFile();

    /*  DELETE FILE  */
  } else if (chosenFunction == DELETE_FILE) {
    mainFunctionDeleteFile();

    /*  RENAME FILE  */
  } else if (chosenFunction == RENAME_FILE) {
    mainFunctionRenameFile();

    /*   WRITE FILE  */
  } else if (chosenFunction == WRITE_FILE) {
    mainFunctionWriteFile();

    /*  Print local file list */
  } else if (chosenFunction == PRINT_FILE_LIST) {
    getFileList();

    /* Brings up more utils submenu*/
  } else if (chosenFunction == MORE_UTILS) {
    mainFunctionMoreUtils();

    /*RESTART */
  } else if (chosenFunction == RESTART_ESP) {
    debug("\nRestarting... \n");
    console("\nRestarting...\n");
    ESP.restart();

    /* Default */
  } else {
    console(F("Unrecognized command"));
  }


  yieldEvent();
}






/*
  .__  __          _____ _   _    ______ _    _ _   _  _____ _______ _____ ____  _   _  _____
  |  \/  |   /\   |_   _| \ | |  |  ____| |  | | \ | |/ ____|__   __|_   _/ __ \| \ | |/ ____|
  | \  / |  /  \    | | |  \| |  | |__  | |  | |  \| | |       | |    | || |  | |  \| | (___
  | |\/| | / /\ \   | | | . ` |  |  __| | |  | | . ` | |       | |    | || |  | | . ` |\___ \
  | |  | |/ ____ \ _| |_| |\  |  | |    | |__| | |\  | |____   | |   _| || |__| | |\  |____) |
  |_|  |_/_/    \_\_____|_| \_|  |_|     \____/|_| \_|\_____|  |_|  |_____\____/|_| \_|_____/

*/

void yieldEvent() {
  server.handleClient();
  yield();
}

void mainFunctionDumpAmiibo() {
  waitForButton();
  triggerReadNFC = true;
  handleNFC();
  console(F("\nEncrypted Data:\n"));
  ESP.wdtDisable(); //Dump takes too long, triggers wdt, we have six seconds before the hardware wdt resets here.
  atool.printData(atool.original, NTAG215_SIZE, 4, false, false);
  ESP.wdtEnable(1000); //reenable wdt, the value passed does nothing.
  consoleln(F("Dump Complete\n\n"));
  //Original dump method, used library method, but it can't be dumped directly.
  //dump_amiibo_array(atool.original, NTAG_SIZE);
  /*
    Serial.println(F("\nDecrypted Data:\n"));
    atool.printData(atool.modified, NFC3D_AMIIBO_SIZE, 4, false, false);
  */
}

void mainFunctionSaveAmiibo() {
  consoleln(F("Save file as (no extension): "));
  String saveName = serialEvent();
  consoleln("Amiibo will be saved as: " + saveName + ".bin");
  consoleln(F("Scan Amiibo"));
  bool amiiboSaved = false;                           //bool to track if the amiibo has been saved successfully
  while (!amiiboSaved) {                              //until we save successfully
    waitForButton();
    triggerReadNFC = true;
    if (handleNFC()) {                                  //if card read successfully
      saveAmiibo(saveName + F(".bin"));               //Save amiibo
      amiiboSaved = true;
    } else {
      consoleln(F("Card read failed.\nRetrying...\n"));
    }
    yieldEvent();
  }
}

//WILL NEED TO BE FIXED IN THE LONG RUN
//CURRENTLY DOESN'T PRINT TO A TARGET LOCATION, MAY USE OLD DUMP METHODS
//WILL HAVE TO MODIFY OLD METHODS TO WORK WITH YIELD CALLS
void mainFunctionDumpFile() {
  consoleln(F("\nChoose Amiibo file to dump:\n"));
  getAmiiboFileList();                                            //print list of valid writiable files
  console(F("Type file path to open or type exit: "));
  String fileString = serialEvent();                              //get the file path to write as a prompt through serial
  if (fileString == F("exit")) {
    consoleln(F("Exiting..."));
  } else {                                                       //if not exit command
    if (SPIFFS.exists(fileString)) {                             //check if file exists
      consoleln("\nDumping file: " + fileString);
      fs::File f = SPIFFS.open(fileString, "r");                      //open the file
      if (atool.loadFileSPIFFS(&f, false)) {                          //if file successfully loaded                                       //set write flag
        ESP.wdtDisable(); //Dump takes too long, triggers wdt, we have six seconds before the hardware wdt resets here.
        atool.printData(atool.original, NTAG215_SIZE, 4, false, false);
        ESP.wdtEnable(1000); //reenable wdt, the value passed does nothing.
      }
    } else {
      consoleln(F("File does not exist"));
    }
  }
}


void mainFunctionDeleteFile() {
  bool fileDeleted = false;                           //boolean to continue retying until the file has been deleted successfully
  while (!fileDeleted) {
    consoleln(F("Choose file to delete or type 'exit':"));
    getFileList();                                      //Print file list
    String fileToDelete = serialEvent();                //get file path to delete
    if (fileToDelete == F("exit")) {
      fileDeleted == true;
      consoleln(F("Exiting..."));
      break;
    } else {
      if (SPIFFS.exists(fileToDelete)) {
        if (SPIFFS.remove(fileToDelete)) {                  //Delete file
          consoleln(F("File deleted successfully"));   //If successful
        } else {
          consoleln(F("File deletion unsuccessful\nRetrying...\n"));  //If unsuccessful
        }
      } else {
        consoleln(F("File does not exist\nRetrying...\n"));
      }
    }
    yieldEvent();
  }
}

void mainFunctionRenameFile() {
  bool fileRenamed = false;
  while (!fileRenamed) {
    consoleln(F("Choose file to rename or type 'exit' to go back:"));
    getFileList();                                      //Print file list
    String fileToRename = serialEvent();                //get file path to rename
    if (fileToRename == F("exit")) {
      fileRenamed = true;
    } else {
      console("Rename " + fileToRename + " to:");
      String newFileName = serialEvent();
      if (SPIFFS.rename(fileToRename, newFileName)) {
        consoleln(F("Rename successful!"));
        fileRenamed = true;
      } else {
        consoleln(F("Rename unsuccessful."));
      }
    }
    yieldEvent();
  }
}

void mainFunctionWriteFile() {
  //repeat until the write is successful
  consoleln(F("\nChoose Amiibo dump:\n"));
  getAmiiboFileList();                                              //print list of valid writiable files
  console(F("Enter file path to open: "));
  String fileString = serialEvent();                                //get the file path to write as a prompt through serial
  if (SPIFFS.exists(fileString)) {                                  //make sure file exists
    consoleln("\nWriting file: " + fileString);
    fs::File f = SPIFFS.open(fileString, "r");                        //open the file
    bool writeSuccessful = false;                                     //bool to track if amiibo was written successfully
    if (atool.loadFileSPIFFS(&f, false)) {                          //if file successfully loaded

      while (!writeSuccessful) {
        waitForButton();
        triggerWriteNFC = true;                                       //set write flag
        if (handleNFC()) {                                            //print whether we wrote successfully
          consoleln(F("\n Tag written successfully."));
          writeSuccessful = true;                                     //if we write successfully set flag to exit loop
        } else {                                                      //if file wasn't writen, retry
          consoleln(F("Tag write unsuccessful\nRetrying..."));
          delay(200);
        }
        yieldEvent();
      }
      yieldEvent();
    } else {
      consoleln(F("Couldn't open file. \n Exiting... \n"));
    }
  } else {
    consoleln(F("File does not exist. \n Exiting... "));
  }
}


void mainFunctionCloneAmiibo() {
  consoleln(F("Scan source Amiibo."));
  waitForButton();
  triggerReadNFC = true;                                  //set flag to read
  bool readSuccessful = false;
  bool writeSuccessful = false;
  server.handleClient();
  while (!readSuccessful) {                                        //repeat until the write is successful
    if (handleNFC()) {
      readSuccessful = true;
      consoleln(F("Amiibo read successful"));
      consoleln(F("Scan blank tag."));
      waitForButton();
      while (!writeSuccessful) {
        waitForButton();
        triggerWriteNFC = true;
        if (handleNFC()) {
          writeSuccessful = true;

          consoleln(F("Tag written successfully\n\n"));
        } else {
          consoleln(F("Write unsuccessful\nRetrying..."));
        }
      }
      yieldEvent();
    }
    yieldEvent();
  }
}

void mainFunctionMoreUtils() {
  int chosenUtil = listFunctions(moreUtilsStrings, UTILSIZE);

  /*  TOGGLE DEBUG  */
  if (chosenUtil == TOGGLE_DEBUG) {
    moreUtilsToggleDebug();

    /*  DOWNLOAD KEY  */
  } else if (chosenUtil == DOWNLOAD_KEY) {
    getKeyBin();
  }
}



/*
  __  __  ____  _____  ______     _    _ _______ _____ _       _____
  |  \/  |/ __ \|  __ \|  ____|   | |  | |__   __|_   _| |     / ____|
  | \  / | |  | | |__) | |__      | |  | |  | |    | | | |    | (___
  | |\/| | |  | |  _  /|  __|     | |  | |  | |    | | | |     \___ \
  | |  | | |__| | | \ \| |____    | |__| |  | |   _| |_| |____ ____) |
  |_|  |_|\____/|_|  \_\______|    \____/   |_|  |_____|______|_____/
*/
//Flips debug mode,
bool moreUtilsToggleDebug() {
  enableDebug = !enableDebug;

  if (enableDebug) {
    debug("Debug output enabled.");
    console("Debug output enabled.");
  } else {
    debug("Debug output disabled.");
    console("Debug output disabled.");
  }
}











/*
  __          _______ _____  ______ _      ______  _____ _____
  \ \        / /_   _|  __ \|  ____| |    |  ____|/ ____/ ____|
  .\ \  /\  / /  | | | |__) | |__  | |    | |__  | (___| (___
  ..\ \/  \/ /   | | |  _  /|  __| | |    |  __|  \___ \\___ \
  ...\  /\  /   _| |_| | \ \| |____| |____| |____ ____) |___) |
  ....\/  \/   |_____|_|  \_\______|______|______|_____/_____/
  You can change Wifi.mode() to WIFI_AP for access point only or
  WIFI_STA for local address mode.
*/

void wifiSetup() {
  //Set up server and Wifi*
  WiFi.hostname(hostName);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESPmiibo");

  if (WiFi.begin(ssid, password)) {
    debug("Connected to " + WiFi.SSID());
    while (WiFi.status() != WL_CONNECTED) {           //waiting dot counter until ip assigned
      delay(500);
      debug(".");
    }
    debug("IP address: " + WiFi.localIP());
  } else {
    debug("Wifi failed to start");
  }
  serialSeparator();
}












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
    yield();
    server.handleClient();
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




/*
  .__  __ _____  _____  _____
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

//List all files on the root directory of the esp
//PLANNED: void getFileList(String pathToList)
//PLANNED: Merge with getAmiiboFileList()
void getFileList() {
  debug(F("\nPrinting local file list: "));
  byte listSize = 0;                                                                    //create a byte to count the number of files in the directory
  fs::Dir dir = SPIFFS.openDir("");                                                     //open node root directory (THIS IS BELOW '/')
  while (dir.next()) {                                                                  //cycle through all files until the end is reached
    fs::File f = dir.openFile("r");                                                     //open the file
    Serial.println(String(dir.fileName()));                                             //print the name
    f.close();                                                                          //close it
    listSize++;                                                                         //increase our list size
  }
  Serial.println("Counted: " + String(listSize) + " file(s)");
}

void getAmiiboFileList() {
  debug(F("\nPrinting local amiibo file list: "));
  byte listSize = 0;                                                                    //create a byte to count the number of files in the directory
  fs::Dir dir = SPIFFS.openDir("");                                                     //open node root directory (THIS IS BELOW '/')
  while (dir.next()) {                                                                  //cycle through all files until the end is reached
    if (dir.fileName().endsWith(F(".bin")) || dir.fileName().endsWith(F(".BIN"))) {     //check each to see if it is a bin file
      fs::File f = dir.openFile("r");                                                   //open the file
      if (amiitool::isSPIFFSFilePossiblyAmiibo(&f)) {                                   //check if the file is an amiibo
        Serial.println(String(dir.fileName()));                                         //print the name of every amiibo file as it is found
        listSize++;                                                                     //increase our list size
      }
      f.close();                                                                        //close it
    }
    //Serial.println("Counted: " + String(listSize) + " Amiibo(s)");
  }
}

//Following code it heavily edited to an unrecognizable state, however original can be found at the wiifibo project on github. I like what they were trying to do, but the webpage interface is too slow for my liking
bool checkRetailKey() {               ///Checks to see if we have a valid decryption key uploaded.
  if (!atool.tryLoadKey()) {
    consoleln(F("Invalid Key"));
  }
  else {
    consoleln(F("Found Key"));
  }
}

//Edit this function based on the restart method of your device
void restartFunction() {
  ESP.restart();
}


//ADDITIONAL OPTIONS// // // // // // // // // // // // // // // // // // // // //
//Displays a list of options from an array of strings and returns the location of the string chosen, returns 0 if nothing chose
int listFunctions(const String * functionStrings, int numberOfFunctions) {
  debug("Printing function list.");
  consoleln(F("Functions: "));
  for (int i = 0; i < numberOfFunctions; i++) {
    console(String(i + 1) + ": ");                       //print the number corresponding to each command in the list
    consoleln(functionStrings[i]);
  }
  serialSeparator();
  int chosenFunction = serialEvent().toInt();            //take the incoming command and turn it into an integer, defaults to 0 which is why we reserve it

  if (chosenFunction) {
    console(F("Function Number: "));
    consoleln(chosenFunction);                           //print the function number
    console(F("Executing Function: "));
    consoleln(functionStrings[chosenFunction - 1]);      //print the command name from the array of strings, increase by one since the incoming number will be one more since the array starts from zero. done because we are reserving 0
    debug("Exiting listFunctions with function: " + String(functionStrings[chosenFunction - 1]));
    debug("Exiting listFunctions with status: " + String(chosenFunction));
    return chosenFunction;
  } else {
    console(F("No function selected."));
    debug(F("Exiting listFunctions with function status 0"));
    return 0;
  }
}






/*
  __          ________ ____  _____        _____ ______      _____ _______ _    _ ______ ______
  \ \        / /  ____|  _ \|  __ \ /\   / ____|  ____|    / ____|__   __| |  | |  ____|  ____|
  .\ \  /\  / /| |__  | |_) | |__) /  \ | |  __| |__      | (___    | |  | |  | | |__  | |__
  ..\ \/  \/ / |  __| |  _ <|  ___/ /\ \| | |_ |  __|      \___ \   | |  | |  | |  __| |  __|
  ...\  /\  /  | |____| |_) | |  / ____ \ |__| | |____     ____) |  | |  | |__| | |    | |
  ....\/  \/   |______|____/|_| /_/    \_\_____|______|   |_____/   |_|   \____/|_|    |_|
  This is still heavily WIP. Will be adding more buttons in the future.
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








/*
   __          ________ ____  _____        _____ ______     ____          _____ _  ________ _   _ _____
  \ \        / /  ____|  _ \|  __ \ /\   / ____|  ____|   |  _ \   /\   / ____| |/ /  ____| \ | |  __ \
  .\ \  /\  / /| |__  | |_) | |__) /  \ | |  __| |__      | |_) | /  \ | |    | ' /| |__  |  \| | |  | |
  ..\ \/  \/ / |  __| |  _ <|  ___/ /\ \| | |_ |  __|     |  _ < / /\ \| |    |  < |  __| | . ` | |  | |
  ...\  /\  /  | |____| |_) | |  / ____ \ |__| | |____    | |_) / ____ \ |____| . \| |____| |\  | |__| |
  ....\/  \/   |______|____/|_| /_/    \_\_____|______|   |____/_/    \_\_____|_|\_\______|_| \_|_____/
  You probably don't want to mess with this.
*/

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}


void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  debug("handleFileList: " + path);
  fs::Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while (dir.next()) {
    fs::File entry = dir.openFile("r");
    if (output != "[") {
      output += ',';
    }
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir) ? "dir" : "file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }

  output += "]";
  server.send(200, "text/json", output);
  serialSeparator();
}


//Webpage Stuff
//MUCH OF THIS IS FROM THE ESP_AsyncFSBrowser example
void handleWebRequests() {
  if (loadFromSpiffs(server.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  debug(message + '\n');
}


bool loadFromSpiffs(String path) {
  String dataType = "text/plain";
  if (path.endsWith("/")) path += "index.htm";
  if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(".html")) dataType = "text/html";
  else if (path.endsWith(".htm")) dataType = "text/html";
  else if (path.endsWith(".css")) dataType = "text/css";
  else if (path.endsWith(".js")) dataType = "application/javascript";
  else if (path.endsWith(".png")) dataType = "image/png";
  else if (path.endsWith(".gif")) dataType = "image/gif";
  else if (path.endsWith(".jpg")) dataType = "image/jpeg";
  else if (path.endsWith(".ico")) dataType = "image/x-icon";
  else if (path.endsWith(".xml")) dataType = "text/xml";
  else if (path.endsWith(".pdf")) dataType = "application/pdf";
  else if (path.endsWith(".zip")) dataType = "application/zip";
  fs::File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }
  dataFile.close();
  return true;
}


bool handleFileRead(String path) { // send the right file to the client (if it exists)
  debug("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    fs::File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    debug(String("\tSent file: ") + path);
    serialSeparator();
    return true;
  }
  debug(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  debug("");
  return false;
}

void handleFileUpload() {
  if (server.uri() != "/edit") {
    return;
  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    debug("handleFileUpload Name: " + filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    debug("handleFileUpload Size: " + upload.totalSize);
  }
  serialSeparator();
}

void handleFileDelete() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  debug("handleFileDelete: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (!SPIFFS.exists(path)) {
    return server.send(404, "text/plain", "FileNotFound");
  }
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
  serialSeparator();
}

void handleFileCreate() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  debug("handleFileCreate: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (SPIFFS.exists(path)) {
    return server.send(500, "text/plain", "FILE EXISTS");
  }
  fs::File file = SPIFFS.open(path, "w");
  if (file) {
    file.close();
  } else {
    return server.send(500, "text/plain", "CREATE FAILED");
  }
  server.send(200, "text/plain", "");
  path = String();
  serialSeparator();
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
bool handleNFC() {                    //Heavily edited, removed a lot of unwanted functionality
  if (triggerReadNFC) {
    triggerReadNFC = false;
    console(F("Reading NTAG"));
    bool result = atool.readTag(sendStatusCharArray, updateProgress);

    if (result)
    {
      return true;
    } else {
      console(F("Tag read failed."));
      return false;
    }
  }
  else if (triggerWriteNFC) {
    triggerWriteNFC = false;
    console(F("Writing NTAG"));
    bool result = atool.writeTag(sendStatusCharArray, updateProgress);
    if (result)
    {
      console(F("Tag written."));
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

/*
  void deleteFile(String filename) {

  Serial.println("Deleting" + filename);
  delay(50);
  if (SPIFFS.remove(filename))
    Serial.println(F("Amiibo Deleted."));
  else
    Serial.println(F("Delete Failed"));
  }
*/

bool saveAmiibo(String filename) {      //altered to return true if successful, heavily altered error reporting to be serial friendly
  int bytesOut = 0;

  if (filename.length() > SPIFFS_OBJ_NAME_LEN - 1) {
    console(F("Filename too long."));                                           //Altered original code for this function to debug via serial //code 1
    return false;
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


/*
  ._____                                _           _
  |  __ \                              | |         | |
  | |  | | ___ _ __  _ __ ___  ___ __ _| |_ ___  __| |
  | |  | |/ _ \ '_ \| '__/ _ \/ __/ _` | __/ _ \/ _` |
  | |__| |  __/ |_) | | |  __/ (_| (_| | ||  __/ (_| |
  |_____/ \___| .__/|_|  \___|\___\__,_|\__\___|\__,_|
  ............| |Working on removing these.
  ............|_|They add nothing at this point.
*/

void sendStatusCharArray(char *statusmsg) {}
void updateProgress(int percent) {}
void sendFunctionStatusCode(char *funcName, int code) {}
void sendTagInfo() {}
bool isSPIFFSFile(String path) {
  return true;
}

