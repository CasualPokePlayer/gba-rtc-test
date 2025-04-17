// a lot of code is #if false / commented out due to it being old testing code

#include <stdio.h>
#include <string.h>
#include <gba.h>

#define GPIO_PORT_DATA *(vu16*)(0x80000C4 + 0)
#define GPIO_PORT_DIR *(vu16*)(0x80000C4 + 2)
#define GPIO_PORT_CTRL *(vu16*)(0x80000C4 + 4)

#define GPIO_WO_MODE 0
#define GPIO_RW_MODE 1

#define GPIO_RTC_SCK_SHIFT 0
#define GPIO_RTC_SIO_SHIFT 1
#define GPIO_RTC_CS_SHIFT 2
#define GPIO_RTC_UNUSED_SHIFT 3

#define GPIO_RTC_SCK (1 << GPIO_RTC_SCK_SHIFT)
#define GPIO_RTC_SIO (1 << GPIO_RTC_SIO_SHIFT)
#define GPIO_RTC_CS (1 << GPIO_RTC_CS_SHIFT)
#define GPIO_RTC_UNUSED (1 << GPIO_RTC_UNUSED_SHIFT)

#define RTC_CMD(n) (0x60 | (n << 1))

#define RTC_CMD_RESET RTC_CMD(0)
#define RTC_CMD_STATUS RTC_CMD(1)
#define RTC_CMD_DATETIME RTC_CMD(2)
#define RTC_CMD_TIME RTC_CMD(3)
#define RTC_CMD_ALARM RTC_CMD(4)
// RTC_CMD(5) is documented to provide "no operation"
// Further testing indicates it infact just does nothing
// (although it'll hold the RTC in "command is active" state)
#define RTC_CMD_NOOP RTC_CMD(5)
#define RTC_CMD_TEST_ENTER RTC_CMD(6)
#define RTC_CMD_TEST_EXIT RTC_CMD(7)

#define RTC_WR 0
#define RTC_RD 1

struct RtcDateTime
{
	u8 year;
	u8 month;
	u8 day;
	u8 weekday;
	u8 hour;
	u8 minute;
	u8 second;
};

struct RtcTime
{
	u8 hour;
	u8 minute;
	u8 second;
};

union RtcAlarm
{
	struct
	{
		u8 alarmHour;
		u8 alarmMinute;
	};

	u16 frequencyDuty;
};

static const char* RtcDateTimeStrs[7] =
{
	"Year",
	"Month",
	"Day",
	"Weekday",
	"Hour",
	"Minute",
	"Second",
};

// for emu autodetection purposes (if gamecode wasn't enough)
#pragma GCC push_options
#pragma GCC optimize("O0")
static __attribute__((aligned(4))) const char* RTC_LIB_STR = "SIIRTC_V001";
#pragma GCC pop_options

#if true // don't know how much delay is really needed for RTC to react to changes? 3 nops seemed fine, but might as well go for overkill
#define GPIO_DELAY() do \
{ \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
} while (0)
#else
#define GPIO_DELAY() do \
{ \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
	__asm__ __volatile__("mov r0, r0"); \
} while (0)
#endif

static void WriteRtcCommand(u8 cmd)
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS;

	for (u8 i = 0; i < 8; i++)
	{
		u8 bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS;
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS;
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS;
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS;
	}
}

static u8 ReadRtcData()
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;

	u8 ret = 0;
	for (u8 i = 0; i < 8; i++)
	{
		// SCK falling edge actually outputs the bit
		// Although many emulators are going to do rising edge rather
		// Doesn't really matter on hardware how this is structured
		GPIO_PORT_DATA = GPIO_RTC_CS;
		GPIO_PORT_DATA = GPIO_RTC_CS;
		GPIO_PORT_DATA = GPIO_RTC_CS;
		GPIO_PORT_DATA = GPIO_RTC_CS;
		GPIO_PORT_DATA = GPIO_RTC_CS;
		GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

		u8 bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT;
		ret = (ret >> 1) | (bit << 7);
	}

	return ret;
}

static void WriteRtcData(u8 data)
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS;

	for (u8 i = 0; i < 8; i++)
	{
		u8 bit = ((data >> i) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS;
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS;
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS;
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS;
	}
}

static void DoRtcReset()
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_RESET);

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static u8 GetRtcStatus()
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_STATUS | RTC_RD);
	u8 status = ReadRtcData();

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;

	return status;
}

static void SetRtcStatus(u8 status)
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_STATUS | RTC_WR);
	WriteRtcData(status);

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static void GetRtcDateTime(struct RtcDateTime* rtcDateTime)
{
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_DATETIME | RTC_RD);
	for (u8 i = 0; i < sizeof(struct RtcDateTime); i++)
	{
		((u8*)rtcDateTime)[i] = ReadRtcData();
	}

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static void SetRtcDateTime(struct RtcDateTime* rtcDateTime)
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_DATETIME | RTC_WR);
	for (u8 i = 0; i < sizeof(struct RtcDateTime); i++)
	{
		WriteRtcData(((u8*)rtcDateTime)[i]);
	}

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static void GetRtcTime(struct RtcTime* rtcTime)
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_TIME | RTC_RD);
	for (u8 i = 0; i < sizeof(struct RtcTime); i++)
	{
		((u8*)rtcTime)[i] = ReadRtcData();
	}

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static void SetRtcTime(struct RtcTime* rtcTime)
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_TIME | RTC_WR);
	for (u8 i = 0; i < sizeof(struct RtcTime); i++)
	{
		WriteRtcData(((u8*)rtcTime)[i]);
	}

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static void SetRtcAlarm(union RtcAlarm* rtcAlarm)
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_ALARM | RTC_WR);
	for (u8 i = 0; i < sizeof(union RtcAlarm); i++)
	{
		WriteRtcData(((u8*)rtcAlarm)[i]);
	}

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static void EnterRtcTestMode()
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_TEST_ENTER);

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static void ExitRtcTestMode()
{
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;

	WriteRtcCommand(RTC_CMD_TEST_EXIT);

	GPIO_PORT_DATA = GPIO_RTC_SCK;
	GPIO_PORT_DATA = GPIO_RTC_SCK;
}

