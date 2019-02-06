//Version and program intro.
char VERSION[] = "1.2";
char PROGRAM_DESCRIPTION[] =
  "LyfeOnEdge's Amiibo cloning tool.\nUART interface for cloning, writing, and dumping Amiibos.\nWorks with NTAG215 NFC Tags and Cards.\nBased on Xerxes3rd's Wifiibo project and socram8888's amitool available on github";



//MENU STUFF                                    //Hard coded definitions for program flow
#define BUTTON D8                               //Button for starting reads and writes, helps prevent damage to cards  //Plan is to change this to a button on the screen of the wifi
#define BACK_BUTTON           99
//Main menu, always reserve 0 for default, 99 for back
#define MAINFUNCTIONSIZE      10                        //Total number of functions, adjust accordingly
#define DUMP_AMIIBO           1
#define CLONE_AMIIBO          2
#define SAVE_AMIIBO           3
#define DUMP_FILE             4
#define DELETE_FILE           5
#define RENAME_FILE           6
#define WRITE_FILE            7
#define PRINT_FILE_LIST       8
#define MORE_UTILS            9
#define RESTART_ESP           10
const String mainFunctionStrings[MAINFUNCTIONSIZE] =      //Function name strings, if you want to add additional functions
{
  "Dump Amiibo",     //1
  "Clone Amiibo",    //2
  "Save Amiibo",     //3
  "Dump File",       //4
  "Delete File",     //5
  "Rename File",     //6
  "Write File",      //7
  "List Files",      //8
  "More Utils",      //9
  "Restart ESP"      //10
};


//UTIL MENU STUFF
#define UTILSIZE              3 //Back button always 99, not defined here
#define TOGGLE_DEBUG          1
#define DOWNLOAD_KEY          2
const String moreUtilsStrings[UTILSIZE] =      //Function name strings, if you want to add additional functions
{
  "Toggle Debug",     //1
  "Download Key",     //2
  "Back"              //3
};


//Global Commands
//Change these to strings that will trigger a function outside normal console flow,
//must have an associated check in function serialInterrupt(String checkString) in order to work
//PLANS: EXIT, UP
#define RESTART_STRING "restart"
#define BACK_STRING    "back"
#define UP_STRING      "up"

//SIZE DEFINITIONS
#include <portable_endian.h>

#ifndef SPIFFS_OBJ_NAME_LEN
#define SPIFFS_OBJ_NAME_LEN             (32)
#endif

#define NTAG_SIZE  540
