#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>

/*
//[<cards.h>示例]
byte id_cards[max_cards][4] = {
	//{0x12, 0x34, 0x56, 0x78},
	//{...},
	{0x00, 0x00, 0x00, 0x00}//结束标志四个零
};
*/
#include "cards.h"

//各种状态的定义
#define BOOL      char
#define FALSE     0
#define TRUE      1
#define drt_close 0
#define drt_open  1
#define drt_stop  2
#define swt_up    1
#define swt_down  2
#define st_wait   0
#define st_open   1
#define st_close  2
#define max_cards 20
#define err_delay 300

#define pwm_PIN  3	//电机pwm
#define up_PIN   4	//上光电开关
#define down_PIN 5	//下光电开关
#define out_PIN1 6	//电机A1
#define out_PIN2 7	//电机A2
#define bb_PIN   8	//蜂鸣器

#define RST_PIN  9
#define SS_PIN   10

MFRC522 rfid(SS_PIN, RST_PIN);

unsigned stats = st_wait;

void setup() {
	Serial.begin(9600);
	SPI.begin();
	rfid.PCD_Init();

	//各种引脚
	pinMode(pwm_PIN, OUTPUT);
	pinMode(up_PIN, INPUT);
	pinMode(down_PIN, INPUT);
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(out_PIN1, OUTPUT);
	pinMode(out_PIN2, OUTPUT);
	pinMode(bb_PIN, OUTPUT);

	//电机
	analogWrite(pwm_PIN, 255);
	digitalWrite(out_PIN1, LOW);
	digitalWrite(out_PIN2, LOW);
	PlaySound(40, 5, 40);
}

void loop() {
	//待机状态的过程
	if (stats == st_wait) {
		//读卡
		if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
			delay(err_delay);
			return;
		}
		//卡类型判断
		MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
		if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI
			&& piccType != MFRC522::PICC_TYPE_MIFARE_1K
			&& piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
			Serial.println(">Unsupport Card Type.");
			delay(err_delay);
			return;
		}
		//输出卡信息到串口
		Serial.print(">");
		printHex(rfid.uid.uidByte, rfid.uid.size);
		Serial.println();
		//判断正确卡并处理
		if (isRightCard(rfid.uid.uidByte)) {
			setOutput(drt_open);
			stats = st_open;
			PlaySound(500, 1, 0);
		}
		else {
			PlaySound(200, 5, 50);
		}
		//等待卡移开
		rfid.PICC_HaltA();
		rfid.PCD_StopCrypto1();
	}
	//正在开门的过程
	else if (stats == st_open) {
		if (!digitalRead(down_PIN)) {
			setOutput(drt_stop);
			PlaySound(50, 2, 25);
			delay(1500);
			stats = st_close;
			setOutput(drt_close);
		}
	}
	//正在关门的过程
	else if (stats == st_close) {
		if (!digitalRead(up_PIN)) {
			delay(500);
			PlaySound(50, 3, 25);
			setOutput(drt_stop);
			stats = st_wait;
		}
	}
	delay(200);
}

//判断是否是正确的卡
BOOL isRightCard(byte* id)
{
	int i = 0;
	while (i < max_cards && id_cards[i][0] != 0x00 && id_cards[i][1] != 0x00 && id_cards[i][2] != 0x00 && id_cards[i][3] != 0x00) {
		if (id_cards[i][0] == id[0] && id_cards[i][1] == id[1] && id_cards[i][2] == id[2] && id_cards[i][3] == id[3]) {
			return TRUE;
		}
		i++;
	}
	return FALSE;
}

//设置电机方向
void setOutput(int drt)
{
	switch (drt) {
	case drt_open:
		digitalWrite(out_PIN2, LOW);
		digitalWrite(out_PIN1, HIGH);
		break;
	case drt_close:
		digitalWrite(out_PIN1, LOW);
		digitalWrite(out_PIN2, HIGH);
		break;
	case drt_stop:
		digitalWrite(out_PIN1, LOW);
		digitalWrite(out_PIN2, LOW);
		break;
	}
	return;
}

//蜂鸣器
//t-单次时间
//lop-循环次数
//dly-两次的间隔
void PlaySound(int t, int lop, int dly)
{
	for (int i = 0; i < lop; i++) {
		digitalWrite(bb_PIN, LOW);
		delay(t);
		digitalWrite(bb_PIN, HIGH);
		delay(dly);
	}
}

void printHex(byte* buffer, byte bufferSize)
{
	for (byte i = 0; i < bufferSize; i++) {
		Serial.print(buffer[i] < 0x10 ? " 0x0" : " 0x");
		Serial.print(buffer[i], HEX);
	}
}