static void CheckRtc()
{
#if false
	GPIO_PORT_CTRL = GPIO_RW_MODE; GPIO_DELAY();
	GPIO_PORT_DIR = 0xF; GPIO_DELAY();
	GPIO_PORT_DATA = 0xF; GPIO_DELAY();
	u16 data0 = GPIO_PORT_DATA; GPIO_DELAY(); // F

	GPIO_PORT_DIR = 0; GPIO_DELAY();
	u16 data1 = GPIO_PORT_DATA; GPIO_DELAY(); // 6

	GPIO_PORT_DIR = 0xF; GPIO_DELAY();
	u16 data2 = GPIO_PORT_DATA; GPIO_DELAY(); // F

	GPIO_PORT_DIR = 0; GPIO_DELAY();
	u16 data3 = GPIO_PORT_DATA; GPIO_DELAY(); // 6

	GPIO_PORT_DATA = 0; GPIO_DELAY();
	u16 data4 = GPIO_PORT_DATA; GPIO_DELAY(); // 6

	GPIO_PORT_DIR = 0xF; GPIO_DELAY();
	u16 data5 = GPIO_PORT_DATA; GPIO_DELAY(); // 0

	GPIO_PORT_DIR = 0; GPIO_DELAY();
	u16 data6 = GPIO_PORT_DATA; GPIO_DELAY(); // 2

	GPIO_PORT_DIR = 0xF; GPIO_DELAY();
	u16 data7 = GPIO_PORT_DATA; GPIO_DELAY(); // 0

	GPIO_PORT_DIR = 0; GPIO_DELAY();
	GPIO_PORT_DATA = 0xF; GPIO_DELAY();
	u16 data8 = GPIO_PORT_DATA; GPIO_DELAY(); // 2

	GPIO_PORT_DIR = 0xF; GPIO_DELAY();
	u16 data9 = GPIO_PORT_DATA; GPIO_DELAY(); // F

	GPIO_PORT_DIR = 0; GPIO_DELAY();
	u16 data10 = GPIO_PORT_DATA; GPIO_DELAY(); // 6

	GPIO_PORT_DIR = 0xF; GPIO_DELAY();
	u16 data11 = GPIO_PORT_DATA; GPIO_DELAY(); // F

	printf("Data = %X / %X / %X / %X / %X / %X / %X / %X / %X / %X / %X / %X\n", data0, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11);

	u8 oldStatus = GetRtcStatus();
	SetRtcStatus(0x00);
	u8 zeroStatus = GetRtcStatus();
	SetRtcStatus(0xFF);
	u8 ffStatus = GetRtcStatus();
	printf("RTC status = %02X / %02X / %02X\n", oldStatus, zeroStatus, ffStatus);
#endif
#if false
	GPIO_PORT_CTRL = GPIO_RW_MODE; GPIO_DELAY();
	GPIO_PORT_DIR = 0xF; GPIO_DELAY();
	GPIO_PORT_DATA = 0xF; GPIO_DELAY();
	u16 data0 = GPIO_PORT_DATA; GPIO_DELAY(); // F

	GPIO_PORT_DIR = 0; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();
	GPIO_PORT_DIR = 0xF; GPIO_DELAY();
	u16 data1 = GPIO_PORT_DATA; GPIO_DELAY(); // 0

	printf("%X / %X\n", data0, data1);
	if (data0 != 0xF || data1 != 0)
	{
		printf("Test Failed (expected F / 0)\n");
	}
	else
	{
		printf("Test Passed\n");
	}
#endif
#if false
	u8 status = GetRtcStatus();
	printf("RTC Status = %02X\n", status);

	struct RtcDateTime dateTime;
	GetRtcDateTime(&dateTime);
	printf("Year = %02X / Month = %02X / Day = %02X / Weekday = %02X / Hour = %02X / Minute = %02X / Second = %02X\n",
		dateTime.year, dateTime.month, dateTime.day, dateTime.weekday, dateTime.hour, dateTime.minute, dateTime.second);

	struct RtcTime time;
	GetRtcTime(&time);
	printf("Hour = %02X / Minute = %02X / Second = %02X\n", time.hour, time.minute, time.second);

	union RtcAlarm alarm;
	GetRtcAlarm(&alarm);
	printf("Alarm Hour = %02X / Alarm Minute = %02X\n", alarm.alarmHour, alarm.alarmMinute);

	while (true)
	{
		VBlankIntrWait();
	}
#endif

	SetRtcStatus(0x40);
	u8 status = GetRtcStatus();
	if (status == 0xFF)
	{
		// might be in test mode, exit test mode and re-set and re-read status
		ExitRtcTestMode();
		SetRtcStatus(0x40);
		status = GetRtcStatus();
		if (status == 0xFF)
		{
			printf("RTC battery is dead\n");

			while (true)
			{
				VBlankIntrWait();
			}
		}
	}

	if (status == 0x00)
	{
		printf("RTC is not present\n");

		while (true)
		{
			VBlankIntrWait();
		}
	}

	if (status != 0x40)
	{
		printf("RTC status write is non-functional\n");
		printf("Likely incomplete emulator\n");

		while (true)
		{
			VBlankIntrWait();
		}
	}
}

