void audioSetup(){
  dac1.analogReference(EXTERNAL); // Increase amp volume baseline
  pinMode(PROP_AMP_ENABLE, OUTPUT); //Turn on Prop Shield Amp
  digitalWrite(PROP_AMP_ENABLE, HIGH);
  AudioMemory(12);
  mixer1.gain(0, 1.0f);
    if (!SerialFlash.begin(FLASH_CHIP_SELECT)) {
      while (1)
      {
        Serial.println ("Cannot access SPI Flash chip");
        delay (1000);
      } 
    }
  }

  //Upload your own files with TeensyTransfer: https://github.com/FrankBoesing/TeensyTransfer
const char * strikelist[9] = {  
  "s1.raw", "s2.raw", "s3.raw", "s4.raw", "s5.raw", "s6.raw","s7.raw", "s8.raw", "s9.raw"
};
const char * breaklist[13] = {  
  "b1.raw", "b2.raw", "s3.raw", "b4.raw", "b5.raw", "b6.raw", "b7.raw", "b8.raw", "b9.raw", "b10.raw", "b11.raw", "b12.raw", "b13.raw"
};
const char * misslist[5] = {  
  "m1.raw", "m2.raw", "m3.raw", "m4.raw", "m5.raw"
};
const char * creaklist[5] = {  
  "c1.raw", "c2.raw", "c3.raw", "c4.raw", "c5.raw",
};
const char * rewardlist[3] = {  
  "r1.raw", "r2.raw", "r3.raw",
};



void playFromArray(int n) {
  if (playFlashRaw1.isPlaying() == false) {
    static int filenumber = 0;
    static const char *filename = strikelist[0];
    switch (n){
      case 0:
        filename = strikelist[random(0, 9)];
        mixer1.gain(0, 1.0f);
        break;
      case 1:
        filename = breaklist[random(0, 13)];
        mixer1.gain(0, 1.0f);
        break;
      case 2:
        filename = misslist[random(0, 5)];
        mixer1.gain(0, 1.0f);
        break;
      case 3:
        filename = creaklist[random(0, 5)];
        mixer1.gain(0, 2.0f);
        break;
      case 4:
        filename = rewardlist[(filenumber%3)];
        mixer1.gain(0, 1.0f);
        filenumber++;
        break;
    }
    Serial.print("Start playing ");
    Serial.println(filename);
    playFlashRaw1.play(filename);
    delay(5); // wait for library to parse WAV info
   }
}


