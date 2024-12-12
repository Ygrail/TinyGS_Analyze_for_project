#include "Power.h"
#include "../Logger/Logger.h"

// PMU(전원 관리 장치) 상태를 저장하는 변수들
byte AXPchip = 0;
byte pmustat1;
byte pmustat2;
byte pwronsta;
byte pwrofsta;
byte irqstat0;
byte irqstat1;
byte irqstat2;

// I2C 장치의 특정 레지스터에 바이트 데이터를 쓰는 함수
void Power::I2CwriteByte(uint8_t Address, uint8_t Register, uint8_t Data)
{
  Wire.beginTransmission(Address);  // 장치와의 통신 시작
  Wire.write(Register);             // 데이터를 쓸 레지스터 지정
  Wire.write(Data);                 // 레지스터에 쓸 데이터
  Wire.endTransmission();           // 통신 종료
}

// I2C 장치에서 특정 레지스터의 바이트 데이터를 읽는 함수
uint8_t Power::I2CreadByte(uint8_t Address, uint8_t Register)
{
  uint8_t Nbytes = 1;  // 읽을 바이트 수
  Wire.beginTransmission(Address);  // 통신 시작
  Wire.write(Register);             // 읽을 레지스터 지정
  Wire.endTransmission();           // 통신 종료
  Wire.requestFrom(Address, Nbytes);  // 장치로부터 1바이트 요청
  byte slaveByte = Wire.read();       // 읽은 바이트 저장
  Wire.endTransmission();             // 통신 종료
  return slaveByte;                   // 읽은 바이트 반환
}

// I2C 장치에서 여러 바이트를 읽는 함수
void Power::I2Cread(uint8_t Address, uint8_t Register, uint8_t Nbytes, uint8_t* Data)
{
  Wire.beginTransmission(Address);  // 통신 시작
  Wire.write(Register);             // 읽을 레지스터 지정
  Wire.endTransmission();           // 통신 종료
  Wire.requestFrom(Address, Nbytes); // 여러 바이트 요청
  uint8_t index = 0;
  while (Wire.available())          // 사용할 수 있는 바이트가 있을 때
    Data[index++] = Wire.read();
}

