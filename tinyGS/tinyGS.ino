#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "src/ConfigManager/ConfigManager.h"
#include "src/Display/Display.h"
#include "src/Mqtt/MQTT_Client.h"
#include "src/Status.h"
#include "src/Radio/Radio.h"
#include "src/ArduinoOTA/ArduinoOTA.h"
#include "src/OTA/OTA.h"
#include "src/Logger/Logger.h"
#include "time.h"

#if RADIOLIB_VERSION_MAJOR != (0x06) || RADIOLIB_VERSION_MINOR != (0x04) || RADIOLIB_VERSION_PATCH != (0x00) || RADIOLIB_VERSION_EXTRA != (0x00)
#error "You are not using the correct version of RadioLib please copy TinyGS/lib/RadioLib on Arduino/libraries"
#endif

#ifndef RADIOLIB_GODMODE
#if !PLATFORMIO
#error "Using Arduino IDE is not recommended, please follow this guide https://github.com/G4lile0/tinyGS/wiki/Arduino-IDE or edit /RadioLib/src/BuildOpt.h and uncomment #define RADIOLIB_GODMODE around line 367"
#endif
#endif

ConfigManager& configManager = ConfigManager::getInstance();
MQTT_Client& mqtt = MQTT_Client::getInstance();
Radio& radio = Radio::getInstance();

const char* ntpServer = "time.cloudflare.com"; // NTP 서버 주소

Status status;

void printControls();
void switchTestmode();
void checkButton();
void setupNTP();

void configured()
{
  configManager.setConfiguredCallback(NULL); // 설정 완료 시 콜백 해제
  configManager.printConfig();
  radio.init(); // 무선 통신 초기화
}

void wifiConnected()
{
  configManager.setWifiConnectionCallback(NULL); // WiFi 연결 시 콜백 해제
  setupNTP(); // NTP 동기화 설정
  displayShowConnected(); // 디스플레이에 연결 상태 표시
  arduino_ota_setup(); // OTA 업데이트 준비
  configManager.delay(100); // 애니메이션 완료 대기

  if (configManager.getLowPower()) // 저전력 모드 설정 확인
  {
    Log::debug(PSTR("Set low power CPU=80Mhz"));
    setCpuFrequencyMhz(80); // CPU 주파수를 80MHz로 설정
  }

  configManager.delay(400); // 화면 안정화 대기
}

void setup()
{ 
#if CONFIG_IDF_TARGET_ESP32C3
  setCpuFrequencyMhz(160); // ESP32-C3의 경우 CPU 주파수를 160MHz로 설정
#else
  setCpuFrequencyMhz(240); // 기본적으로 CPU 주파수를 240MHz로 설정
#endif
  Serial.begin(115200); // 시리얼 통신 시작
  delay(100);
  Log::console(PSTR("TinyGS Version %d - %s"), status.version, status.git_version);
  Log::console(PSTR("Chip  %s - %d"), ESP.getChipModel(), ESP.getChipRevision());
  configManager.setWifiConnectionCallback(wifiConnected); // WiFi 연결 시 콜백 등록
  configManager.setConfiguredCallback(configured); // 설정 완료 시 콜백 등록
  configManager.init(); // 설정 관리자 초기화

  if (configManager.isFailSafeActive()) // 복구 모드 활성화 확인
  {
    Log::console(PSTR("FATAL ERROR: The board is in a boot loop, rescue mode launched. Connect to the WiFi AP: %s, and open a web browser on ip 192.168.4.1 to fix your configuration problem or upload a new firmware."), configManager.getThingName());
    return;
  }

  configManager.doLoop(); // 설정 관리 루프 호출
  board_t board;
  if (configManager.getBoardConfig(board)) // 보드 구성 확인
    pinMode(board.PROG__BUTTON, INPUT_PULLUP); // 프로그래밍 버튼 핀 설정
  displayInit(); // 디스플레이 초기화
  displayShowInitialCredits(); // 초기 크레딧 표시
  configManager.delay(1000);
  mqtt.begin(); // MQTT 클라이언트 초기화

  if (configManager.getOledBright() == 0) // 디스플레이 밝기 설정 확인
  {
    displayTurnOff();
  }
  
  printControls();
}

