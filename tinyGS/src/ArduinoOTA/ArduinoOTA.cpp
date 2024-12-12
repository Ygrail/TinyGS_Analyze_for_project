#include "ArduinoOTA.h"
#include "../Logger/Logger.h"

void arduino_ota_setup() {
	ArduinoOTA
		.onStart([]() { // OTA 업데이트 시작 시 호출
			String type;
			if (ArduinoOTA.getCommand() == U_FLASH) 
				type = "sketch"; // 스케치(펌웨어) 업데이트
			else // U_SPIFFS
				type = "filesystem"; // 파일시스템 업데이트

			Log::console(PSTR("Start updating %s"), type.c_str());
		})
		.onEnd([]() { // OTA 업데이트 완료 시 호출
			Log::console(PSTR("End"));
		})
		.onProgress([](unsigned int progress, unsigned int total) { // 업데이트 진행률 계산 및 호출
			static uint8_t lastValue = 255; // 이전 진행률 저장, 중복 출력 방지
			uint8_t nextValue = progress / (total / 100); // 진행률(%) 계산
			if (lastValue != nextValue) { // 진행률이 변경되었을 때만 출력
				Log::debug(PSTR("Progress: %u%%\r"), nextValue);
				lastValue = nextValue; // 마지막 출력된 값을 갱신
			}
		})
		.onError([](ota_error_t error) { // OTA 과정에서 오류 발생 시 호출
			Log::debug(PSTR("Error[%u]: %u"), error);
			
			if (error == OTA_AUTH_ERROR) Log::debug(PSTR("Auth Failed"));
			else if (error == OTA_BEGIN_ERROR) Log::debug(PSTR("Begin Failed"));
			else if (error == OTA_CONNECT_ERROR) Log::debug(PSTR("Connect Failed"));
			else if (error == OTA_RECEIVE_ERROR) Log::debug(PSTR("Receive Failed"));
			else if (error == OTA_END_ERROR) Log::debug(PSTR("End Failed"));
		});

	ArduinoOTA.setHostname("TinyGS");
	ArduinoOTA.begin(); // OTA 업데이트 기능 활성화
}