// AXP 전원 관리 칩이 정상적으로 연결되어 있는지 확인하는 함수
void Power::checkAXP() 
{ 
   board_t board;
   if (!ConfigManager::getInstance().getBoardConfig(board))
    return;
  
  Log::console(PSTR("AXPxxx chip?"));  // AXP 칩 확인 로그 출력
  
  byte regV = 0;
  Wire.begin(board.OLED__SDA, board.OLED__SCL); // 지정된 SDA, SCL 핀으로 I2C 초기화
  
  byte ChipID = I2CreadByte(0x34, 0x03);  // AXP 칩 ID 읽기
  
  // AXP192 칩 (T-Beam V1.1) 확인
  if (ChipID == XPOWERS_AXP192_CHIP_ID) { 
    AXPchip = 1;  // AXP192 칩이 발견되었음을 표시
    Log::console(PSTR("AXP192 found"));  // AXP192 발견 로그 출력
    
    // AXP192 전원 채널 설정
    I2CwriteByte(0x34, 0x28, 0xFF);       // LDO2(LoRa) 및 LDO3(GPS)를 3.3V로 설정
    regV = I2CreadByte(0x34, 0x12);       // 전원 출력 제어 레지스터 읽기
    regV = regV | 0x0C;                   // LDO2 및 LDO3 활성화
    I2CwriteByte(0x34, 0x12, regV);       // 레지스터 업데이트
    
    // ADC 샘플링 속도 및 다른 설정
    I2CwriteByte(0x34, 0x84, 0b11000010); // ADC 샘플링 속도 200Hz 설정
    I2CwriteByte(0x34, 0x82, 0xFF);       // ADC 채널 활성화
    I2CwriteByte(0x34, 0x83, 0x80);       // 모든 ADC 채널 활성화
    I2CwriteByte(0x34, 0x33, 0xC3);       // 배터리 충전 전압 4.2V로 설정
    I2CwriteByte(0x34, 0x36, 0x0C);       // 전원 켜기 및 끄기 타이밍 설정
    I2CwriteByte(0x34, 0x30, 0x80);       // VBUS 제한 비활성화
    I2CwriteByte(0x34, 0x39, 0xFC);       // TS 보호 비활성화
    I2CwriteByte(0x34, 0x32, 0x46);       // CHGLED 충전 기능으로 제어
    
    // PMU 및 IRQ 상태 읽기 및 로그 출력
    pmustat1 = I2CreadByte(0x34, 0x00); 
    pmustat2 = I2CreadByte(0x34, 0x01);
    irqstat0 = I2CreadByte(0x34, 0x44); 
    irqstat1 = I2CreadByte(0x34, 0x45); 
    irqstat2 = I2CreadByte(0x34, 0x46);
    
    Log::console(PSTR("PMU status1,status2 : %02X,%02X"), pmustat1, pmustat2); 
    Log::console(PSTR("IRQ status 1,2,3    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2); 
  }

  // AXP2101 칩 (T-Beam V1.2) 확인
  if (ChipID == XPOWERS_AXP2101_CHIP_ID) { 
    AXPchip = 2;  // AXP2101 칩이 발견되었음을 표시
    Log::console(PSTR("AXP2101 found"));  // AXP2101 발견 로그 출력
    
    // AXP2101 전원 채널 설정
    I2CwriteByte(0x34, 0x93, 0x1C);       // ALDO2 전압을 3.3V로 설정 (LoRa용)
    I2CwriteByte(0x34, 0x94, 0x1C);       // ALDO3 전압을 3.3V로 설정 (GPS용)
    I2CwriteByte(0x34, 0x6A, 0x04);       // 버튼 배터리 전압 설정
    I2CwriteByte(0x34, 0x64, 0x03);       // 주 배터리 전압 설정
    I2CwriteByte(0x34, 0x61, 0x05);       // 주 배터리 사전 충전 전류 설정
    I2CwriteByte(0x34, 0x62, 0x0A);       // 주 배터리 충전 전류 설정
    I2CwriteByte(0x34, 0x63, 0x15);       // 주 배터리 충전 종료 전류 설정
    
    // 전원 채널 활성화 및 배터리 충전
    regV = I2CreadByte(0x34, 0x90);       
    regV = regV | 0x06;                   // ALDO2 및 ALDO3 활성화
    I2CwriteByte(0x34, 0x90, regV);       // 레지스터 업데이트
    
    // 두 배터리 모두 충전 기능 활성화
    regV = I2CreadByte(0x34, 0x18);       
    regV = regV | 0x06;                   // 두 배터리에 대해 비트 설정
    I2CwriteByte(0x34, 0x18, regV);       // 배터리 충전 활성화
    
    // 시스템 전압 한계 설정 및 기타 파라미터
    I2CwriteByte(0x34, 0x14, 0x30);       // 최소 시스템 전압 4.4V 설정
    I2CwriteByte(0x34, 0x15, 0x05);       // 입력 전압 한계 설정
    I2CwriteByte(0x34, 0x24, 0x06);       // Vsys 전압 한계 설정
    
    // TS 핀을 외부 입력으로 설정 (온도 센싱 아님)
    I2CwriteByte(0x34, 0x50, 0x14);       // TS 핀을 외부 입력으로 설정
    
    // 전원 상태 읽기 및 로그 출력
    pmustat1 = I2CreadByte(0x34, 0x00); 
    pmustat2 = I2CreadByte(0x34, 0x01);
    pwronsta = I2CreadByte(0x34, 0x20); 
    pwrofsta = I2CreadByte(0x34, 0x21);
    irqstat0 = I2CreadByte(0x34, 0x48); 
    irqstat1 = I2CreadByte(0x34, 0x49); 
    irqstat2 = I2CreadByte(0x34, 0x4A);
    
    Log::console(PSTR("PMU status1,status2 : %02X,%02X"), pmustat1, pmustat2);
    Log::console(PSTR("PWRON,PWROFF status : %02X,%02X"), pwronsta, pwrofsta);
    Log::console(PSTR("IRQ status 0,1,2    : %02X,%02X,%02X"), irqstat0, irqstat1, irqstat2);
  }

  Wire.end();  // I2C 통신 종료
}
