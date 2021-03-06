/*----------------------------------
thanks to IsO-Mick for the dynamic filename handling
http://pastebin.com/gT3QM6jH

equiv vars left is IsO-Mick's right is mmmaxwwwell's
m_CurrentFilename = filename
m_FileIndex = imageSel
-----------------------------------*/
#include <SD.h>
#include <Adafruit_GFX.h>   // Core graphics library
#include <avr/pgmspace.h>
#include "LPD8806.h"
#include "SPI.h"
#define numBmpPixels 32
#define _width numBmpPixels
#define _height numBmpPixels



int m_NumberOfFiles;
String m_FileNames[25];
String m_CurrentFilename;


//prog_uint32_t imageArray[_width][_height];
uint8_t modeSel, imageSel;

//LPD8806 strip = LPD8806(numPixels);
uint32_t imageArray[numBmpPixels][numBmpPixels];
LPD8806 strip = LPD8806(numBmpPixels);

void (*Mode[])(byte) = {
  bmpSave,
  displayBmp,
  wait
};



void GetFileNamesFromSD(File dir)
{
  int fileCount = 0;
  String CurrentFilename = "";
  while(1)
  {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      m_NumberOfFiles = fileCount;
      break;
    }
    else
    {
      if (entry.isDirectory()) {
        //GetNextFileName(root);
      }
      else {
        CurrentFilename = entry.name();
        if (CurrentFilename.endsWith(".bmp") || CurrentFilename.endsWith(".BMP") )//find files with our extension only
        {
          m_FileNames[fileCount] = entry.name();
          fileCount++;
        }
      }
    }
  }
}

void isort(String *filenames, int n)
{
  for (int i = 1; i < n; ++i)
  {
    String j = filenames[i];
    int k;
    for (k = i - 1; (k >= 0) && (j < filenames[k]); k--)
    {
      filenames[k + 1] = filenames[k];
    }
    filenames[k + 1] = j;
  }
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);


  while(!Serial.available()){
  }


  Serial.println("init serial");
  strip.begin(); 
  strip.show();
  Serial.println("init strip");
  // delay(10);
  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  pinMode(10, OUTPUT);

  if (!SD.begin(10)) {
    Serial.println(" SD initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  //dynamic filename handling
  File root = SD.open("/");
  GetFileNamesFromSD(root);//look
  isort(m_FileNames, m_NumberOfFiles);//rearrange
  m_CurrentFilename = m_FileNames[0];//set

  // attachInterrupt(19, buttonpress, LOW); //button 19 external interrupt 7

}

void loop() {

  (*Mode[modeSel])(imageSel);

}

void wait(byte ignored){
}


uint8_t displayPos;
void displayBmp(byte ignored){
  for(uint8_t i=0; i<_height; i++){

    strip.setPixelColor(i,imageArray[displayPos][i]);
  }
  displayPos=(displayPos >= _width)?
  displayPos+1:
  0;
  strip.show();
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.
// BTH - this code adopted from the AdaFruit ST7735 spitftbitmap example 7.16.2013

#define BUFFPIXEL 20



void bmpSave(byte sel) {
  Serial.print("start bmpSave");
  /*
  ////digitalWrite(SS, LOW);
  char filename[8] = "XXX.bmp";//need to set to make sure filename is big enough for the string to char* conversion
  //otherwise we have an overrun and crash
  Serial.print("b1");
  String filenamestr = String(sel);
  Serial.print("b2");
  filenamestr += ".bmp";
  Serial.print("b3");
  filenamestr.toCharArray(filename, 8);
  Serial.print("b4");
  //for(int i = 0; i<sizeof(filenamestr);i++){
  //  Serial.print("b4");
  // filename[i] = filenamestr.charAt(i);
  //  Serial.print("b5");
  // }  
  */
  char* filename;
  m_FileNames[sel].toCharArray(filename, m_FileNames[sel].length());
  Serial.print("!");
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint8_t  x, y;
  uint32_t pos = 0, startTime = millis();

  if((x >= width()) || (y >= height())) return;

  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print("File not found");
    modeSel = 2;// wait
    //  digitalWrite(SS, HIGH);//enable LED strip
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); 
    Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print("Image Offset: "); 
    Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); 
    Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print("Bit Depth: "); 
      Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print("Image size: ");
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= width())  w = width()  - x;
        if((y+h-1) >= height()) h = height() - y;

        // Set TFT address window to clipped image bounds
        //bth tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
          pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            //   b = sdbuffer[buffidx++];
            //   g = sdbuffer[buffidx++];
            //   r = sdbuffer[buffidx++];
            // bth  tft.pushColor(tft.Color565(r,g,b));
            savePixel(col,row,sdbuffer[buffidx++],sdbuffer[buffidx++],sdbuffer[buffidx++]);
            //savePixel(col,row,r,g,b);    

          } // end pixel
        } // end scanline
        Serial.print("Loaded in ");
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) {
    Serial.println("BMP format not recognized.");
  } 
  else {
  } 
  modeSel=1;
  Serial.print("Starting display mode");
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

int16_t width(void) { 
  return _width; 
}

int16_t  height(void) { 
  return _height; 
}

void savePixel(uint8_t x, uint8_t y, uint8_t b, uint8_t g, uint8_t r){
  imageArray[x][y] = (r << 16) | (g << 8) | b ;
  Serial.print("x:");
  Serial.print(x);
  Serial.print(", y:");
  Serial.print(y);
  Serial.print(",hex:");
  Serial.println(imageArray[x][y],HEX);//
}

unsigned long lastDebounceTime;
uint8_t debounceDelay = 200;
void buttonpress(){
  lastDebounceTime = millis();
  if ((millis() - lastDebounceTime) > debounceDelay) {
    imageSel++;

  }

}

