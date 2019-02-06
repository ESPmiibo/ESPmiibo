               _______   _______  ______       .__._.__. 
              / / ____/ / / ___/ / ____ |_____ |__|__\_ |__   ____   
             / / ____/ /___   / / / ___/      \|  |  || __ \ /  _ \   
            /_/_____/ /______/ /_/_/   |  Y Y  \  |  || \_\ (  <_> )
                                       |__|_|__/__|__||_____/\____/   
                            
       /Nd+`                 Written by LyfeOnEdge                ,`:hNo      
     `yMMMMMy:                                                   -sNMMMMd.     
    `dMMMMMMMMmo.          https://discord.gg/NfpNXqD         .+dMMMMMMMMN-    
   `mMMMMMMMMMMMMs                                           /NMMMMMMMMMMMN:   
   .+sdNNMMMMMMMs                                             /NMMMMMMMNmy+-   
       `-+sdNNM+      `````.`      ```````````       .----.   -NMNmyo:.       
            `::   .:shdmNNNNN-  `+hmNNNNNNNNNN-    /hmMMMMMMmh/            
                -yNMMMmdso+/:` .mMMMms++++++++`  .dMMMd+::+dMMMm/              
 //::-...``   `sNMMMdo.        +MMMM.            hMMMd`    `dMMMN.  ,,....:://.
