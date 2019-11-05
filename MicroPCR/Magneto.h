#pragma once

#define EMULATOR  //KJD

#ifdef EMULATOR  //KJD
#define MAX_DRIVER_NUM 1
#else
#define MAX_DRIVER_NUM 3
#endif

#define WM_MOTOR_POS_CHANGED			WM_USER + 100
#define WM_WAIT_TIME_CHANGED			WM_USER + 101
#define WM_START_PCR					WM_USER + 102
#define WM_MAGNET_CHANGED				WM_USER + 103
#define WM_HEATING_CHANGED				WM_USER + 104
#define WM_SECOND_WAIT_TIME_CHANGED		WM_USER + 105

// Magneto 관련 상수 선언
// 상수 define 이 아니므로, 전체 upper case 를 하지 않고, 첫 문자만 upper 로 naming 설정
namespace Magneto{
//	const static int ComPort = 9;
	const static int BaudRate = 115200;

	const static int MaxSlaves = MAX_DRIVER_NUM; //KJD

	const static CString CompileMessageOk = L"Compile Success";

	const static int TimerRuntaskID = 0x01;
	const static int TimerRuntaskDuration = 200;

	// Motor 좌표 관련 값들
	// 드라이버 배치
	// (위에서 아래로)
	// front
	// 20 - filter
	// 28 - syringe
	// 42 - X axis
	// (위에서 아래로)
	// rear
	// 20 - rotate
	// 28 - Y axis
	// 42 - loading
	// 42 - magnet
	// XY 축 모터
	// X 축: M42, 4 mm/rev, stroke 41.66 mm
	// Y 축: M28, 1 mm/rev
	#define	M_X_AXIS_LEAD	4	// 4 mm/rev
	#define	M_Y_AXIS_LEAD	1	// 4 mm/rev
	#define M_X_AXIS_OFFSET -2	// mm    //center
	#define M_Y_AXIS_OFFSET -16.6	// mm // center
	#define	M_X_AXIS_PUSLE2MILI(pulse)	(pulse / 10000.0 * M_X_AXIS_LEAD)
	#define	M_Y_AXIS_PUSLE2MILI(pulse)	(pulse / 10000.0 * M_Y_AXIS_LEAD)
	// 마그넷 축에 대한 로딩 축의 상대 위치 (옵셋)
	// 챔버 포지션 테이블은 마그넷 축 위치 기준
	// 로드 축과 관계되는 x-y 평면상의 좌표는 이 옵셋을 고려.
	#define M_LOAD_OFFSET_FROM_MAGNET_X (int)(29.1)    // ksy
	#define M_LOAD_OFFSET_FROM_MAGNET_Y (int)( 0.0)    // ksy
	// 챔버 구조
	// 6번 챔버는 믹싱 전용. 바닥에 작은 홈이 있어 마이크로 파이펫 팁에 해당하는 튜브 관통.
	#define	MAX_CHAMBER		6
	#define	MIXING_CHAMBER	5
	// 
	//       (1)
	//        ^
	// (6)    |     (2)
	//    ----+----->
	// (5)    |     (3)
	//        |
	//       (4)
	// 챔버 포지션 (10000 마이크로 스텝 단위)
	// micro step = mm / (mm/rev) * (10000 step/rev)
	#define M_X_AXIS_MAGNET_POSITION (int)((  0.00 + M_X_AXIS_OFFSET) / M_X_AXIS_LEAD * 10000)
	#define M_Y_AXIS_MAGNET_POSITION (int)((  0.00 + M_Y_AXIS_OFFSET) / M_Y_AXIS_LEAD * 10000)
	#define	CHAMBER_1_POS_X	(int)((  0.00 + M_LOAD_OFFSET_FROM_MAGNET_X + M_X_AXIS_OFFSET) / M_X_AXIS_LEAD * 10000)	//       0
	#define	CHAMBER_1_POS_Y	(int)(( 14.49 + M_LOAD_OFFSET_FROM_MAGNET_Y + M_Y_AXIS_OFFSET) / M_Y_AXIS_LEAD * 10000)	//  144900
	#define	CHAMBER_2_POS_X	(int)((-12.55 + M_LOAD_OFFSET_FROM_MAGNET_X + M_X_AXIS_OFFSET) / M_X_AXIS_LEAD * 10000)	//   31375
	#define	CHAMBER_2_POS_Y	(int)((  7.24 + M_LOAD_OFFSET_FROM_MAGNET_Y + M_Y_AXIS_OFFSET) / M_Y_AXIS_LEAD * 10000)	//   72400
	#define	CHAMBER_3_POS_X	(int)((-12.55 + M_LOAD_OFFSET_FROM_MAGNET_X + M_X_AXIS_OFFSET) / M_X_AXIS_LEAD * 10000)	//   31375
	#define	CHAMBER_3_POS_Y	(int)(( -7.24 + M_LOAD_OFFSET_FROM_MAGNET_Y + M_Y_AXIS_OFFSET) / M_Y_AXIS_LEAD * 10000)	//  -72400
	#define	CHAMBER_4_POS_X	(int)((  0.00 + M_LOAD_OFFSET_FROM_MAGNET_X + M_X_AXIS_OFFSET) / M_X_AXIS_LEAD * 10000)	//       0
	#define	CHAMBER_4_POS_Y	(int)((-14.49 + M_LOAD_OFFSET_FROM_MAGNET_Y + M_Y_AXIS_OFFSET) / M_Y_AXIS_LEAD * 10000)	// -144900
	#define	CHAMBER_5_POS_X	(int)(( 12.55 + M_LOAD_OFFSET_FROM_MAGNET_X + M_X_AXIS_OFFSET) / M_X_AXIS_LEAD * 10000)	//  -31375
	#define	CHAMBER_5_POS_Y	(int)(( -7.24 + M_LOAD_OFFSET_FROM_MAGNET_Y + M_Y_AXIS_OFFSET) / M_Y_AXIS_LEAD * 10000)	//  -72400
	#define	CHAMBER_6_POS_X	(int)(( 12.55 + M_LOAD_OFFSET_FROM_MAGNET_X + M_X_AXIS_OFFSET) / M_X_AXIS_LEAD * 10000)	//  -31375
	#define	CHAMBER_6_POS_Y	(int)((  7.24 + M_LOAD_OFFSET_FROM_MAGNET_Y + M_Y_AXIS_OFFSET) / M_Y_AXIS_LEAD * 10000)	//   72400
	static const int gAxisPosition[MAX_CHAMBER][2] = {
		{CHAMBER_1_POS_X, CHAMBER_1_POS_Y},
		{CHAMBER_2_POS_X, CHAMBER_2_POS_Y},
		{CHAMBER_3_POS_X, CHAMBER_3_POS_Y},
		{CHAMBER_4_POS_X, CHAMBER_4_POS_Y},
		{CHAMBER_5_POS_X, CHAMBER_5_POS_Y},
		{CHAMBER_6_POS_X, CHAMBER_6_POS_Y},
	};
	#define M_X_AXIS_SPEED			(int)20000	// microstep / s 
	#define M_Y_AXIS_SPEED			(int)60000

