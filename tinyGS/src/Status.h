#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif

#ifndef Status_h
#define Status_h

struct PacketInfo {
  String time = "Waiting";
  float rssi = 0; // 무선 통신에서 수신된 신호의 세기
  float snr = 0; // 신호 대 잡음비
  float frequencyerror = 0; // Hz 
  bool crc_error = false;
};

struct ModemInfo {
  char satellite[25]  = "Waiting";
  String  modem_mode  = "LoRa" ;     // 1-LoRa  2-FSK  3-GMSK
  float   frequency   = 0; // MHz  
  float   freqOffset  = 0; // Hz 
  float   bw          = 0; // kHz dual sideban
  uint8_t sf          = 0 ; // 스프레딩 팩터
  uint8_t cr          = 0 ; // 코딩 레이트
  uint8_t sw          = 18; // 소프트웨어 버전
  int8_t  power       = 5 ;
  uint16_t preambleLength = 8;
  float  	bitrate     = 9.6 ;
  float   freqDev     = 5.0;
  uint8_t    OOK      = 0; // 0 disable  1 -> 0.3  2-> 0.5  3 -> 0.6  4-> 1.0
  bool    crc         = true;
  uint8_t fldro       = 1;
  uint8_t gain        = 0;
  uint32_t  NORAD     = 46494;  // funny this remember me WARGames
  uint8_t   fsw[8]    = {0,0,0,0,0,0,0,0};
  uint8_t   swSize    = 0;
  uint8_t   filter[8] = {0,0,0,0,0,0,0,0};
  uint8_t   len       = 64;     // FSK expected lenght in packet mode
  uint8_t   enc       = 0;      // FSK  transmission encoding. (0 -> NRZ(sx127x, sx126x)(defaul).  1 -> MANCHESTER(sx127x), WHITENING(sx126x).  2 -> WHITENING(sx127x, sx126x). 10 -> NRZ(sx127x), WHITENING(sx126x).
  float currentRssi = 0;
};

struct TextFrame {   
  uint8_t text_font;
  uint8_t text_alignment;
  int16_t text_pos_x;
  int16_t text_pos_y; 
  String text = "123456789012345678901234567890";
};

struct Status {
  const uint32_t version = 2403241; // version year month day release
  const char* git_version = GIT_VERSION;
  bool mqtt_connected = false;
  bool radio_ready = false;
  int16_t radio_error = 0;
  PacketInfo lastPacketInfo;
  ModemInfo modeminfo;
  float satPos[2] = {0, 0}; // 위성 위치(경도, 위도)
  uint8_t remoteTextFrameLength[4] = {0, 0, 0, 0};
  TextFrame remoteTextFrame[4][15];
  float time_offset = 0;
 };

#endif