void loop()
{  
  configManager.doLoop(); // 설정 관리자 루프 호출
  if (configManager.isFailSafeActive()) // 복구 모드 활성화 확인
  {
    static bool updateAttepted = false;
    if (!updateAttepted && configManager.isConnected()) {
      updateAttepted = true;
      OTA::update(); // OTA 업데이트 시도
    }

    if (millis() > 10000 || updateAttepted)
      configManager.forceApMode(true); // AP 모드 강제 전환
    
    return;
  }

  ArduinoOTA.handle(); // OTA 요청 처리
  handleSerial(); // 시리얼 명령 처리

  if (configManager.getState() < 2) // 설정 준비되지 않은 상태
  {
    displayShowApMode(); // AP 모드 화면 표시
    return;
  }

  checkButton();
  if (radio.isReady()) // 무선 통신 준비 상태 확인
  {
    status.radio_ready = true; // 무선 통신 준비 완료
    radio.listen(); // 무선 신호 수신
  }
  else {
    status.radio_ready = false;
  }

  if (configManager.getState() < 4) // 연결 또는 AP 모드 상태
  {
    displayShowStaMode(configManager.isApMode()); // STA 또는 AP 모드 화면 표시
    return;
  }

  mqtt.loop();
  OTA::loop();
  if (configManager.getOledBright() != 0) displayUpdate(); // 디스플레이 업데이트
}

void setupNTP()
{
  configTime(0, 0, ntpServer); // NTP 서버 설정
  setenv("TZ", configManager.getTZ(), 1); // 시간대 설정
  tzset(); // 시간대 적용
  
  configManager.delay(1000);
}

void checkButton()
{
  #define RESET_BUTTON_TIME 8000
  static unsigned long buttPressedStart = 0; // 버튼이 눌린 시간을 기록하기 위한 변수
  board_t board;
  
  if (configManager.getBoardConfig(board) && !digitalRead(board.PROG__BUTTON)) // 버튼이 눌린 상태 확인
  {
    if (!buttPressedStart) // 버튼이 처음 눌린 경우
    {
      buttPressedStart = millis(); // 현재 시간을 기록
    }
    else if (millis() - buttPressedStart > RESET_BUTTON_TIME) // 버튼이 길게 눌린 경우
    {
      Log::console(PSTR("Rescue mode forced by button long press!"));
      Log::console(PSTR("Connect to the WiFi AP: %s and open a web browser on ip 192.168.4.1 to configure your station and manually reboot when you finish."), configManager.getThingName());
      configManager.forceDefaultPassword(true); // 기본 비밀번호 강제 설정
      configManager.forceApMode(true); // AP 모드 강제 설정
      buttPressedStart = 0; // 버튼 상태 초기화
    }
  }
  else {
    unsigned long elapsedTime = millis() - buttPressedStart;
    if (elapsedTime > 30 && elapsedTime < 1000) // 버튼이 짧게 눌린 경우
      displayNextFrame(); // 디스플레이 프레임 전환
    buttPressedStart = 0; // 버튼 상태 초기화
  }
}

void handleSerial()
{
  if (Serial.available()) // 시리얼 입력 확인
  {
    radio.disableInterrupt(); // 무선 통신 인터럽트 비활성화

    char serialCmd1 = Serial.read(); // 첫 번째 명령어 읽기
    char serialCmd = ' ';

    configManager.delay(50); // 추가 입력을 기다림
    if (serialCmd1 == '!') serialCmd = Serial.read(); // 두 번째 명령어 읽기
    while (Serial.available()) // 시리얼 버퍼 비우기
    {
      Serial.read();
    }

    switch (serialCmd) {
      case ' ': // 공백일 경우 아무 작업도 하지 않음
        break;
      case 'e': // 설정 초기화 및 재부팅
        configManager.resetAllConfig();
        ESP.restart();
        break;
      case 'b': // 재부팅
        ESP.restart();
        break;
      case 'p': // 테스트 패킷 전송
        if (!configManager.getAllowTx())
        {
          Log::console(PSTR("Radio transmission is not allowed by config! Check your config on the web panel and make sure transmission is allowed by local regulations"));
          break;
        }

        static long lastTestPacketTime = 0;
        if (millis() - lastTestPacketTime < 20 * 1000) // 전송 간격 확인
        {
          Log::console(PSTR("Please wait a few seconds to send another test packet."));
          break;
        }
        
        radio.sendTestPacket(); // 테스트 패킷 전송
        lastTestPacketTime = millis(); // 마지막 전송 시간 업데이트
        Log::console(PSTR("Sending test packet to nearby stations!"));
        break;
      default:
        Log::console(PSTR("Unknown command: %c"), serialCmd);
        break;
    }

    radio.enableInterrupt(); // 무선 통신 인터럽트 활성화
  }
}

void printControls()
{
  Log::console(PSTR("------------- Controls -------------"));
  Log::console(PSTR("!e - erase board config and reset"));
  Log::console(PSTR("!b - reboot the board"));
  Log::console(PSTR("!p - send test packet to nearby stations (to check transmission)"));
  Log::console(PSTR("------------------------------------"));
}