	//180810 KJD
	#define M_CHAMBER	0
	#define M_FILTER	1
	#define M_PUMPING	2
	#define MAX_DRIVER_NUM 3

	// M_MAGNET (마그넷 상하 이동)
	// 마그넷 축: M?, 2 mm/rev, stroke ? mm
	#define	M_MAGNET_LEAD				2	// 2 mm/rev
	#define M_MAGNET_OFFSET				0	// mm
	#define	M_MAGNET_PUSLE2MILI(pulse)	(pulse / 10000.0 * M_MAGNET_LEAD)
	#define M_MAGNET_POS_ORIGIN			(int)(     0 + M_MAGNET_OFFSET)
	//#define M_MAGNET_POS_DOWN			(int)(-1*(48 + M_MAGNET_OFFSET) / M_MAGNET_LEAD * 10000)
	//#define M_MAGNET_POS_LOCK			(int)(-1*(50 + M_MAGNET_OFFSET) / M_MAGNET_LEAD * 10000)
	#define M_MAGNET_POS_DOWN			(int)(-1*(42 + M_MAGNET_OFFSET) / M_MAGNET_LEAD * 10000) // test
	#define M_MAGNET_POS_LOCK			(int)(-1*(44 + M_MAGNET_OFFSET) / M_MAGNET_LEAD * 10000) // test
	#define M_MAGNET_SPEED				(int)20000
	// M_LOAD (시약 주입 및 혼합)
	// 로딩 축: M?, 1 mm/rev, stroke ? mm
	#define	M_LOAD_LEAD					1	// 1 mm/rev
	#define M_LOAD_OFFSET				0	// mm
	#define	M_LOAD_PUSLE2MILI(pulse)	(pulse / 10000.0 * M_LOAD_LEAD)
	#define	M_LOAD_POS_ORIGIN			(int)(     0.0 + M_LOAD_OFFSET)
	#define	M_LOAD_POS_LOAD				(int)(-1*(23.0 + M_LOAD_OFFSET) / M_LOAD_LEAD * 10000)
	#define	M_LOAD_POS_MIXING_TOP		(int)(-1*(20.8 + M_LOAD_OFFSET) / M_LOAD_LEAD * 10000)
	#define	M_LOAD_POS_MIXING_BOTTOM	(int)(-1*(30.8 + M_LOAD_OFFSET) / M_LOAD_LEAD * 10000)
	#define M_LOAD_SPEED				(int)20000
	#define M_LOAD_MIXING_SPEED			(int)40000

