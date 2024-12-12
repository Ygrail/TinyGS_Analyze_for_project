#include "Logger.h"
#include "time.h"

// Static members initialization
char Log::logIdx = 1;
Log::LoggingLevels Log::logLevel = LOG_LEVEL;
char Log::log[MAX_LOG_SIZE] = "";

// 콘솔 출력 함수 (로그 레벨에 상관없이 출력)
void Log::console(const char* formatP, ...)
{
  va_list arg;
  char buffer[256];
  va_start(arg, formatP); // 가변 인자 리스트 초기화
  vsnprintf_P(buffer, sizeof(buffer), formatP, arg); // 형식에 맞게 문자열을 buffer에 저장
  va_end(arg); // 인자 리스트 종료
  AddLog(LOG_LEVEL_NONE, buffer); // 로그 추가
}

// 오류 메시지 로그 출력 함수
void Log::error(const char* formatP, ...)
{
  va_list arg;
  char buffer[256];
  va_start(arg, formatP);
  vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
  va_end(arg);
  AddLog(LOG_LEVEL_ERROR, buffer); // ERROR 레벨로 로그 추가
}

// 정보 메시지 로그 출력 함수
void Log::info(const char* formatP, ...)
{
  va_list arg;
  char buffer[256];
  va_start(arg, formatP);
  vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
  va_end(arg);
  AddLog(LOG_LEVEL_INFO, buffer); // INFO 레벨로 로그 추가
}

// 디버그 메시지 로그 출력 함수
void Log::debug(const char* formatP, ...)
{
  va_list arg;
  char buffer[256];
  va_start(arg, formatP);
  vsnprintf_P(buffer, sizeof(buffer), formatP, arg);
  va_end(arg);
  AddLog(LOG_LEVEL_DEBUG, buffer); // DEBUG 레벨로 로그 추가
}

// 로그 추가 함수 (레벨에 맞춰 로그를 추가하고 시간도 기록)
void Log::AddLog(Log::LoggingLevels level, const char* logData)
{
  if (level > Log::logLevel) // 로그 레벨이 설정된 레벨보다 높으면 로그 추가 안 함
    return;

  char timeStr[10];  // "00:00:00" 형태의 시간 문자열을 위한 배열
  time_t currentTime = time (NULL); // 현재 시간 가져오기
  if (currentTime > 0) {
      struct tm *timeinfo = localtime (&currentTime); // 로컬 시간 정보로 변환
      snprintf_P (timeStr, sizeof (timeStr), "%02d:%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  }
  else {
      timeStr[0] = '\0'; // 시간 가져오기에 실패하면 빈 문자열 처리
  }

  Serial.printf (PSTR ("%s%s\n"), timeStr, logData); // 시간과 로그 데이터를 콘솔에 출력

  // 로그 배열에 새로운 로그를 추가할 수 있는 공간을 확보
  while (logIdx == log[0] ||  // 만약 새로운 로그 인덱스가 이미 존재한다면
          strlen(log) + strlen(logData) + 13 > MAX_LOG_SIZE)  // 로그 버퍼가 꽉 찼을 때
  {
    char* it = log;
    it++;  // 로그 인덱스를 건너뛰고
    it += strchrspn(it, '\1');  // 로그 데이터 부분으로 이동
    it++;  // 종료 구분자 "\1" 건너뛰기
    memmove(log, it, MAX_LOG_SIZE -(it-log));  // 로그를 앞으로 당겨서 오래된 로그 삭제
  }

  // 새로운 로그를 로그 배열에 추가
  snprintf_P(log, sizeof(log), PSTR("%s%c%s%s\1"), log, logIdx++, timeStr, logData);
  logIdx &= 0xFF; // 인덱스를 0~255 범위로 제한
  if (!logIdx) 
    logIdx++; // 인덱스 0은 허용하지 않음
}

// 특정 인덱스의 로그 항목을 가져오는 함수
void Log::getLog(uint32_t idx, char** entry_pp, size_t* len_p)
{
  char* entry_p = nullptr;
  size_t len = 0;

  if (idx) {
    char* it = log;
    do {
      uint32_t cur_idx = *it;  // 현재 로그 인덱스 가져오기
      it++;  // 인덱스 건너뛰기
      size_t tmp = strchrspn(it, '\1');  // 종료 문자 '\1'까지 이동
      tmp++;  // 종료 문자 포함
      if (cur_idx == idx) {  // 원하는 인덱스 찾았으면
        len = tmp;  // 로그 길이 저장
        entry_p = it;  // 로그 데이터 시작 위치 저장
        break;
      }
      it += tmp;
    } while (it < log + MAX_LOG_SIZE && *it != '\0');  // 로그 끝까지 탐색
  }
  *entry_pp = entry_p;
  *len_p = len;
}

// 문자열에서 특정 문자가 나타날 때까지의 길이를 계산하는 함수
size_t Log::strchrspn(const char *str1, int character)
{
  size_t ret = 0;
  char *start = (char*)str1;
  char *end = strchr(str1, character);  // 문자 찾기
  if (end) ret = end - start;  // 문자가 있으면 그 위치까지의 길이 반환
  return ret;
}

// 현재 로그 인덱스를 반환하는 함수
char Log::getLogIdx()
{
  return logIdx;
}

// 로그 레벨을 설정하는 함수
void Log::setLogLevel(LoggingLevels level)
{
  logLevel = level;
}
