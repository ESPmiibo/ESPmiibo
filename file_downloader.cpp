/*
  .______ _____ _      ______      _____   ______          ___   _ _      ____          _____  ______ _____
  |  ____|_   _| |    |  ____|    |  __ \ / __ \ \        / / \ | | |    / __ \   /\   |  __ \|  ____|  __ \
  | |__    | | | |    | |__       | |  | | |  | \ \  /\  / /|  \| | |   | |  | | /  \  | |  | | |__  | |__) |
  |  __|   | | | |    |  __|      | |  | | |  | |\ \/  \/ / | . ` | |   | |  | |/ /\ \ | |  | |  __| |  _  /
  | |     _| |_| |____| |____     | |__| | |__| | \  /\  /  | |\  | |___| |__| / ____ \| |__| | |____| | \ \
  |_|    |_____|______|______|    |_____/ \____/   \/  \/   |_| \_|______\____/_/    \_\_____/|______|_|  \_\
*/
/*
  My hacky initial file downloader.
  #define URLSTRING                   0
  #define FILESTRING                  1
  sitesToDownload[NUM_FILES_TO_DOWNLOAD][2] structured as:
  [URL 1      ][URL 2      ][URL etc... ]
  [File Name 1][File Name 2][File Name 3]
*/
void handleDownloadFiles(const String pairArray[][2], int arraySize) {
  debug("Downloading any missing files.");
  for (int pair = 0; pair < arraySize; pair++) {
    if (!SPIFFS.exists(pairArray[pair][FILESTRING])) {                                 //if the file is missing
      debug("Missing " + pairArray[pair][FILESTRING] + " Downloading...");
      getFile(pairArray[pair][URLSTRING], pairArray[pair][FILESTRING]);          //download it
    }
  }
}


//supply this with a web address and a file name and it will download the file to that name. CAREFUL WITH THIS
void getFile(String downloadSite, String downloadName) {
  HTTPClient http;
  debug("Getting file from: " + downloadSite);
  fs::File f = SPIFFS.open(downloadName, "w");
  if (f) {
    http.begin(downloadSite);
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        Serial.println(F("Writing file..."));
        http.writeToStream(&f);
      }
    } else {
    }
    f.close();
  }
  http.end();
}

void getKeyBin() {
  HTTPClient http;
  String url = keyfilesite;     //Go to static key location
  String file_name = String(keyfilename);                                   //Name it to the normal keyfilename
  Serial.print("Getting key from: "); Serial.println(url);
  fs::File f = SPIFFS.open(file_name, "w");
  if (f) {
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        Serial.println(F("Writing file..."));
        http.writeToStream(&f);
      }
    } else {
    }
    f.close();
  }
  http.end();
}