	// M_SYRINGE (잔여물 제거)
	// 시린지 축: M?, 1 mm/rev, stroke 50 mm
	#define	M_SYRINGE_LEAD					1	// 1 mm/rev
	#define M_SYRINGE_OFFSET				0	// mm
	#define	M_SYRINGE_PUSLE2MILI(pulse)		(pulse / 10000.0 * M_SYRINGE_LEAD)
	#define M_SYRINGE_POS_ORIGIN			(int)(     0.0 + M_SYRINGE_OFFSET)
	#define M_SYRINGE_POS_DOWN				(int)(-1*(40.0 + M_SYRINGE_OFFSET) / M_SYRINGE_LEAD * 10000)
	#define M_SYRINGE_SPEED					(int)100000
	// M_ROTATE (마그넷 회전)
	// 마그넷 회전 축: M?, 회전축, 홈 스위치 장착 요망
	// pulse to mili degree
	#define M_ROTATE_OFFSET					0	// mm
	#define	M_ROTATE_PUSLE2DEGREE(pulse)	(pulse / 10000.0 * 360 * 1000)
	#define M_ROTATE_POS_ORIGIN				(int)(   0 + M_ROTATE_OFFSET)
	#define M_ROTATE_POS_BLOCKED			(int)(   0 + M_ROTATE_OFFSET)
	#define M_ROTATE_POS_WASTE				(int)((+90 + M_ROTATE_OFFSET) / 360.0 * (10000/1))		// (10000 pulse/rev)
	#define M_ROTATE_POS_PCR				(int)((-90 + M_ROTATE_OFFSET) / 360.0 * (10000/1))		// (10000 pulse/rev)
	#define M_ROTATE_SPEED					(int)3500