static void TestRtcCsHighStartCommand()
{
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	// CS high begins an RTC command
	// Nothing fancy has to actually be done here (despite what games and spec sheets imply)

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	for (u8 i = 0; i < 8; i++)
	{
		u8 bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = ReadRtcData();
	GPIO_PORT_DATA = 0;

	printf("CS High Start Cmd Test: %s", status == 0x6A ? "Passed" : "Failed");
}

static void TestRtcCsLowAbortCommand()
{
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS | GPIO_RTC_SCK; GPIO_DELAY();
	// CS low aborts the current RTC command
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	for (u8 i = 0; i < 8; i++)
	{
		u8 bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = ReadRtcData();
	GPIO_PORT_DATA = 0;

	printf("CS Low Abort Cmd Test: %s\n", status == 0x6A ? "Passed" : "Failed");
}

static void TestRtcSioOldDataUsed()
{
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	// old SIO data is used rather than newer data
	GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS | GPIO_RTC_SCK; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	for (u8 i = 1; i < 8; i++)
	{
		u8 bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = ReadRtcData();
	GPIO_PORT_DATA = 0;

	printf("SIO Old Data Used Test: %s", status == 0x6A ? "Passed" : "Failed");
}

static void TestRtcSioNotSckEdge()
{
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	// old SIO data is used rather than newer data
	// this is not picked up at a falling SCK edge!
	GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS | GPIO_RTC_SCK; GPIO_DELAY();
	GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	// this data is used, rather than the above
	GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS | GPIO_RTC_SCK; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	for (u8 i = 2; i < 8; i++)
	{
		u8 bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = ReadRtcData();
	GPIO_PORT_DATA = 0;

	printf("SIO Not SCK Edge Test: %s\n", status == 0x6A ? "Passed" : "Failed");
}

static void TestRtcLowCsIsHighSck()
{
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	// CS low implies SCK high
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();
	// This should not trigger a bit read, despite the seeming rising edge (CS low meant SCK was "high" to the chip)
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	for (u8 i = 0; i < 8; i++)
	{
		u8 bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = ReadRtcData();
	GPIO_PORT_DATA = 0;

	printf("Low CS = High SCK Test: %s", status == 0x6A ? "Passed" : "Failed");
}

static void TestRtcUnusedLowInPin()
{
	// bit 3 is unused normally
	// in the case of Solar Sensor + RTC however, it is used
	// for unused, it should be forced low with an in direction
	// for sensor, it should be 0 in practice in this case, since SIO/RST bit is high (unless light is somehow strong enough in this test?)

	// test CS low
	GPIO_PORT_DIR = GPIO_RTC_UNUSED | GPIO_RTC_CS | GPIO_RTC_SIO | GPIO_RTC_SCK; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_UNUSED; GPIO_DELAY();
	GPIO_PORT_DIR = 0; GPIO_DELAY();
	u8 inPins0 = GPIO_PORT_DATA; GPIO_DELAY(); // expected: 0b0010

	// test CS high
	GPIO_PORT_DIR = GPIO_RTC_UNUSED | GPIO_RTC_CS | GPIO_RTC_SIO | GPIO_RTC_SCK; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_UNUSED | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DIR = GPIO_RTC_CS; GPIO_DELAY();
	u8 inPins1 = GPIO_PORT_DATA; GPIO_DELAY(); // expected: 0b0110 (note it'd be 0b0010 if CS was in an in direction!)

	GPIO_PORT_DATA = 0;

	printf("Unused Low In Pin Test: %s", (inPins0 & 8) == 0 && (inPins1 & 8) == 0 ? "Passed" : "Failed");
}

static void TestRtcLowCsInPins()
{
	// CS low forces SIO in pins to be high
	// CS/SCK are forced low for in pins
	GPIO_PORT_DIR = GPIO_RTC_UNUSED | GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	GPIO_PORT_DIR = GPIO_RTC_UNUSED; GPIO_DELAY();
	u8 inPins0 = GPIO_PORT_DATA; // expected: 0b0010

	GPIO_PORT_DIR = GPIO_RTC_UNUSED | GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_SCK; GPIO_DELAY(); // this won't affect in pins
	GPIO_PORT_DIR = GPIO_RTC_UNUSED; GPIO_DELAY();
	u8 inPins1 = GPIO_PORT_DATA; // expected: 0b0010

	printf("Low CS In Pins Test: %s\n", inPins0 == 2 && inPins1 == 2 ? "Passed" : "Failed");
}

static void TestRtcCsInPin()
{
	// CS is forced low for in pins, even if it was high before

	// test CS high
	GPIO_PORT_DIR = GPIO_RTC_CS | GPIO_RTC_SIO | GPIO_RTC_SCK; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DIR = 0; GPIO_DELAY();
	u8 inPins = GPIO_PORT_DATA; GPIO_DELAY(); // expected: 0b0010

	GPIO_PORT_DIR = GPIO_RTC_CS | GPIO_RTC_SIO | GPIO_RTC_SCK; GPIO_DELAY();
	GPIO_PORT_DATA = 0;

	printf("CS In Pin Test: %s\n", (inPins & 4) == 0 ? "Passed" : "Failed");
}

static void TestRtcSckInPin()
{
	// SCK is forced low for in pins

	// test CS high + SCK high
	GPIO_PORT_DIR = GPIO_RTC_CS | GPIO_RTC_SIO | GPIO_RTC_SCK; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS | GPIO_RTC_SCK; GPIO_DELAY();
	GPIO_PORT_DIR = GPIO_RTC_CS; GPIO_DELAY();
	u8 inPins = GPIO_PORT_DATA; GPIO_DELAY(); // expected: 0b0110 (note it'd be 0b0010 if CS was in an in direction!)

	printf("SCK In Pin Test: %s\n", (inPins & 1) == 0 ? "Passed" : "Failed");
}

static void TestRtcSioFallingEdgeRead()
{
	SetRtcStatus(0x6A);
	// For SIO reading (i.e. RTC is writing to SIO), SIO is updated on the FALLING edge
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	u8 bit;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	u8 inPins0 = GPIO_PORT_DATA; // expected: 0b0111

	u8 status = 0;
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	u8 inPins1 = GPIO_PORT_DATA; // expected: 0b0111
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	u8 inPins2 = GPIO_PORT_DATA; // expected: 0b0101
	bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
	status = (status >> 1) | (bit << 7);

	for (u8 i = 1; i < 8; i++)
	{
		GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
		bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
		status = (status >> 1) | (bit << 7);
	}

	printf("SIO Read F. Edge Test: %s\n", (inPins0 & 2) == 2 && (inPins1 & 2) == 2 && (inPins2 & 2) == 0 && status == 0x6A ? "Passed" : "Failed");
}

static void TestRtcSioHighInWriting()
{
	// For SIO writing (i.e. CPU is writing to SIO), SIO is kept high for its in pins
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	u8 bit;
	u8 inData0[16];
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData0[i * 2 + 0] = GPIO_PORT_DATA; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData0[i * 2 + 1] = GPIO_PORT_DATA; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	}

	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	cmd = RTC_CMD_STATUS | RTC_WR;
	u8 inData1[16];
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData1[i * 2 + 0] = GPIO_PORT_DATA; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData1[i * 2 + 1] = GPIO_PORT_DATA; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = 0x00;
	u8 inData2[16];
	// write in status
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((status >> i) & 1);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData2[i * 2 + 0] = GPIO_PORT_DATA; GPIO_DELAY();

		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData2[i * 2 + 1] = GPIO_PORT_DATA; GPIO_DELAY();
	}

	u8 passed = 2;
	for (u8 i = 0; i < 16; i++)
	{
		// SIO bit should always be high in all these reads
		passed &= inData0[i];
		passed &= inData1[i];
		passed &= inData2[i];
	}

	printf("SIO High Writing Test: %s\n", passed ? "Passed" : "Failed");
}

static void TestRtcStatusWriteSet()
{
	// RTC status write is set once all 8 bits are written
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_WR;
	u8 bit;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = 0x00;
	// write in status
	for (u8 i = 0; i < 7; i++)
	{
		bit = ((status >> i) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	GPIO_PORT_DATA = 0; GPIO_DELAY();

	// expected: 0x6A
	u8 status0 = GetRtcStatus();
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	// write in status
	for (u8 i = 0; i < 7; i++)
	{
		bit = ((status >> i) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	bit = ((status >> 7) & 1);
	GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();
	// expected: 0x6A
	u8 status1 = GetRtcStatus();
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	// write in status
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((status >> i) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	GPIO_PORT_DATA = 0; GPIO_DELAY();
	// expected: 0x00
	u8 status2 = GetRtcStatus();

	printf("Status Write Set Test: %s\n", status0 == 0x6A && status1 == 0x6A && status2 == 0x00 ? "Passed" : "Failed");
}

static void TestRtcStatusReadLoop()
{
	// RTC status read loops over if you attempt to read past the initial 8 bits
	// The same goes for other commands (time repeats h/m/s, datetime repeats y/m/d/wd/h/m/s, but that is handled as a separate test)
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	u8 bit;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status0 = ReadRtcData();
	u8 status1 = ReadRtcData();

	GPIO_PORT_DATA = 0; GPIO_DELAY();

	printf("Status Read Loop Test: %s\n", status0 == 0x6A && status1 == 0x6A ? "Passed" : "Failed");
}

static void TestRtcStatusWriteLoop()
{
	// RTC status write loops over if you attempt to write past the initial 8 bits
	// The same does not strictly goes for other commands, which rather repeat the last byte over and over
	// Other commands are tested separately anyways
	SetRtcStatus(0x22);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_WR;
	u8 bit;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = 0x60;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((status >> i) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	status = 0x0A;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((status >> i) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	GPIO_PORT_DATA = 0; GPIO_DELAY();
	status = GetRtcStatus();

	printf("Status Write Loop Test: %s", status == 0x0A ? "Passed" : "Failed");
}

static void TestRtcStatusPartialWrite()
{
	// RTC status write loops over if you attempt to write past the initial 8 bits
	// Upon the 9th bit being written, the data seems to be committed into status (likely as a way to "reset" the bit counter)
	// However, this data also includes this 9th bit, so the 1st bit is discarded effectively
	// With the 16th bit, CS can go low to commit the data over to status
	// With the 17th bit, you get the same behavior as the 9th bit
	// This repeats so on forevermore
	// The same does not strictly goes for other commands(?), which have different behavior depending on the command
	// Other commands are tested separately anyways
	u8 bits;
	for (bits = 8; bits < 32; bits++)
	{
		SetRtcStatus(0x00);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = 0; GPIO_DELAY();

		u8 cmd = RTC_CMD_STATUS | RTC_WR;
		u8 bit;
		for (u8 i = 0; i < 8; i++)
		{
			bit = ((cmd >> (7 - i)) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}

		u8 zero_bytes = bits / 8;
		u8 remaining_bits = bits;
		if (zero_bytes > 1)
		{
			zero_bytes--;
			for (u8 i = 0; i < zero_bytes; i++)
			{
				for (u8 j = 0; j < 8; j++)
				{
					GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
					GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
				}
			}

			remaining_bits -= zero_bytes * 8;
		}

		u8 status;
		if (remaining_bits == 8)
		{
			status = 0x6A;
			for (u8 i = 0; i < 8; i++)
			{
				bit = ((status >> i) & 1);
				GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
			}
		}
		else
		{
			status = 0x6A << 1;
			for (u8 i = 0; i < 8; i++)
			{
				bit = ((status >> i) & 1);
				GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
			}

			remaining_bits -= 8;
			for (u8 i = 0; i < remaining_bits; i++)
			{
				GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
			}
		}

		GPIO_PORT_DATA = 0; GPIO_DELAY();
		status = GetRtcStatus();
		if (status != 0x6A)
		{
			break;
		}
	}

	printf("Status Part Write Test: %s", bits == 32 ? "Passed" : "Failed");
}

static void TestRtcStatusReadCorruption()
{
	// Reading data while asserting 0 in the SIO pin results in corruption of the shift register
	// The data which is corrupted is the previous data read (so upon reading bit 1, bit 0 may be corrupted)
	// This is not corruption of the underlying data (e.g. status) anyways

	// test 1s (these do not result in corruption)
	u8 result0;
	SetRtcStatus(0x00);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	u8 bit;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	for (u8 i = 0; i < 16; i++)
	{
		// 1 being asserted here will result in no corruption
		GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 status = 0;
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	for (u8 i = 0; i < 8; i++)
	{
		GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
		bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
		status = (status >> 1) | (bit << 7);
	}

	result0 = status;

	// test 0s (these result in corruption, but only for the shift register, not the original register)
	u8 result1;
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	for (u8 i = 0; i < 16; i++)
	{
		GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	}

	GPIO_PORT_DATA = 0; GPIO_DELAY();
	result1 = GetRtcStatus();

	// test 0 being set before falling edge, changing to 1 at falling edge
	// this results in corruption
	u8 results0[16];
	for (u8 bitpos = 0; bitpos < 16; bitpos++)
	{
		SetRtcStatus(0x6A);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = 0; GPIO_DELAY();

		for (u8 i = 0; i < 8; i++)
		{
			bit = ((cmd >> (7 - i)) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}

		for (u8 i = 0; i < 16; i++)
		{
			if (i == bitpos)
			{
				GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			}
			else
			{
				// 1 being asserted here will result in no corruption
				GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			}
		}

		status = 0;
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		for (u8 i = 0; i < 8; i++)
		{
			GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
			bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
			status = (status >> 1) | (bit << 7);
		}

		results0[bitpos] = status;
	}

	// test 1 being set before falling edge, changing to 0 at falling edge
	// this results in corruption
	u8 results1[16];
	for (u8 bitpos = 0; bitpos < 16; bitpos++)
	{
		SetRtcStatus(0x6A);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = 0; GPIO_DELAY();

		for (u8 i = 0; i < 8; i++)
		{
			bit = ((cmd >> (7 - i)) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}

		for (u8 i = 0; i < 16; i++)
		{
			if (i == bitpos)
			{
				GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			}
			else
			{
				// 1 being asserted here will result in no corruption
				GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			}
		}

		status = 0;
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		for (u8 i = 0; i < 8; i++)
		{
			GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
			bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
			status = (status >> 1) | (bit << 7);
		}

		results1[bitpos] = status;
	}

	// test 0 being set before falling edge, staying at 0 at falling edge
	// this results in corruption
	u8 results2[16];
	for (u8 bitpos = 0; bitpos < 16; bitpos++)
	{
		SetRtcStatus(0x6A);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = 0; GPIO_DELAY();

		for (u8 i = 0; i < 8; i++)
		{
			bit = ((cmd >> (7 - i)) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}

		for (u8 i = 0; i < 16; i++)
		{
			if (i == bitpos)
			{
				GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			}
			else
			{
				// 1 being asserted here will result in no corruption
				GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
				GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			}
		}

		status = 0;
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		for (u8 i = 0; i < 8; i++)
		{
			GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
			bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
			status = (status >> 1) | (bit << 7);
		}

		results2[bitpos] = status;
	}

	static const u8 correct_read_corruption_results[16] =
	{
		0x6A, 0x6A, 0x68, 0x6A, 0x62, 0x6A, 0x4A, 0x2A,
		0x6A, 0x6A, 0x68, 0x6A, 0x62, 0x6A, 0x4A, 0x2A,
	};

	printf("Stat Read Corrupt Test: %s", result0 == 0 && result1 == 0x6A
		&& memcmp(results0, correct_read_corruption_results, sizeof(correct_read_corruption_results)) == 0
		&& memcmp(results1, correct_read_corruption_results, sizeof(correct_read_corruption_results)) == 0
		&& memcmp(results2, correct_read_corruption_results, sizeof(correct_read_corruption_results)) == 0
		? "Passed" : "Failed");
}

static void TestRtcInvalidMagic()
{
	// If the magic upper 4 bits are not 0b0110, then the command will not be processed
	// The RTC will act like no command is sent, allowing another command to be sent without CS low
	// This differs however from invalid command 5 or other valid commands, which stay "active"
	u8 bits;
	for (bits = 0; bits < 16; bits++)
	{
		SetRtcStatus(0x6A);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = 0; GPIO_DELAY();

		// RTC cmd is a byte large, so only if (bits % 8) == 0 here will the command end up succeeding
		u8 bit;
		for (u8 i = 0; i < bits; i++)
		{
			GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}

		u8 cmd = RTC_CMD_STATUS | RTC_RD;
		for (u8 i = 0; i < 8; i++)
		{
			bit = ((cmd >> (7 - i)) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}

		u8 status = 0;
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		for (u8 i = 0; i < 8; i++)
		{
			GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
			bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
			status = (status >> 1) | (bit << 7);
		}

		if (bits == 0 || bits == 8)
		{
			if (status != 0x6A)
			{
				break;
			}
		}
		else
		{
			if (status == 0x6A)
			{
				break;
			}
		}
	}

	printf("Invalid Cmd Magic Test: %s", bits == 16 ? "Passed" : "Failed");
}

static void TestRtcReadInPins()
{
	SetRtcStatus(0x6A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	// CS low implies SCK and SIO high
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_RD;
	u8 bit;
	u8 inData0[16];
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData0[i * 2 + 0] = GPIO_PORT_DATA; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData0[i * 2 + 1] = GPIO_PORT_DATA; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
	}

	// cmd is in, now for reading bits
	u8 status = 0; // expected: 0x6A (first 2 bits coming in are 0, 1)
#if false
	// expected: 0b101
	u8 outData = GPIO_PORT_DATA; GPIO_DELAY();
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	// expected: 0b111??? (cmd clocked in ended up setting SIO? maybe it's old data here? investigate)
	u8 inData0 = GPIO_PORT_DATA; GPIO_DELAY();

	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
	status = (status >> 1) | (bit << 7);
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();

	// expected: 0b100 (bit has not been clocked in)
	// actually is 0b110 ???
	u8 inData1 = GPIO_PORT_DATA; GPIO_DELAY();
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();

	// expected: 0b111 (bit has been clocked in)
	u8 inData2 = GPIO_PORT_DATA; GPIO_DELAY();
	bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
	status = (status >> 1) | (bit << 7);
#endif

	u8 inData1[16];
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY(); // comment when uncommenting
	for (u8 i = 0; i < 8; i++)
	{
		//GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = /*(GPIO_RTC_SIO << GPIO_RTC_SIO_SHIFT) | */GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		//GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData1[i * 2 + 0] = GPIO_PORT_DATA; GPIO_DELAY();

		//GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = /*(GPIO_RTC_SIO << GPIO_RTC_SIO_SHIFT) | */GPIO_RTC_CS; GPIO_DELAY();
		//GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData1[i * 2 + 1] = GPIO_PORT_DATA; GPIO_DELAY();
		bit = (GPIO_PORT_DATA & GPIO_RTC_SIO) >> GPIO_RTC_SIO_SHIFT; GPIO_DELAY();
		status = (status >> 1) | (bit << 7);
	}

	GPIO_PORT_DATA = 0;

	printf("Read In Pins Test: %02X\n", status /*inData == 0x7 ? "Passed" : "Failed"*/);
	for (u8 i = 0; i < 16; i++)
	{
		printf("%d ", inData0[i]);
	}
	printf("\n");
	for (u8 i = 0; i < 16; i++)
	{
		printf("%d ", inData1[i]);
	}
	printf("\n");
}

static void TestRtcWriteInPins()
{
	SetRtcStatus(0x0A);
	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	// CS low implies SCK and SIO high
	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_STATUS | RTC_WR;
	u8 bit;
	u8 inData0[16];
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData0[i * 2 + 0] = GPIO_PORT_DATA; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData0[i * 2 + 1] = GPIO_PORT_DATA; GPIO_DELAY();
	}

	// cmd is in, now for writing bits over
	u8 status = 0x60;

	u8 inData1[16];
	// write in lower status nybble
	for (u8 i = 0; i < 4; i++)
	{
		bit = ((status >> i) & 1);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY(); // SIO is not used
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData1[i * 2 + 0] = GPIO_PORT_DATA; GPIO_DELAY();

		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY(); // SIO is used?
		GPIO_PORT_DATA = ((bit ^ 0) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY(); // SIO is used?
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY(); // SIO is not used
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData1[i * 2 + 1] = GPIO_PORT_DATA; GPIO_DELAY();
	}
	// write in upper status nybble
	for (u8 i = 4; i < 8; i++)
	{
		bit = ((status >> i) & 1);
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY(); // SIO is not used
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData1[i * 2 + 0] = GPIO_PORT_DATA; GPIO_DELAY();

		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = ((bit ^ 0) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY(); // SIO is used?
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY(); // SIO is used?
		GPIO_PORT_DATA = ((bit ^ 1) << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY(); // SIO is not used
		GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		inData1[i * 2 + 1] = GPIO_PORT_DATA; GPIO_DELAY();
	}

	GPIO_PORT_DATA = 0;
	status = GetRtcStatus();

	printf("Write In Pins Test: %02X\n", status /*inData == 0x7 ? "Passed" : "Failed"*/);
	for (u8 i = 0; i < 16; i++)
	{
		printf("%d ", inData0[i]);
	}
	printf("\n");
	for (u8 i = 0; i < 16; i++)
	{
		printf("%d ", inData1[i]);
	}
	printf("\n");
}
#if false
static void TestRtcSioHighAssert()
{
	struct RtcDateTime rtcDateTime;
	rtcDateTime.year = 0x99;
	rtcDateTime.month = 1;
	rtcDateTime.day = 1;
	rtcDateTime.weekday = 0;
	rtcDateTime.hour = 0x23;
	rtcDateTime.minute = 0x52;
	rtcDateTime.second = 0x39;
	SetRtcStatus(0x40);
	SetRtcDateTime(&rtcDateTime);

	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_DATETIME | RTC_RD;
	u8 bit;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	// "write" data for days
	GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = (0 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	for (u8 i = 2; i < 8 * 7; i++)
	{
		GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
	}

	u8 unknown[7];
	GPIO_PORT_DATA = GPIO_RTC_SCK | GPIO_RTC_CS;
	for (u8 i = 0; i < 7; i++)
	{
		unknown[i] = ReadRtcData();
	}

	GPIO_PORT_DATA = 0;

	printf("Double Time Read Test:\n");
	for (u8 i = 0; i < 7; i++)
	{
		printf("%02X ", unknown[i]);
	}
}
#else
static void TestRtcSioHighAssert()
{
	struct RtcDateTime rtcDateTime;
	rtcDateTime.year = 0x00;
	rtcDateTime.month = 0x11;
	rtcDateTime.day = 0x24;
	rtcDateTime.weekday = 6;
	rtcDateTime.hour = 0x23;
	rtcDateTime.minute = 0x52;
	rtcDateTime.second = 0x59;
	SetRtcStatus(0x40);
	SetRtcDateTime(&rtcDateTime);

	GetRtcDateTime(&rtcDateTime);
	printf("%02X %02X %02X %02X %02X %02X %02X\n", rtcDateTime.year, rtcDateTime.month, rtcDateTime.day, rtcDateTime.weekday, rtcDateTime.hour, rtcDateTime.minute, rtcDateTime.second);

	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS; GPIO_DELAY();

	GPIO_PORT_DATA = GPIO_RTC_CS; GPIO_DELAY();
	GPIO_PORT_DATA = 0; GPIO_DELAY();

	u8 cmd = RTC_CMD_DATETIME | RTC_WR;
	u8 bit;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	rtcDateTime.year = 0x99;
	rtcDateTime.month = 0x01;
	rtcDateTime.day = 0x02;
	rtcDateTime.weekday = 3;
	rtcDateTime.hour = 0x16;
	rtcDateTime.minute = 0x25;
	rtcDateTime.second = 0x00;

	for (u8 i = 0; i < 6; i++)
	{
		u8 data = ((u8*)&rtcDateTime)[i];
		for (u8 j = 0; j < 8; j++)
		{
			bit = ((data >> j) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}
	}

	u8 data = 0x59;
	for (u8 j = 0; j < 1; j++)
	{
		bit = ((data >> j) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	GPIO_PORT_DATA = 0; GPIO_DELAY();
	GetRtcDateTime(&rtcDateTime);
#if false
	u8 cmd = RTC_CMD_DATETIME | RTC_WR;
	u8 bit;
	for (u8 i = 0; i < 8; i++)
	{
		bit = ((cmd >> (7 - i)) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}

	GPIO_PORT_DIR = GPIO_RTC_SCK | GPIO_RTC_SIO | GPIO_RTC_CS;

	rtcDateTime.year = 0x99;
	rtcDateTime.month = 0x01;
	rtcDateTime.day = 0x02;
	rtcDateTime.weekday = 3;
	rtcDateTime.hour = 0x16;
	rtcDateTime.minute = 0x25;
	rtcDateTime.second = 0x59;

	for (u8 i = 0; i < 7; i++)
	{
		u8 data = ((u8*)&rtcDateTime)[i];
		for (u8 j = 0; j < 8; j++)
		{
			bit = ((data >> j) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}
	}

	GPIO_PORT_DATA = (1 << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS;
	for (u16 i = 0; i < 61 * 60; i++)
	{
		VBlankIntrWait();
	}

	/*for (u8 i = 0; i < 2; i++)
	{
		//u8 data = ((u8*)&rtcDateTime)[6 - i];
		u8 data = 0x00;
		for (u8 j = 0; j < 8; j++)
		{
			bit = ((data >> j) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}
	}*/

	/*for (u8 i = 0; i < 1; i++)
	{
		//u8 data = ((u8*)&rtcDateTime)[6 - i];
		u8 data = 0x52;
		for (u8 j = 0; j < 8; j++)
		{
			bit = ((data >> j) & 1);
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
			GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
		}
	}*/

	/*for (u8 j = 0; j < 4; j++)
	{
		bit = ((0x5 >> j) & 1);
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_CS; GPIO_DELAY();
		GPIO_PORT_DATA = (bit << GPIO_RTC_SIO_SHIFT) | GPIO_RTC_SCK | GPIO_RTC_CS; GPIO_DELAY();
	}*/

	GPIO_PORT_DATA = 0;
	GetRtcDateTime(&rtcDateTime);
	GPIO_PORT_DATA = 0;
#endif

	printf("Cancel Time Write Test:\n%02X %02X %02X %02X %02X %02X %02X\n", rtcDateTime.year, rtcDateTime.month, rtcDateTime.day, rtcDateTime.weekday, rtcDateTime.hour, rtcDateTime.minute, rtcDateTime.second);

	u8 delay = 120;
	while (delay--)
	{
		VBlankIntrWait();
	}

	printf(CON_CLS());
	/*struct RtcTime rtcTime;
	rtcTime.hour = 0x23;
	rtcTime.minute = 0x59;
	rtcTime.second = 0x59;
	SetRtcTime(&rtcTime);*/
	GetRtcDateTime(&rtcDateTime);
	printf("%02X %02X %02X %02X %02X %02X %02X\n", rtcDateTime.year, rtcDateTime.month, rtcDateTime.day, rtcDateTime.weekday, rtcDateTime.hour, rtcDateTime.minute, rtcDateTime.second);
	printf(CON_UP());

	while (true)
	{
		/*SetRtcTime(&rtcTime);

		u8 delay = 61;
		while (delay--)
		{
			VBlankIntrWait();
		}*/

		VBlankIntrWait();
		GetRtcDateTime(&rtcDateTime);
		printf("%02X %02X %02X %02X %02X %02X %02X\n", rtcDateTime.year, rtcDateTime.month, rtcDateTime.day, rtcDateTime.weekday, rtcDateTime.hour, rtcDateTime.minute, rtcDateTime.second);
		printf(CON_UP());
	}

}
#endif

int main()
{
	irqInit();
	irqEnable(IRQ_VBLANK);

	consoleDemoInit();

	/*printf("Waiting for A press");
	while (true)
	{
		VBlankIntrWait();
		scanKeys();
		if (keysDown() & KEY_A)
		{
			break;
		}
	}
	printf(CON_CLS());*/

	// Init GPIO for read/writing
	GPIO_PORT_CTRL = GPIO_RW_MODE;

	CheckRtc();

	TestRtcCsHighStartCommand();
	TestRtcCsLowAbortCommand();
	TestRtcSioOldDataUsed();
	TestRtcSioNotSckEdge();
	TestRtcLowCsIsHighSck();
	TestRtcUnusedLowInPin();
	TestRtcLowCsInPins();
	TestRtcCsInPin();
	TestRtcSckInPin();
	TestRtcSioFallingEdgeRead();
	TestRtcSioHighInWriting();
	TestRtcStatusWriteSet();
	TestRtcStatusReadLoop();
	TestRtcStatusWriteLoop();
	TestRtcStatusPartialWrite();
	TestRtcStatusReadCorruption();
	TestRtcInvalidMagic();
	//TestRtcReadInPins();
	//TestRtcWriteInPins();
	//TestRtcSioHighAssert();

	while (true)
	{
		VBlankIntrWait();
	}
#if false
	while (true)
	{
		for (u8 regOffset = 0; regOffset < 6; regOffset++)
		{
			for (u8 reg = 0; reg < 7; reg++)
			{
				struct RtcDateTime dateTime;
				u8 timeResults[0x100];
				uint8_t regReadback = reg + regOffset;
				if (regReadback > 6)
				{
					regReadback -= 6;
				}

				for (u16 i = 0; i < 0x100; i++)
				{
					dateTime.year = 0; // & 0xFF
					dateTime.month = 1; // & 0x1F
					dateTime.day = 1; // & 0x3F
					dateTime.weekday = 0; // & 0x07
					dateTime.hour = 0; // & 0x1F / & 0x3F
					dateTime.minute = 0x59; // & 0x7F
					dateTime.second = 0; // & 0x7F
					((u8*)&dateTime)[reg] = i;
					DoRtcReset();
					SetRtcDateTime(&dateTime);

					/*u8 delay = 61;
					while (delay--)
					{
						VBlankIntrWait();
					}*/

					GetRtcDateTime(&dateTime);
					timeResults[i] = ((u8*)&dateTime)[regReadback];
				}

				VBlankIntrWait();
				printf(CON_CLS());

				for (u16 i = 0; i < 0x80;)
				{
					for (u8 j = 0; j < 7; j++)
					{
						printf("%02X ", timeResults[i + j]);
					}

					printf("%02X\n", timeResults[i + 7]);
					i += 8;
				}

				printf("Results 00-7F\n");
				printf("Setting %s register\nReading %s register\n", RtcDateTimeStrs[reg], RtcDateTimeStrs[regReadback]);

				u8 delay = 60;
				while (delay--)
				{
					VBlankIntrWait();
				}

				printf(CON_CLS());

				for (u16 i = 0x80; i < 0x100;)
				{
					for (u8 j = 0; j < 7; j++)
					{
						printf("%02X ", timeResults[i + j]);
					}

					printf("%02X\n", timeResults[i + 7]);
					i += 8;
				}

				printf("Results 80-FF\n");
				printf("Setting %s register\nReading %s register\n", RtcDateTimeStrs[reg], RtcDateTimeStrs[regReadback]);

				delay = 60;
				while (delay--)
				{
					VBlankIntrWait();
				}
			}
		}
	}
#endif
}
