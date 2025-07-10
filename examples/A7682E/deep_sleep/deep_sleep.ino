#include "Arduino.h"
#include "utilities.h"

bool reply = false;

/**
 * After first time power on, some register of this pin will be written then
 * it will lose shutdown function, so it is recommended to use PWRKEY to power
 * on the module and RESET key only used as reset function.
 */
void A7682E_reset(void)
{
	pinMode(BOARD_A7682E_RST, OUTPUT);
	digitalWrite(BOARD_A7682E_RST, HIGH);
	delay(100);
	digitalWrite(BOARD_A7682E_RST, LOW);
	delay(2500); // Pull down for at least 2 seconds to reset
	digitalWrite(BOARD_A7682E_RST, HIGH);
}
/**
 *
 */
void A7682E_power_on(void)
{
	digitalWrite(BOARD_A7682E_PWRKEY, LOW);
	delay(10);
	digitalWrite(BOARD_A7682E_PWRKEY, HIGH);
	delay(50);
	digitalWrite(BOARD_A7682E_PWRKEY, LOW);
	delay(10);
}
/**
 *
 */
void A7682E_power_off(void)
{
	digitalWrite(BOARD_A7682E_PWRKEY, LOW);
	delay(10);
	digitalWrite(BOARD_A7682E_PWRKEY, HIGH);
	delay(3000);
	digitalWrite(BOARD_A7682E_PWRKEY, LOW);
	delay(10);
}

void setup(void)
{
	// enable A7682 module power
	pinMode(BOARD_A7682E_POWER_EN, OUTPUT);
	digitalWrite(BOARD_A7682E_POWER_EN, HIGH);

	SerialMon.begin(115200);
	SerialAT.begin(115200, SERIAL_8N1, BOARD_A7682E_TXD, BOARD_A7682E_RXD);

	// Power on the modem PWRKEY
	pinMode(BOARD_A7682E_PWRKEY, OUTPUT);
	digitalWrite(BOARD_A7682E_PWRKEY, HIGH);

	delay(2000);

#ifdef BOARD_A7682E_RST
	// Release reset GPIO hold
	gpio_hold_dis((gpio_num_t)BOARD_A7682E_RST);

	// Set modem reset pin ,reset modem
	// The module will also be started during reset.
	Serial.println("Set Reset Pin.");
	A7682E_reset();
#endif

	// A7682  Power on
	Serial.println("Power on the modem PWRKEY.");
	A7682E_power_on();

	// UART is ready to exit from the sleep mode if DTR pin is pulled down
	pinMode(BOARD_A7682E_DTR, OUTPUT);
	digitalWrite(BOARD_A7682E_DTR, LOW);

	int i = 10;
	Serial.println("\nTesting Modem Response...\n");
	Serial.println("****");
	while (i)
	{
		SerialAT.println("AT");
		delay(500);
 		if (SerialAT.available())
		{
			String r = SerialAT.readString();
			SerialAT.println(r);
			if (r.indexOf("OK") >= 0)
			{
				reply = true;
				break;
			}
		}
		delay(500);
		i--;
	}
	Serial.println("****\n");

	if (reply)
	{
        Serial.println(F("***********************************************************"));
		Serial.println(F(" You can now send AT commands"));
		Serial.println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
		Serial.println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
		Serial.println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
		Serial.println(F(" can have undesired consiquinces..."));
		Serial.println(F("***********************************************************\n"));
	}
	else
	{
		Serial.println(F("***********************************************************"));
		Serial.println(F(" Failed to connect to the modem! Check the baud and try again."));
		Serial.println(F("***********************************************************\n"));
	}
}

void loop(void)
{
	// while (SerialAT.available())
	// {
	//     SerialMon.write(SerialAT.read());
	// }
	// while (SerialMon.available())
	// {
	//     SerialAT.write(SerialMon.read());
	// }

	// 检查是否有可读取的串口数据
	if (Serial.available() > 0)
	{
		char inputChar = Serial.read(); // 读取一个字符
		Serial.print("recvice commend:");
		Serial.println(inputChar);

		// 根据输入的字符输出不同的消息
		switch (inputChar)
		{
		case 'o':
			Serial.println("A7682 Power on");
			A7682E_power_on();
			break;
		case 'f':
			Serial.println("A7682 Power off");
			A7682E_power_off();
			break;
		case 'r':
			Serial.println("A7682 reset");
			A7682E_reset();
			break;
		default:
			Serial.println("input 'o', 'f' or 'r'");
			break;
		}
	}
}