	//********************************************* 13.10.2015 *********
	//#define M_ROTATING	0
	// Rotating axis
#define M_CHAMBER_STEP_PER_REV			32000.0
#define M_CHAMBER_GEAR_RATIO			2
#define M_CHAMBER_OFFSET				1464//s1-1618// s2-1464//1297 //1197	// origin offset (rotate 축 origin이후 보정 값)
#define M_CHAMBER_DISK_OFFSET			3695	// 20.7844 deg (디스크 기준점에서 디스크의 홀까지 간격)
#define M_CHAMBER_BACKLASH				100//0//680//682
#define M_CHAMBER_DIFF					1304	// 7.335 deg
#define M_CHAMBER_INTERVAL				4923	// chamber에 각 간격 사이//(float)27.692
#define M_CHAMBER_STEP_PER_DEGREE		(float)(M_CHAMBER_GEAR_RATIO*M_CHAMBER_STEP_PER_REV/360)
#define	M_CHAMBER_PUSLE2DEGREE(pulse)	(pulse / M_CHAMBER_STEP_PER_REV * 360)
#define M_CHAMBER_SPEED					(int)16000//32000  //(int)16000  //KSY 챔버 회전 속도 

	// pumping axi
#define PI								(float)3.141592
#define M_PUMPING_STEP_PER_REV			16000.0 //16000.0
#define M_PUMPING_LEAD					1		// 1 mm / 1 rev
#define M_PUMPING_OFFSET				0	    // mm
#define	M_PUMPING_PUSLE2MILI(pulse)		(pulse / M_PUMPING_STEP_PER_REV * M_PUMPING_LEAD)
#define	M_PUMPING_POS_ORIGIN			(int)(   0.0 + M_PUMPING_OFFSET)
#define	M_PUMPING_POS_TOP				(int)((28.00 + M_PUMPING_OFFSET) / M_PUMPING_LEAD * M_PUMPING_STEP_PER_REV)
#define M_PUMPING_POS_MIXING_TOP		(int)((28.00 + M_PUMPING_OFFSET) / M_PUMPING_LEAD * M_PUMPING_STEP_PER_REV)	// mixing 시에 상단값 설정
#define	M_PUMPING_POS_BOTTOM			(int)((60.69 + M_PUMPING_OFFSET) / M_PUMPING_LEAD * M_PUMPING_STEP_PER_REV)
#define M_PUMPING_POS_UP_OFFSET			(int)-28000 //KSY			// -값을 주면 PUMPING 축이 상승(원하는 위치에 도달하지 못했을 때)
	// +값을 주면 PUMPING 축이 하강(원하는 위치보다 초과하였을 때)
#define M_PUMPING_DISK_RADIUS			(float)3.0 //s1-2.8//2.83//2.3	//kSY	// puming syringe의 반지름으로 mm 단위로 입력
	//170106_2 KSY - 1
#define M_PUMPING_SPEED					(int)100000//(int)100000
#define M_PUMPING_MIXING_SPEED			(int)100000//(int)100000
#define M_PUMPING_READY_SPEED			(int)100000//
#define M_PUMPING_SLOWLY_SPEED			(int)20000//	// 161027 자석을 On 상태에서 펌핑 업의 속도를 조절
	//170106_2 KSY - 2

	// filter axis  // 셋업 org :0  Motion : 1
#define M_FILTER_STEP_PER_REV			10000.0		// step
#define M_FILTER_OFFSET					338//s1-530//s2-338//390//770		// step ( about 5.24 deg)
#define M_FILTER_GEAR_RATIO				5.25	
#define M_FILTER_INTERVAL				90			// deg
#define M_FILTER_INTERVAL_PULSE			(int)(M_FILTER_GEAR_RATIO * M_FILTER_STEP_PER_REV) * ( 90.0 / 360.0)
#define	M_FILTER_PUSLE2DEGREE(pulse)	(pulse / M_FILTER_STEP_PER_REV * 360)
#define M_FILTER_SPEED					(int)26250

	// 150920 YJ 
	static const int DefaultPos =			INT_MAX;
	//180810 KJD
#ifdef EMULATOR
#define MAX_DRIVER_NUM 1
#define M_CHAMBER	0
#define M_FILTER	0
#define M_PUMPING	0
#define M_FILTER_SPEED	(int)16000
#define M_CHAMBER_SPEED	(int)16000//2500//(int)16000//32000 //(int)16000 //KSY 챔버 회전 속도 
#define M_PUMPING_SPEED	(int)16000//(int)100000
#define M_PUMPING_MIXING_SPEED	(int)16000//(int)100000
#define M_PUMPING_READY_SPEED	(int)16000//
#define M_PUMPING_SLOWLY_SPEED	(int)16000//	// 161027 자석을 On 상태에서 펌핑 업의 속도를 조절
#define M_PUMPING_STEP_PER_REV	(int)160// 1/100 emulation
#endif