:MMMMNNmmmdhs oMMMM+ -         :NMMMh/.          MMMMo      sMMMM.:hdmmmNNMMMM+
+MAMAMADEMEMy NMMMNssssssssss.  :hNMMMNds/.      MMMM+````./NMMMd /MMMMMMMMMMMo
+MASHMYMNMSSy MMMMNhhhhhhhhhh.    ./ydNMMMNh+.   MMMMmmmmmmMMMms` /MMMMMMMMMMMs
+MMMMMMNNNmmy mMMMN.                 `./ymMMMm/  MMMMyMMMMMM+:`   /dmmNNMMMMMM+
 ++/:::....`  :NMMMd-                    `hMMMN  MMMM/             ``...-::/++.
               -hMMMNy:.                 .yMMMM  MMMM/                         
             .. `-sdMMMNmmmmm- yummmmmmmmNMMMN/  MMMM/          .-`            
        ./oymNN:   `./assssss. assssssssssss/.   asss-         .mNmho/-        
   `:ohmNMOOOOMNo                                             :NMMMMMMNmho/.   
   .mMMMMOOMMMMOOs                                           /NMMMMMMMMMMMN:   
    .mMMCOWMMMNy:`                                            -sNMMMMMMMMN:    
     .hMMMMMd+`                                                 `/hMMMMMm-     
      `MOOO.        Developed for and with help from /hgb/        `+dMy`      
#KhatrieHatesASCII #KhatrieSawATheAsciiD #:(){ :|: & };: #ForkYou #ChownSlash

V1.3 Released 2/6/2019
Special thanks to:
discord users:
  /hbg/ 
    not-cdn - - - - - - - - - - - - - - - - - - - - HTML Support
    Dave Devils - - - - - - - - - - - - - - - - - - Hosting Support
    Waifupls  - - - - - - - - - - - - - - - - - - - Other Support
    (╯°□°）╯︵ ┻━┻  - - - - - - - - - - - - - - - - General Table Flipping

#MEGAripoff #ARR #Torrents>Reindeer #MEGAFBI #HackThePlanet

If I forgot you message me and I'll credit you.

LyfeOnEdge's $8 Amiibo cloning tool.
  ESP8266 interface for reading, writing, cloning, and dumping Amiibos.
  Works with NTAG215 NFC Tags and Cards.
  Based on Xerxes3rd's Wifiibo project and socram8888's amitool available on github.

  It is based on Wifiibo project found here: https://github.com/Xerxes3rd/Wifiibo
  The Wifiibo project is a port of the amiitool project found here: https://github.com/socram8888/amiitool

  Understand that the long-term goal of this tool is to make flashing large batches of amiibos cheap. Currently only one at a time is supported but in version 1.0 I will try to add batch mode functionality for flashing whole folders in one go or the same amiibo multiple times.
  
  Necessary parts:
    ESP8266 (Look for NodeMCU, usually $4)
    MFRC522 NFC reader breakout (also $4)
    Button (Will not be required in version 1.1 and later)

  Recommended parts:
    HC05 or similar Serial -> Bluetooth adapter (This will allow you to control the cloner from your phone or pc via bluetooth)
    Blank NTAG215 Cards for Cloning 

  If you decide to alter anything, feel free, but please credit me (LyfeOnEdge) and release it under a distinct name. My only warning is to make sure you use yield() judiciously within loops, the watchdog barks easily.

Features: 
    
    Upload: Files can be uploaded by USB (tedious) or WiFi (easy!)
    Connect to the device like you would a wifi port and visit a simple website to upload Amiibos and the encryption key if you don't have it (not included).

    Flashing:
        Amiibos saved onboard can be flashed to blank NTAG215 Cards.

    Cloning:
        Currently allows only exact duplicates to be made (eg. no on-the fly adjustment encrypted data and Stats), in the future I plan on adding stat adjustment abilities like more hearts for the midna link Amiibo.

    File management:
        Basic file management. With the Amiibo dumps being 540 bytes in length and the ESP8266 having 4mb of space you could fit roughly 7000 amiibos on it. That being said it's sometimes nice to delete a few... 

    Dumping:
        Dump the text contents of physical figures or saved amiibos for use elsewhere.

    Planned Features: 
        Command Line:
            - Batch: Both folder batch and single amiibo bulk production

        WiFi:
             - Webpage: Flash button, file-to-flash selection, file-name entrance when saving, easy access to html as hex dump, ftp over wifi, file management etc

             - Captive Portaling: Connecting to the access point will automatically direct you to the main page instead of typing an ip

        SD Card:
             - SD card implementation will be pretty easy and allow you to add about 1500 amiibos per MB of storage. Coming soonish maybe.

Warnings:
    
    Sometimes it can damage NTags during the initial write. This is only a problem when programming blank cards, your Amiibos will not be harmed. You can however lose cards if you aren't careful when flashing them. I recommend having them sit on the reader rather than holding them.

Known Issues:
    local.ip/edit won't load right when connected directly to the esp8266 as an acces point. Hope to fix in future

    local.ip/upload is broken. Use local.ip/edit, it's better anyway.

    Files with names longer than 30 characters will not upload, this is due to a limitation of the hardware I'm working with. There are no plans to fix this unless I can find a way to do it with an SD card library.

FAQs:
    Will this get me banned?
        Nope. Nintendo can't really tell that your amiibos are fake or altered. Amiibo cloning/spoofing is pretty widespread (just look on amazon or wish.com)

    What if I don't want an exact clone of one I already have? 
        Make an exact clone and use an actual switch or 3DS to reset the card to "new amiibo" status.  
        https://en-americas-support.nintendo.com/app/answers/detail/a_id/22344/~/how-to-reset-amiibo-data

    How do I upload amiibos to the ESPmiibo?
        After connecting the ESPmiibo to your wifi network visit 192.168.4.1/edit on your smartphone or pc to upload decryption keys and amiibo dumps in the for of .bins


Instructions:

Wiring is pretty easy

MFRC-522 Pin    ESP Pin
SDA - - - - - - D2
SCK - - - - - - D5
MOSI- - - - - - D7
MISO- - - - - - D6
GND - - - - - - GND
RST - - - - - - D3
3.3v- - - - - - 3.3v


This button is no longer required as of V1.1
However I will continue to support a physical button as buttons are the best.
Button- - - - - D8


BLUETOOTH ADAPTER WIRING
HC05/HC06       ESP PIN
TXD - - - - - - TX
RXD - - - - - - RX
VCC - - - - - - 3.3V
GND - - - - - - GND

Steps:

1 - Install the Arduino IDE:
http://www.circuitbasics.com/arduino-basics-installing-software/

2 - Install the ESP8266 core:
http://esp8266.github.io/Arduino/versions/2.0.0/doc/installing.html

Optional - Use something like this if you have an HC-05 or HC-06
https://play.google.com/store/apps/details?id=project.bluetoothterminal

This might help you configure the ide, you don't need to follow the whole guide.
https://www.instructables.com/id/Quick-Start-to-Nodemcu-ESP8266-on-Arduino-IDE/

3 - Move the contents of ESPMIIBO/libraries to C:\Users\YOU\Documents\Arduino\libraries

4 - Move the remainder of the folder to
C:\Users\YOU\Documents\Arduino 

5 - Open the Arduino IDE and go File -> Open -> ESPMIIBO -> ESPMIIBO.ino
  5.1 - In the file find the lines labeled 
    const char* ssid = "Ridley";
    const char* password = "metroids7";

    Change the text in the quotes ssid to your Wifi's Name and the text in the quotes after 'password' to the password. It is CASE SENSITIVE.

  5.2 - At this point save the sketch.

6 - After that connect the esp8266 to your pc and click the upload button, this will flash ESPMIIBO.ino to your board. 

7 - At this point you should place any Amiibo Dumps in .bin format that you want preloaded onto the cloner in C:\Users\YOU\Documents\Arduino\ESPMIIBO\data.


8 - Next in the Arduino IDE, click Tools -> ESP8266 Sketch Data Upload -> Wait for the programmer to do it's thing, it'll take a minute. After this your cloner should be functional. 

9 - To upload or manage the files on the ESPmiibo log on to a web browser on a device connected to the same wifi and go to 192.168.4.1/edit 
This will allow you to add/remove files from your ESPmiibo.

9 - To clone, flash, or dump amiibos connect the ESPmiibo to a pc with the Arduino IDE, open the IDE, open up the serial prompt (click the magnifying glass on the top right of the program), and click the restart button on the ESPmiibo to see the easy command-line interface.

10 - Unfortunately Wifi control of flashing and saving is not cupported. It is planned for future updates. For more info on what functionality is available read the table below:

Current functionality:
                            Serial                  Wifi

File upload                 (Slow, resets storage)  (Good, Web browser)
File Deletion               (Command Line)          (Good, Web browser)
File Rename                 (Command Line)          (Good, web browser)
Initiate Amiibo Flash       (Command Line)          (Yes, more work needed)
Dump Amiibo byte text       (Command Line)          (soon)
Download File from ESP      (Not Possible)          (Good, Web browser)
Cloning                     (Command Line W/ Button)(Next Release)
Bulk Ammibo production      (After Wifi features)   (Eventually)


Changelog:  
0.9 
  Command-line only, never released.

1.0
  First release. Still working out the kinks.

1.1
  Improved Site navigation
    Site now auto-directs base IP address to main.htm

    Main.htm Now has a web - based button for initiating amiibo flashes/reads
    Main.htm Now has a button that leads to the file system at /edit
    Main.htm Now has a button theat directs to /graphs (stats for nerd/stack)
    Added server.handleClient(); to a lot of loops, moved away from async

1.1b
  Added getKeyBin(); to fetch key if it's not local
  getKeyBin(); is now called when the key is missing


1.2
Changlog:
  Created system to fetch the web pages so they don't have to be flashed any more, relatively simple system 

  Begin separating out console vs debug, my vision is 3 separate features. Wifi Client, Console, and Debug.

  Added feature to spew console updates to debug, I plan to enable the console to be output to web browser as well as serial in the future, and maybe even add a full debug page. 

  Moved function listing to its own function so I can have sub-sections

  Moved main program funcctions to separate funtions for readability

  Added debug enable option, not fully implemented.

  Made stuff a bit more tidy
    Separated Wifi and Webpage Setup functions

  Made baudRate of serial connection adjustable at flash time. WARNING: I ONLY TEST WITH 9600

  Added list to handle downloading key and main files.

  Wrote flexible downloader getFile(String downloadSite,String downloadName)

  Made web code a lot more readable by separating out thei functions
  Removed old tag that said writefile() was broken

1.3
  more code readability adjustments now that it's working well
  Much of the console additions are to improve the websit's control of the process
  I am working to greatly simlify the menuing system, plan on adding a function to do it with just an array of strings and an array_len
  added "Restart" to main function list
  added restart global command to console via serialEvent()
  added serialIntercepter() to execute global console commands, I plan on implementing a universal menu system that I can port elsewhere for easy console stuff

  added definitions.h to deal with the increasing number of definitions, especially since they are going to be important for future serial menuing. 
  added support for commands 'up' and 'back' in console
  added support for command 'restart' that calls a reset in esp sdk
  added debug toggle
  added 'more utils' menu, not much there yet
  added 'toggle debug' and 'download key' to menu
  moved the file downloader to a separate file

Future Plan List:
Add Serial console outout to a webpage (thinking @ /console)
Add batch production (folder/single bulk)
Add quick-clone for browser
Add "Batch Tool" to main menu


A new secret with every full build.
Message me the translated secret for kudos.
Kudos are worth 1/100th of a dollar.
Kudos are rounded down at time of transaction to nearest dollar.
Only one Kudo redemable per transaction.

Secret 1: 
69 83 80 109 105 105 98 111 

Secret 2:
bfv ght fhyyve ar mtxk jpt ptbax ab thaxr kmp | hglzvjqfadcxytmowerbpsnuki

Secret 3:
32 20 2B 20 32 20 69 73 20 34 20 6D 69 6E 75 73 20 31 20 64 61 74 73 20 33 20 71 75 69 63 6B 20 6D 61 66 73

