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