	// Protocol check function
	/*
	CString checkProtocol(CString& protocol) {
		vector<CString> convertedProtocol;

		// CString to vector
		CString token;
		int count = 0;
		while (AfxExtractSubString(token, protocol, count, L'\n')) {
			convertedProtocol.push_back(token);
			count++;
			AfxMessageBox(token);
		}

		CString compileMessage = L"=====Compile Error=====\n";
		// cmd list 를 만들어 command 와 mapping 시킨다.
		static const CString tempCmdList[14] = { L"goto", L"filter", L"mixing", L"waiting", L"pumping up", L"pumping sup", L"pumping down", L"pumping sdown", L"ready", L"home", L"magnet on", L"magnet off", L"heating", L"pcr", };

		// 비정상적인 파일임을 알림
		if (convertedProtocol.size() == 0)
			return L"Compile Error: 비정상인 파일입니다.";

		// 모든 Protocol line 을 읽는다.
		for (int i = 0; i < convertedProtocol.size(); ++i)
		{
			int offset = 0;
			CString line = convertedProtocol[i].Trim();
			CString cmd = line.Tokenize(L" ", offset).Trim();

			if( cmd.Compare(L"pumping") == 0 )
			{
				CString subCmd = line.Tokenize(L" ", offset).Trim();

			
				if (subCmd.Compare(L"up") == 0 || subCmd.Compare(L"sup") == 0 || subCmd.Compare(L"down") == 0 || subCmd.Compare(L"sdown") == 0) // 기존 if( subCmd.Compare(L"up") == 0 || subCmd.Compare(L"down") == 0 ) 170106_2 KSY 
					cmd += L" " + subCmd;
				else
				{
					compileMessage.Format(L"%s\nLine %d : Invalid argument value", compileMessage, i+1);
					continue;
				}
			}
		
			// Command 가 없는 경우와 주석 문자가 처음 시작되는 경우 무시
			if (cmd.IsEmpty())
				continue;
		
			else if (cmd.GetAt(0) == '%')
				continue;
		
			// 값을 저장할 구조체 초기화
			ProtocolBinary bin = { -1, -1 };

			for (int j = 0; j < ProtocolCmd::MAX+1; ++j)
			{
				// 커맨드에 매개변수(args) 가 없는 경우 처리
				// 아래 명시된 프로토콜 커맨드는 매개변수가 없다.
				if ( (j == ProtocolCmd::MAGNET_ON) || (j == ProtocolCmd::MAGNET_OFF) || (j == ProtocolCmd::PCR) ||
						(j == ProtocolCmd::HOME) || (j == ProtocolCmd::READY) )
				{
					if (line.Compare(tempCmdList[j]) == 0)
						bin.cmd = j;
				}

				// 커맨드 매개변수(args) 가 있는 경우 처리
				// pumping 은 위에서 처리,
				// GO, FILTER, MIX, WAIT, HEATING 에 대해 처리한다.
				else if (cmd.Compare(tempCmdList[j]) == 0)
				{
					bin.cmd = j;

					CString arg = line.Tokenize(L" ", offset);

					// arg 값이 있는지 체크
					if (arg.Compare(L"") != 0)
					{
						if( arg.Compare(L"full") == 0 )
							bin.arg = -1;
						else
							bin.arg = _ttoi(arg);
					}

					// 없는 경우 에러 메시지 추가
					else
					{
						compileMessage.Format(L"%s\nLine %d : Invalid argument value", compileMessage, i+1);
					}
				
					break;
				}
			}

			if (bin.cmd == -1)
				compileMessage.Format(L"%s\nLine %d : Invalid command value", compileMessage, i + 1);
		}

		// Compile error message 가 변경되지 않은 경우 성공한 경우
		if (compileMessage.Compare(L"=====Compile Error=====\n") == 0)
		{
			return Magneto::CompileMessageOk;
		}

		return compileMessage;
	}
	*/
};

