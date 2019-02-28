/*
  _____  ______ ______ _____ _   _ _____ _______ _____ ____  _   _  _____
  |  __ \|  ____|  ____|_   _| \ | |_   _|__   __|_   _/ __ \| \ | |/ ____|
  | |  | | |__  | |__    | | |  \| | | |    | |    | || |  | |  \| | (___
  | |  | |  __| |  __|   | | | . ` | | |    | |    | || |  | | . ` |\___ \
  | |__| | |____| |     _| |_| |\  |_| |_   | |   _| || |__| | |\  |____) |
  |_____/|______|_|    |_____|_| \_|_____|  |_|  |_____\____/|_| \_|_____/

..    _________________   _________________
  .-/|                 \ /                 |\-.
  ||||                  |                  ||||
  ||||       ~~~~       |       ~~*~~      ||||
  ||||       READ       |        EAT       ||||
  ||||       BOOK       |       PANTS      ||||
  ||||                  |                  ||||
  ||||                  |                  ||||
  ||||                  |                  ||||
  ||||                  |                  ||||
  ||||                  |                  ||||
  ||||                  |                  ||||
  ||||_________________ | _________________||||
  ||/==================\|/==================\||
  `-------------------~___~------------------''


Non-user definitions, mostly for version control and menuing
*/

//Version and program intro.
char VERSION[] = "1.9";
char PROGRAM_DESCRIPTION[] =
  "LyfeOnEdge's Amiibo cloning tool.\nUART interface for cloning, writing, and dumping Amiibos.\nWorks with NTAG215 NFC Tags and Cards.\nBased on Xerxes3rd's Wifiibo project and socram8888's amitool available on github";

//SIZE DEFINITIONS
#include <portable_endian.h>

#ifndef SPIFFS_OBJ_NAME_LEN
#define SPIFFS_OBJ_NAME_LEN             (32)
#endif

#define NTAG_SIZE  540

//MENU STUFF                                    //Hard coded definitions for program flow
#define BUTTON D8                               //Button for starting reads and writes, helps prevent damage to cards  //Plan is to change this to a button on the screen of the wifi
//Main menu, always reserve 0 for default, 99 for back
#define FUNCNUM               11
#define DUMP_AMIIBO           "dumpa"
#define CLONE_AMIIBO          "clone"
#define SAVE_AMIIBO           "savea"
#define DUMP_FILE             "dumpf"
#define DELETE_FILE           "delete"
#define RENAME_FILE           "rename"
#define WRITE_FILE            "write"
#define PRINT_FILE_LIST       "list"
#define TOGGLE_DEBUG          "debug"
#define RESTART_ESP           "restart"
#define HELP                  "help"

//flag order: -p -n -t -a
#define MAXTOKENS             3     //max number of tokens ever expected to be passed
#define pToken                "-p"  //standard file path
#define nToken                "-n"  //number of ops
#define tToken                "-t"  //rename target
#define aToken                "-a"  //bool, modifier for list functions

#define TRUESTRING            "true"
#define FALSESTRING           "false"

const String commandList[FUNCNUM] = 
{
  DUMP_AMIIBO, 
  CLONE_AMIIBO, 
  SAVE_AMIIBO, 
  DUMP_FILE, 
  DELETE_FILE, 
  RENAME_FILE, 
  WRITE_FILE, 
  PRINT_FILE_LIST, 
  TOGGLE_DEBUG, 
  RESTART_ESP, 
  HELP
};

const String funcStrings[FUNCNUM] =      //Function name strings, if you want to add additional functions
{
  "Dump Amiibo",     //1
  "Clone Amiibo",    //2
  "Save Amiibo",     //3
  "Dump File",       //4
  "Delete File",     //5
  "Rename File",     //6
  "Write File",      //7
  "List Files",      //8
  "Debug",           //9
  "Restart ESP",     //10
  "Help",            //11
};

const String funcDesc[FUNCNUM] =      //Function name strings, if you want to add additional functions
{
  "Dump byte array of physical amiibo",          //no arguments
  "Clone Amiibo, specify a number of clones",    //-n (number of clones to make)
  "Scan and save Amiibo to file",                //-p (file to save as)
  "Dump File to byte array",                     //-p (file to dump)
  "Delete File at given location",               //-p (file to delete)
  "Rename File at given location",               //-p (file to rename) -t (rename target)
  "Write File to blank card",                    //-p (file to write) -n (number of time to write)
  "List Files",                                  //-a (amiibos only) || (no argument for all files)
  "Toggles debug spewage",                       //no arguments
  "Immediately Restart ESP",                     //no arguments
  "Displays help for given command"

};

const String funcUsage[FUNCNUM] =
{
  "dumpa",
  "clone -n (number)",
  "savea -p (file to save as)",
  "dumpf -p (file to dump)",
  "delete -p (file to delete)",
  "rename -p (file to rename) -t (rename target)",
  "write -p (file to write) -n (number of time to write)",
  "list -a true/false (true = list amiibos only)",
  "debug",
  "restart",
  "help -p (command)"
};





