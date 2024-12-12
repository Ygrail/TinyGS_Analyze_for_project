#include "OTA.h"
#include "../ConfigManager/ConfigManager.h"
#include "../Status.h"
#include "../Logger/Logger.h"

extern Status status;
bool usingNewCert = true;
const long MIN_TIME_BEFORE_UPDATE = random(60000, 20*60*1000); // 업데이트 전에 대기할 최소 시간

void OTA::update()
{
#ifdef SECURE_OTA // 보안 OTA 사용 여부에 따라 분기
  WiFiClientSecure client; // 보안 클라이언트 사용
  if (usingNewCert)
    client.setCACert(newRoot_CA); // 새 루트 인증서 설정
  else
    client.setCACert(DSTroot_CA); // 대체 루트 인증서 설정
#else
  WiFiClient client; // 일반 클라이언트 사용
#endif

  uint64_t chipId = ESP.getEfuseMac(); // 칩 ID 가져오기
  char clientId[13];
  sprintf(clientId, "%04X%08X", (uint16_t)(chipId>>32), (uint32_t)chipId); // 칩 ID를 클라이언트 ID로 변환

  ConfigManager& c = ConfigManager::getInstance();
  char url[255];
  sprintf_P(url, PSTR("%s?user=%s&name=%s&mac=%s&version=%d&rescue=%s"), OTA_URL, c.getMqttUser(), c.getThingName(), clientId, status.version, (c.isFailSafeActive()?"true":"false")); 
  // 업데이트 서버 URL 생성, 디바이스 정보 포함

  Log::debug(PSTR("Checking for firmware Updates...  "));
  t_httpUpdate_return ret = httpUpdate.update(client, url, status.git_version); // HTTP 업데이트 실행

  switch (ret) { // 업데이트 결과에 따라 처리
    case HTTP_UPDATE_FAILED: // 업데이트 실패
      usingNewCert = !usingNewCert; // 인증서 교체 시도
      Log::info(PSTR("Update failed Error (%d): %s\n"), httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES: // 업데이트 없음 (서버가 304 응답)
      Log::info(PSTR("No updates required"));
      break;

    case HTTP_UPDATE_OK: // 업데이트 성공 (ESP가 재시작해야 함)
      Log::info(PSTR("Update ok but ESP has not restarted!!! (This should never be printed)"));
      break;
  }
}

unsigned static long lastUpdateTime = 0; // 마지막 업데이트 확인 시간 저장

void OTA::loop()
{
  if (millis() < MIN_TIME_BEFORE_UPDATE) // 초기 대기 시간 확인
    return;

  if (millis() - lastUpdateTime > TIME_BETTWEN_UPDATE_CHECK) // 업데이트 확인 주기 경과 여부 확인
  {
    lastUpdateTime = millis(); // 마지막 업데이트 시간 갱신
    update(); // 업데이트 확인 수행
  }
}