namespace DriverStatus{
	enum Enum{

		CONNECTED = 0,
		NOT_CONNECTED = 1,
		COMM_ERROR = 2,
		TOO_FEW_SLAVES = 3,
		TOO_MUCH_SLAVES = 4,
		SLAVE_ORDER_ERROR = 5,
		MAX = 5,
	};
};

namespace ProtocolCmd{
	enum Enum{
		GO = 0,
		FILTER = 1,
		MIX = 2,
		WAIT = 3,
		PUMPING_UP = 4,
		PUMPING_SUP = 5,	//170106_2 KYS - 1 sup 추가
		PUMPING_DOWN = 6,	//170106_2 KYS - 1  5->6 변경
		PUMPING_SDOWN = 7,	//170106_2 KYS - 1  sdown 추가
		READY = 8,			//170106_2 KYS - 1  6->8 변경
		HOME = 9,			//170106_2 KYS - 1  7->9 변경
		
		//siri 151206 - add arduino cmd
		MAGNET_ON = 10,		//170106_2 KYS - 1  8->10 변경
		MAGNET_OFF = 11,	//170106_2 KYS - 1  9->11 변경
		HEATING = 12,		//170106_2 KYS - 1  10->12 변경
		PCR = 13,			//170106_2 KYS - 1  11->13 변경
		MAX = 13,			//170106_2 KYS - 1  11->13 변경
	};

	const static CString toString[MAX+1] =
	{
		L"GO", L"FILTER", L"MIX", L"WAIT", L"PUMPING_UP", L"PUMPING_SUP",
		L"PUMPING_DOWN", L"PUMPING_SDOWN", L"READY", L"HOME", L"MAGNET_ON",
		L"MAGNET_OFF", L"HEATING", L"PCR"
	};
};

namespace ActionCmd{
	enum Enum{
		MOVE_ABS = 0,
		MOVE_INC = 1,
		MOVE_DEC = 2,
		HOME = 3,
		WAIT = 4,
		SECOND_WAIT = 5,
		PROTOCOL_STOP = 6,
		INTERNAL_END = 7,
		
		MAGNET_ON = 8,
		MAGNET_OFF = 9,
		HEATING = 10,
		PCR = 11,
		MAX = 11,
	};

	const static CString toString[MAX + 1] =
	{
		L"MOVE_ABS", L"MOVE_INC", L"MOVE_DEC", L"HOME",
		L"WAIT", L"SECOND_WAIT", L"PROTOCOL_STOP", L"INTERNAL_END",
		L"MAGNET_ON", L"MAGNET_OFF", L"HEATING", L"PCR"
	};
};

namespace MotorType{
	enum Enum{
		CHAMBER = M_CHAMBER,
		FILTER = M_FILTER,
		PUMPING = M_PUMPING,
		X_AXIS = 12,
		Y_AXIS = 13,
		MAGNET = 14,
		LOAD = 15,
		SYRINGE = 16,
		ROTATE = 17,
		MAX = 8,
	};

	const static CString toString[MAX + 1] = {L"CHAMBER", L"FILTER", L"PUMPING", L"X_AXIS", L"Y_AXIS", L"MAGNET", L"LOAD", L"SYRINGE", L"ROTATE"};
};

// Protocol raw data 를 관리하는 구조체
typedef struct PROTOCOL_BINARY{
	int cmd;
	int arg;
} ProtocolBinary;

// Action 들에 대한 cmd 와 argument 를 관리하는 클래스
class ActionBinary{
public:
	int cmd;
	vector<int> args;

	ActionBinary(int cmd, int count, ...) 
		:cmd(cmd)
	{
		va_list ap;
		va_start(ap, count);
		for (int i = 0; i < count; ++i)
			this->args.push_back(va_arg(ap, int));

		va_end(ap);
	}
};

class ActionData{
public:
	int cmd;
	vector<ActionBinary> actions;

	ActionData(int cmd) :cmd(cmd){}
};

class ActionBeans{
public:
	CString parentAction;
	vector<CString> childAction;

	ActionBeans(CString parentAction) :parentAction(parentAction){}
};

// For send message data format
struct MotorPos{
	double targetPos;
	double cmdPos;
	int driverErr;
};

typedef struct HEATING_CONTROL{
	int temper;
} heat_ctrl;




class CMagneto {

private:
	/** Driver field				***************/
	bool connected;
	int comPortNo;
	int driverErrCnt;

	/** Protocol Management			***************/
	vector<ProtocolBinary> protcolBinary;
	vector<vector<ActionBinary>> preDefinedAction;
	vector<ActionData> actionList;
	
	CString protocolCompile(vector<CString> &protocol);

	void initPredefinedAction();
	void initDriverParameter();

	inline double pulse2mili(MotorType::Enum type, long value){
		if (type == MotorType::CHAMBER) return M_CHAMBER_PUSLE2DEGREE(value);
		else if (type == MotorType::PUMPING) return M_PUMPING_PUSLE2MILI(value);
		else if (type == MotorType::FILTER) return M_FILTER_PUSLE2DEGREE(value);



		/*
		if (type == MotorType::CHAMBER) return M_MAGNET_PUSLE2MILI(value);
		else if (type == MotorType::PUMPING) return M_Y_AXIS_PUSLE2MILI(value);
		else if (type == MotorType::FILTER) return M_LOAD_PUSLE2MILI(value);
		*/
		else return 0.0;
	}

	/** Operation					***************/
	int currentAction;
	int currentSubAction;

	double currentTargetPos;
	double currnetPos;
	
	bool isStarted;
	bool isCompileEnd;

	/** For Waiting operation		***************/
	bool isWaitEnd;
	int waitCounter;
	bool isSecondWaitEnd;
	int secondwaitCounter;
	HWND hwnd;
	
	/** Constructure field			***************/
public:
	CMagneto();
	virtual ~CMagneto();

	/** Connection					***************/
public:
	void searchPort(vector<CString> &portList);

	DriverStatus::Enum connect(int comPortNo);
	void disconnect();
	bool isConnected();
	void setHwnd(HWND hwnd);

	/** File Management				***************/
public:
	CString loadProtocol(CString filePath);
	CString loadProtocolFromData(CString protocolData);
	bool isCompileSuccess(CString res);
	void generateActionList(vector<ActionBeans> &returnValue);

	/** Operation					***************/
private:
	bool isActionFinished();
	void runNextAction();
	void resetAction();

public:
	void start();
	bool runTask();
	void stop();

	/** For Waiting operation		***************/
public:
	bool isLimitSwitchPushed();
	HWND getSafeHwnd();
	int getWaitingTime();
	void setWaitEnded();
	bool isWaitEnded();

	int getSecondWaitingTime();
	void setSecondWaitEnded();
	bool isSecondWaitEnded();

	bool isIdle();
	bool isCompileEnded();
	LPARAM getCurrentAction();
	MotorType::Enum getCurrentMotor();
	ActionCmd::Enum getCurrentActionCmd();
	int getCurrentfilter();
	int getCurrentCmd();
	int getTotalActionNumber();
	int getCurrentActionNumber();

	// 190318 YJ for temp value
	int chamberDiskOffset;

	// siri 151207
public:
	bool isTargetTemp;
	void CMagneto::setIsTargetTemp(bool isTargetTemp);
	bool CMagneto::getIsStarted();
	int CMagneto::getProtocolLength();

	int t_temp;
	
	void Alarmreset();
	bool CMagneto::FilterOperation();

	bool CMagneto::isFilterActionFinished();
	void CMagneto::runFilterAction(int absPos);

	long getSerialNumber();
};