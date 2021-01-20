
#include "stdafx.h"
#include "Magneto.h"
#include "EziMOTIONPlusR\FAS_EziMOTIONPlusR.h"
#include "FileManager.h"

#include "setupapi.h"

#pragma comment (lib, "setupapi.lib")

CMagneto::CMagneto()
	: connected(false)
	, comPortNo(-1)
	, driverErrCnt(0)
	, currentAction(0)
	, currentSubAction(-1)
	, isStarted(false)
	, isWaitEnd(false)
	, isCompileEnd(false)
	, waitCounter(0)
	, currentTargetPos(0.0)
	, currnetPos(0.0)
	, isTargetTemp(false)
	, chamberDiskOffset(M_CHAMBER_DISK_OFFSET)
{
	initPredefinedAction();
}

CMagneto::~CMagneto(){
	
}

void CMagneto::searchPort(vector<CString> &portList)
{
	for (int i = 1; i<30; i++)
	{
		CString portName;
		portName.Format(L"\\\\.\\COM%d", i + 1);

		HANDLE hComm = CreateFile(portName,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		COMMPROP pProp;
		GetCommProperties(hComm, &pProp);

		// dwSettableBaud : 268856176 or dwSettableData : 12 
		if (hComm != INVALID_HANDLE_VALUE && pProp.dwProvSubType == PST_RS232)
		{
			portName.Format(L"COM%d", i + 1);
			portList.push_back(portName);
		}

		CloseHandle(hComm);
	}
}

void CMagneto::searchPortByReg(vector<CString>& portList) {
	HDEVINFO hDevInfo = 0;
	SP_DEVINFO_DATA spDevInfoData = { 0 };

	hDevInfo = SetupDiGetClassDevs(0L, 0L, hwnd, DIGCF_PRESENT | DIGCF_ALLCLASSES | DIGCF_PROFILE);

	if (hDevInfo == (void*)-1) {
		return;
	}

	short wIndex = 0;
	spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	while (true) {
		if (SetupDiEnumDeviceInfo(hDevInfo, wIndex, &spDevInfoData)) {
			TCHAR szBuf[MAX_PATH] = { 0 };
			short wImageIdx = 0;
			short wItem = 0;

			if (!SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData, SPDRP_CLASS, 0L, (PBYTE)szBuf, 2048, 0)) {
				wIndex++;
				continue;
			}

			if (_tcscmp(szBuf, L"Ports") == 0) {
				TCHAR szName[64] = { 0 };
				TCHAR szID[LINE_LEN] = { 0 };
				TCHAR szPath[MAX_PATH] = { 0 };
				DWORD dwRequireSize;

				if (!SetupDiGetClassDescription(&spDevInfoData.ClassGuid, szBuf, MAX_PATH, &dwRequireSize)) {
					wIndex++;
					continue;
				}

				if (!SetupDiGetDeviceInstanceId(hDevInfo, &spDevInfoData, szID, LINE_LEN, 0)) {
					wIndex++;
					continue;
				}

				BOOL res = SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData, SPDRP_FRIENDLYNAME, 0, (PBYTE)szName, 63, 0);

				if (!res) {	// second try
					res = SetupDiGetDeviceRegistryProperty(hDevInfo, &spDevInfoData, SPDRP_DEVICEDESC, 0, (PBYTE)szName, 63, 0);

					if (!res) {
						wIndex++;
						continue;
					}
				}

				CString deviceName;
				deviceName.Format(L"%s", szName);

				int startIdx = -1, endIdx = -1;

				startIdx = deviceName.Find(L"(COM");
				endIdx = deviceName.Find(L")");

				// Check the COM Port & COM port name
				if (startIdx != -1 && endIdx != -1 && deviceName.Find(L"USB Serial Port") != -1) {
					CString devicePort = deviceName.Mid(startIdx);
					devicePort.Replace(L"(", L"");
					devicePort.Replace(L")", L"");
					portList.push_back(devicePort);
				}
			}
		}
		else {
			break;
		}

		wIndex++;
	}
}

void CMagneto::setHwnd(HWND hwnd){
	this->hwnd = hwnd;
}

DriverStatus::Enum CMagneto::connect(int comPortNo){
	// Com Port 연결 체크


	if (!FAS_Connect(comPortNo, Magneto::BaudRate))
		return DriverStatus::NOT_CONNECTED;

	connected = true;
	this->comPortNo = comPortNo;
	
	// Magneto 에 맞춰 slave 의 개수를 체크한다.
	for (int i = 0; i < Magneto::MaxSlaves; ++i){
		if (!FAS_IsSlaveExist(comPortNo, i))
			return DriverStatus::TOO_FEW_SLAVES;
	}

	return DriverStatus::CONNECTED;
}

// 연결이 되어 있는 상태에서는 종료하는 명령어를 실행
// 연결이 되지 않은 상태에서는 아무 작업을 수행하지 않음
void CMagneto::disconnect(){
	if (connected && (comPortNo != -1)){
		FAS_Close(comPortNo);
		connected = false;
	}
}

bool CMagneto::isConnected(){
	return connected;
}

long CMagneto::getSerialNumber() {
	long serialNumber;

	if (FAS_GetROMParameter(comPortNo, 0, 12, &serialNumber) != FMM_OK) 
		return 0;
	else
		return serialNumber - 134000000;
}

bool CMagneto::setSerialNumber(long serialNumber) {
	serialNumber += 134000000;

	bool res = FAS_SetParameter(comPortNo, 0, 12, serialNumber) == FMM_OK;

	if (res) {
		// Need to call the function for saving into the ROM.
		return FAS_SaveAllParameters(comPortNo, 0) == FMM_OK;
	}
	else 
		return false;
}

bool CMagneto::isCompileSuccess(CString res){
	return res.Compare(Magneto::CompileMessageOk) == 0;
}

CString CMagneto::loadProtocol(CString filePath){
	CStdioFile file;
	vector<CString> rawProtocol;
	
	try
	{
		BOOL res = file.Open(filePath, CStdioFile::modeRead);

		if (res) {
			CString line;

			while (file.ReadString(line))
			{
				line.MakeLower();
				rawProtocol.push_back(line);
			}
		}
		else {
			return L"Compile Error: 프로토콜 파일이 존재하지 않습니다.";
		}
	}
	catch (CException* e){
		return L"Compile Error: 프로토콜 파일이 존재하지 않습니다.";
	}

	return protocolCompile(rawProtocol);
}

CString CMagneto::loadProtocolFromData(CString protocolData) {
	CStdioFile file;
	vector<CString> rawProtocol;

	// CString to vector
	CString token;
	int count = 0;
	int protocolCount = 0;
	while (AfxExtractSubString(token, protocolData, count, L'\n')) {
		rawProtocol.push_back(token.MakeLower());
		count++;
	}

	return protocolCompile(rawProtocol);
}

CString CMagneto::loadHomeProtocol() {
	vector<CString> rawProtocol;
	rawProtocol.push_back(L"home");
	return protocolCompile(rawProtocol);
}

/**
		GO = 0,
		FILTER = 1,
		MIX = 2,
		WAIT = 3,
		PUMPING_UP = 4,
		PUMPING_DOWN = 5,
		READY = 6, // 
		HOME = 7, // 
		MAX = 7,

		**/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 프로토콜 변경으로 인해 수정해야 함.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CString CMagneto::protocolCompile(vector<CString> &protocol)
{
	CString compileMessage = L"=====Compile Error=====\n";
	// cmd list 를 만들어 command 와 mapping 시킨다.
	static const CString tempCmdList[14] = { L"goto", L"filter", L"mixing", L"waiting", L"pumping up", L"pumping sup", L"pumping down", L"pumping sdown", L"ready", L"home", L"magnet on", L"magnet off", L"heating", L"pcr", };

	// 이전의 Protocol 을 비운다.
	protcolBinary.clear();
	isCompileEnd = false;

	// 비정상적인 파일임을 알림
	if (protocol.size() == 0)
		return L"Compile Error: 비정상인 파일입니다.";

	// 모든 Protocol line 을 읽는다.
	for (int i = 0; i < protocol.size(); ++i)
	{
		int offset = 0;
		CString line = protocol[i].Trim();
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
		else
			protcolBinary.push_back(bin);
	}

	// Compile error message 가 변경되지 않은 경우 성공한 경우
	if (compileMessage.Compare(L"=====Compile Error=====\n") == 0)
	{
		isCompileEnd = true;
		return Magneto::CompileMessageOk;
	}

	return compileMessage;
}

void CMagneto::initPredefinedAction()
{
	// Protocol Command 와 vector 와 mapping 이 되므로 
	// ProtocolCmd namespace 의 enum 순서대로 생성하면 된다.

	// 기존의 preDefinedAction 이 있는 경우, vector 초기화
	if (preDefinedAction.size() != 0)
		preDefinedAction.clear();
	
	// Command 의 길이만큼 vector 에 새로운 ActionBinary 들을 미리 넣어둔다.
	// 변수 값으로 가지지 않고, 바로 생성하여 넣음으로써 memory leak 을 방지.
	for (int i = 0; i < ProtocolCmd::MAX + 1; ++i)
		preDefinedAction.push_back(vector<ActionBinary>());
	
	// GO Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::GO].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::CHAMBER, Magneto::DefaultPos, M_CHAMBER_SPEED));

	// Filter Command argument 값을 넣어준다.
	//preDefinedAction[ProtocolCmd::FILTER].push_back(ActionBinary(ActionCmd::MOVE_INC, 3, MotorType::FILTER, M_FILTER_INTERVAL_PULSE, M_FILTER_SPEED));
	//preDefinedAction[ProtocolCmd::FILTER].push_back(ActionBinary(ActionCmd::MOVE_INC, 3, MotorType::FILTER, Magneto::DefaultPos, M_FILTER_SPEED));
	preDefinedAction[ProtocolCmd::FILTER].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::FILTER, Magneto::DefaultPos, M_FILTER_SPEED));


	// MIX Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::MIX].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, M_PUMPING_POS_MIXING_TOP, M_PUMPING_SPEED));
	preDefinedAction[ProtocolCmd::MIX].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, M_PUMPING_POS_BOTTOM, M_PUMPING_SPEED));

	// WAIT Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::WAIT].push_back(ActionBinary(ActionCmd::WAIT, 1, Magneto::DefaultPos));

	// PUMPING UP Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::PUMPING_UP].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, Magneto::DefaultPos, M_PUMPING_SPEED));
	
	// 170106_2 KSY SUP /  PUMPING SUP Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::PUMPING_SUP].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, Magneto::DefaultPos, M_PUMPING_SLOWLY_SPEED));

	// PUMPING DOWN Command argument 값을 넣어준다.
	//preDefinedAction[ProtocolCmd::PUMPING_DOWN].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, M_PUMPING_POS_BOTTOM, 10000));	// axis(M1), pos(0), speed
	preDefinedAction[ProtocolCmd::PUMPING_DOWN].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, Magneto::DefaultPos, M_PUMPING_SPEED));	// axis(M1), pos(0), speed

	//
	preDefinedAction[ProtocolCmd::PUMPING_SDOWN].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, Magneto::DefaultPos, M_PUMPING_SLOWLY_SPEED));	// axis(M1), pos(0), speed

	// READY Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::READY].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::CHAMBER, -13402, M_CHAMBER_SPEED));//S1-13220// S2-13402//13402 // 5번방에서 4번방으로 변경 (23505 - 32000)
	preDefinedAction[ProtocolCmd::READY].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, M_PUMPING_POS_BOTTOM, M_PUMPING_SPEED));

	// HOME Command argument 값을 넣어준다.

	preDefinedAction[ProtocolCmd::HOME].push_back(ActionBinary(ActionCmd::HOME, 1, MotorType::PUMPING));
	preDefinedAction[ProtocolCmd::HOME].push_back(ActionBinary(ActionCmd::HOME, 1, MotorType::CHAMBER));
	preDefinedAction[ProtocolCmd::HOME].push_back(ActionBinary(ActionCmd::HOME, 1, MotorType::FILTER));
	preDefinedAction[ProtocolCmd::HOME].push_back(ActionBinary(ActionCmd::SECOND_WAIT, 1, 1));//MotorType::CHAMBER //SECOND_WAIT
	preDefinedAction[ProtocolCmd::HOME].push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::CHAMBER, M_CHAMBER_OFFSET, M_CHAMBER_SPEED));
	preDefinedAction[ProtocolCmd::HOME].push_back(ActionBinary(ActionCmd::SECOND_WAIT, 1, 1));//MotorType::CHAMBER //SECOND_WAIT
	preDefinedAction[ProtocolCmd::HOME].push_back(ActionBinary(ActionCmd::MOVE_INC, 3, MotorType::FILTER,  M_FILTER_OFFSET, M_CHAMBER_SPEED));

	// SIRI 151206 - for arduino
	// MAGNET ON Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::MAGNET_ON].push_back(ActionBinary(ActionCmd::MAGNET_ON, 1, MotorType::PUMPING));
	
	// MAGNET OFF Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::MAGNET_OFF].push_back(ActionBinary(ActionCmd::MAGNET_OFF, 1, MotorType::PUMPING));

	// HEATING Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::HEATING].push_back(ActionBinary(ActionCmd::HEATING, 1, INT_MAX));

	// PCR Command argument 값을 넣어준다.
	preDefinedAction[ProtocolCmd::PCR].push_back(ActionBinary(ActionCmd::PCR, 1, INT_MAX));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CMagneto::initDriverParameter() {
	/*
	if (FAS_SetParameter(comPortNo, 0, 17, 20000) != FMM_OK)
		driverErrCnt++;// magnet home speed: 20000 pps
	if (FAS_SetParameter(comPortNo, 1, 17, 20000) != FMM_OK)
		driverErrCnt++;// load home speed: 20000 pps
	if (FAS_SetParameter(comPortNo, 2, 17, 60000) != FMM_OK)
		driverErrCnt++;// Y axis home speed: 60000 pps
	if (FAS_SetParameter(comPortNo, 3, 17, 20000) != FMM_OK)
		driverErrCnt++;// rotate home speed: 미정
	if (FAS_SetParameter(comPortNo, 4, 17, 20000) != FMM_OK)
		driverErrCnt++;// X axis home speed: 20000 pps
	if (FAS_SetParameter(comPortNo, 5, 17, 60000) != FMM_OK)
		driverErrCnt++;// Syringe home speed: 미정
	if (FAS_SetParameter(comPortNo, 4, 21, 1) != FMM_OK)
		driverErrCnt++;		// X axis 의 홈 방향을 ccw 로 설정

	// For filter
	if (FAS_SetParameter(comPortNo, 6, 17, 20000) != FMM_OK)
		driverErrCnt++;// Syringe home speed: 미정
	*/
	//KSY
	// set default
	if (FAS_SetParameter(comPortNo, MotorType::CHAMBER, 13, -134217728) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::CHAMBER, 13, -134217728 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::CHAMBER, 21, 0) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::CHAMBER, 21, 0 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::CHAMBER, 34, 400) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::CHAMBER, 34, 400 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::CHAMBER, 35, 1) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::CHAMBER, 35, 1 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::FILTER, 13, -134217728) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::FILTER, 13, -134217728 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::FILTER, 21, 0) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::FILTER, 21, 0 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::FILTER, 34, 400) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::FILTER, 34, 400 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::FILTER, 35, 1) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::FILTER, 35, 1 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::PUMPING, 13, -134217728) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::PUMPING, 13, -134217728 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::PUMPING, 21, 0) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::PUMPING, 21, 0 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::PUMPING, 34, 400) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::PUMPING, 34, 400 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::PUMPING, 35, 1) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::PUMPING, 35, 1 error", getSerialNumber(), driverErrCnt);
	}

	// set parameter
	if (FAS_ServoEnable(comPortNo, MotorType::CHAMBER, 1) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_ServoEnable MotorType::CHAMBER, 1 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::CHAMBER, 28, 1) != FMM_OK) {		// rotate 축 motion dir 변경(CW->CCW)
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::CHAMBER, 28, 1 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::CHAMBER, 0, 0) != FMM_OK) {		// rotate 축 resolution 변경(500)
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::CHAMBER, 0, 0 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_ServoEnable(comPortNo, MotorType::PUMPING, 1) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_ServoEnable MotorType::PUMPING, 1 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::PUMPING, 21, 1) != FMM_OK) {		// PUMPING 축 home 방향 변경(CCW->CW)
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::PUMPING, 21, 1 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::PUMPING, 17, M_PUMPING_SPEED / 2) != FMM_OK) {	// PUMPING 축 home 속도 변경(50000) //KJD
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::PUMPING, 17 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_SetParameter(comPortNo, MotorType::PUMPING, 18, M_PUMPING_SPEED / 10) != FMM_OK) {	// PUMPING 축 serach speed 변경(10000) //KJD
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_SetParameter MotorType::PUMPING, 18 error", getSerialNumber(), driverErrCnt);
	}
	if (FAS_ServoEnable(comPortNo, MotorType::FILTER, 1) != FMM_OK) {
		driverErrCnt++;
		FileManager::errorLog(L"initDriverParameter FAS_ServoEnable MotorType::FILTER, 1 error", getSerialNumber(), driverErrCnt);
	}
}

void CMagneto::generateActionList(vector<ActionBeans> &returnValue)
{
	// siri - actionList가 있어야 기기 작동함
	// protocolBinaray 에 새로운 G 코드 명령 추가했음
	// 새로운 명령에 대한 preDefinedAction 초기화 해야함
	

	actionList.clear();
	returnValue.clear();

	for (int i = 0; i < protcolBinary.size(); ++i)
	{
		ProtocolBinary pb = protcolBinary[i];

		// 저장할 action 의 부모 command 값을 설정
		ActionData actionExe(pb.cmd);
		CString protocolBean = ProtocolCmd::toString[pb.cmd];
		
		if( pb.arg != -1 )
			protocolBean.Format(L"%s %d", protocolBean, pb.arg);
		else // 170106_2 KSY 마그네토 리스트 창에 FULL 출려 오류 수정 
			protocolBean.Format(L"%s %s", protocolBean, L"FULL");

		ActionBeans actionBean(protocolBean);

		// cmd 값을 predefined action 과 mapping 시킨다.
		vector<ActionBinary> ab = preDefinedAction[pb.cmd];
		

		for (int j = 0; j < ab.size(); ++j)
		{
			ActionBinary tempAb = ab[j];
			CString command = ActionCmd::toString[tempAb.cmd];
			CString motor = MotorType::toString[tempAb.args[0]];
			CString child;

			// GOTO 의 경우 arg 값을 chamber 에 따른 x, y 에 따라 다른 값을 설정해줘야 한다.
			// arg[1] 값은 position 값인데, default 로 되어 있을 경우, 정해져 있는 position 값으로 설정하도록 한다.
			if (pb.cmd == ProtocolCmd::GO){
				static int _hallNum = 4;
				int chamber = pb.arg;
				int position = 0;
				
				if (chamber == 0)		// 홀 테스트용 4번방 잡을때 사용
					position = -13212;
				else if (_hallNum < chamber) {
					// 190318 YJ removed
					// position = (chamber*M_CHAMBER_INTERVAL) + M_CHAMBER_DIFF + M_CHAMBER_OFFSET - M_CHAMBER_DISK_OFFSET - 50 - 32000; //  1->4 으로 갈때 간격 발생으로 보정(하드코딩)
					position = (chamber*M_CHAMBER_INTERVAL) + M_CHAMBER_DIFF + M_CHAMBER_OFFSET - chamberDiskOffset - 50 - 32000; //  1->4 으로 갈때 간격 발생으로 보정(하드코딩)
				}
				else if (_hallNum >= chamber) {
					// position = (chamber*M_CHAMBER_INTERVAL) + M_CHAMBER_DIFF + M_CHAMBER_OFFSET - M_CHAMBER_DISK_OFFSET - 167 - 32000;
					position = (chamber*M_CHAMBER_INTERVAL) + M_CHAMBER_DIFF + M_CHAMBER_OFFSET - chamberDiskOffset - 167 - 32000;
				}
					
				_hallNum = chamber;
				if( chamber == 0)
					_hallNum = 4;

				if (tempAb.args[1] == Magneto::DefaultPos){
					// args[0] 값은 어떤 모터를 설정할지 설정하는 값으로, x, y axis 에 따라 값을 설정한다.
					if (tempAb.args[0] == MotorType::CHAMBER)
						tempAb.args[1] = position;
				}
			}
			else if (pb.cmd == ProtocolCmd::FILTER){
				int filterNo = pb.arg;
				int position = 0;
				position = M_FILTER_INTERVAL_PULSE * (filterNo-1) + M_FILTER_OFFSET; //+ M_FILTER_OFFSET; // (filter*M_FILTER_INTERVAL) + M_FILTER_OFFSET;
				

				if (tempAb.args[1] == Magneto::DefaultPos){
					// args[0] 값은 어떤 모터를 설정할지 설정하는 값으로, x, y axis 에 따라 값을 설정한다.
					if (tempAb.args[0] == MotorType::FILTER)
						tempAb.args[1] = position;
				}

			}
			else
			{
				// 전체 argument 목록 중에 DEFAULT_POS 값으로 설정된 것이 있으면 argument 값으로 설정해준다.

				for (int k = 0; k < tempAb.args.size(); ++k){
					if (tempAb.args[k] == Magneto::DefaultPos)
					{
						if (pb.cmd == ProtocolCmd::PUMPING_UP || pb.cmd == ProtocolCmd::PUMPING_SUP)//if (pb.cmd == ProtocolCmd::PUMPING_UP)
						{
							int volume = pb.arg;
							tempAb.args[k] = M_PUMPING_POS_BOTTOM - (M_PUMPING_STEP_PER_REV * volume) / (PI * M_PUMPING_DISK_RADIUS * M_PUMPING_DISK_RADIUS) + M_PUMPING_POS_UP_OFFSET;
							if (tempAb.args[k] < M_PUMPING_POS_TOP)
								tempAb.args[k] = M_PUMPING_POS_TOP;
						}
						else if (pb.cmd == ProtocolCmd::PUMPING_DOWN || pb.cmd == ProtocolCmd::PUMPING_SDOWN) { //else if (pb.cmd == ProtocolCmd::PUMPING_DOWN){
							int volume = pb.arg;
							if (volume == -1)
							{
								tempAb.args[k] = M_PUMPING_POS_BOTTOM;
							}
							else
							{
								tempAb.args[k] = M_PUMPING_POS_BOTTOM - (M_PUMPING_STEP_PER_REV * volume) / (PI * M_PUMPING_DISK_RADIUS * M_PUMPING_DISK_RADIUS) + M_PUMPING_POS_UP_OFFSET;
								if (tempAb.args[k] > M_PUMPING_POS_BOTTOM)
									tempAb.args[k] = M_PUMPING_POS_BOTTOM;
							}
						}
						else
							tempAb.args[k] = pb.arg;

					}
				}
			}
			
			if (tempAb.cmd == ActionCmd::WAIT && tempAb.args.size() == 1)
				child.Format(L"%s, Wait(min): %d", command, tempAb.args[0]);
			else if (tempAb.cmd == ActionCmd::SECOND_WAIT && tempAb.args.size() == 1)
				child.Format(L"%s, Wait(sec): %d", command, tempAb.args[0]);
			else if (tempAb.args.size() == 1)
				child.Format(L"%s, %s", command, motor);
			else if (tempAb.args.size() == 3){
				child.Format(L"%s, %s, %0.3f, %d",
					command, motor, pulse2mili(((MotorType::Enum)tempAb.args[0]), tempAb.args[1]), tempAb.args[2]);
			}
			else
				child.Format(L"%s, %s", command, motor);


			// actionList 에 저장하기 위해, childAction 에 현재 값을 저장함
			actionExe.actions.push_back(tempAb);
			actionBean.childAction.push_back(child);
		}

		// mix 값의 경우는 arg 에 mix 횟수 값이 있으므로, 해당 mixing 값만큼 정해진 mixing routing 을 반복한다.
		if (pb.cmd == ProtocolCmd::MIX)
		{
			CString child;
			for (int j = 0; j < pb.arg; ++j)
			{
				// bottom, top 순서로 집어넣는다.
				actionExe.actions.push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, M_PUMPING_POS_MIXING_TOP, M_PUMPING_MIXING_SPEED));
				actionExe.actions.push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, M_PUMPING_POS_BOTTOM, M_PUMPING_MIXING_SPEED));
				
				
				// for return value
				child.Format(L"%s, %s, %0.3f, %d", 
					ActionCmd::toString[ActionCmd::MOVE_ABS], MotorType::toString[MotorType::PUMPING], M_MAGNET_PUSLE2MILI(M_PUMPING_POS_MIXING_TOP), M_PUMPING_MIXING_SPEED);
				actionBean.childAction.push_back(child);
				child.Format(L"%s, %s, %0.3f, %d",
					ActionCmd::toString[ActionCmd::MOVE_ABS], MotorType::toString[MotorType::PUMPING], M_MAGNET_PUSLE2MILI(M_PUMPING_POS_BOTTOM), M_PUMPING_MIXING_SPEED);
				actionBean.childAction.push_back(child);
			}

			// origin 으로 이동하는 routine 을 삽입
			actionExe.actions.push_back(ActionBinary(ActionCmd::MOVE_ABS, 3, MotorType::PUMPING, M_PUMPING_POS_BOTTOM, M_PUMPING_MIXING_SPEED));
			child.Format(L"%s, %s, %0.3f, %d",
				ActionCmd::toString[ActionCmd::MOVE_ABS], MotorType::toString[MotorType::PUMPING], M_MAGNET_PUSLE2MILI(M_PUMPING_POS_BOTTOM), M_PUMPING_MIXING_SPEED);
			actionBean.childAction.push_back(child);
		}

		returnValue.push_back(actionBean);
		actionList.push_back(actionExe);
	}
}

//211019 KBH remove unused code 
//bool CMagneto::isLimitSwitchPushed()
//{
//	return false; //KJD 200804 limit switch가 아예 없으니까 이 기능을 없앤다.
//	//// slave 들에 대해 limit switch 들을 체크해본다.
//	//EZISTEP_MINI_AXISSTATUS axisStatus;
//	//for (int i = 0; i < Magneto::MaxSlaves; ++i){
//	//	if (FAS_GetAxisStatus(comPortNo, i, &axisStatus.dwValue) != FMM_OK){
//	//		driverErrCnt++;
//	//		FileManager::errorLog(L"isLimitSwitchPushed FAS_GetAxisStatus error", getSerialNumber(), driverErrCnt);
//	//		return true;
//	//	}
//	//	if (axisStatus.FFLAG_HWPOSILMT || axisStatus.FFLAG_HWNEGALMT)
//	//		return true;
//	//}
//
//	//return false;
//}

bool CMagneto::motorIsStucked()
{
	//// slave 들에 대해 Stuck를 체크해본다.
	EZISTEP_MINI_AXISSTATUS axisStatus;

	for (int i = 0; i < Magneto::MaxSlaves; ++i){
		if (FAS_GetAxisStatus(comPortNo, i, &axisStatus.dwValue) != FMM_OK){
			driverErrCnt++;
			return true;
		}
		// fas_motor error (0x00000001)
		if (axisStatus.FFLAG_ERRORALL)
		{
			// motor palus error (0x00000400)
			if (axisStatus.FFLAG_ERRSTEPOUT)
			{
				FileManager::errorLog(L"motorIsStucked FFLAG_ERRSTEPOUT error", getSerialNumber(), driverErrCnt);
			}

			return true;
		}
	}

	
	return false;
}

bool CMagneto::isActionFinished()
{
	ActionData action = actionList[currentAction];
	ActionBinary ab = action.actions[currentSubAction];

	// axis 상태를 받아온다.
	EZISTEP_MINI_AXISSTATUS axisStatus;
	/*
	CString out;
	out.Format(L"%d %d\n", ab.cmd, ab.args[0]);
	::OutputDebugString(out);
	*/
	
	if( !( (ab.cmd == ActionCmd::PCR) || (ab.cmd == ActionCmd::MAGNET_ON) || (ab.cmd == ActionCmd::HEATING) 
		|| (ab.cmd == ActionCmd::MAGNET_OFF) || (ab.cmd == ActionCmd::SECOND_WAIT) || (ab.cmd == ActionCmd::WAIT) ) )
	{
		if (FAS_GetAxisStatus(comPortNo, ab.args[0], &axisStatus.dwValue) != FMM_OK)
		{
			driverErrCnt++;
			FileManager::errorLog(L"isActionFinished FAS_GetAxisStatus error", getSerialNumber(), driverErrCnt);
			return true;
		}
	}

	switch (ab.cmd)
	{
		case ActionCmd::MOVE_ABS:
		case ActionCmd::MOVE_INC:
		case ActionCmd::MOVE_DEC:
		case ActionCmd::HOME:
			return !axisStatus.FFLAG_MOTIONING;
		case ActionCmd::WAIT:
			return isWaitEnd;
		case ActionCmd::SECOND_WAIT:
			return isSecondWaitEnd;
		case ActionCmd::HEATING:
			return isTargetTemp;
		case ActionCmd::MAGNET_ON:
		case ActionCmd::MAGNET_OFF:
			return true;
	}

	return false;
}

/******For Waiting Command Counter**********/
UINT waitThread(LPVOID pParam)
{
	CMagneto *magneto = (CMagneto *)pParam;
	HWND hwnd = magneto->getSafeHwnd();
	long startTime = timeGetTime();
	int waitTime = magneto->getWaitingTime();
	
	while (true)
	{
		Sleep(1000);
		double elapsedTime = ((double)(timeGetTime()-startTime)/1000.);

		::SendMessage(hwnd, WM_WAIT_TIME_CHANGED, waitTime, elapsedTime);

		if (magneto->isIdle())
			break;

		if (elapsedTime >= (waitTime))//if (elapsedTime >= (waitTime*60)) //170106_2 KSY
			break;
	}

	magneto->setWaitEnded();

	return 0;
}
UINT secondwaitThread(LPVOID pParam)
{
	CMagneto *magneto = (CMagneto *)pParam;
	HWND hwnd = magneto->getSafeHwnd();

	long startTime = timeGetTime();
	int secondwaitTime = magneto->getSecondWaitingTime();
	while (true)
	{
		Sleep(1000);
		double elapsedTime = ((double)(timeGetTime() - startTime) / 1000.);

		::SendMessage(hwnd, WM_SECOND_WAIT_TIME_CHANGED, secondwaitTime, elapsedTime);
		

 		if (magneto->isIdle())
			break;

		if (elapsedTime >= secondwaitTime)
			break;

	}
	magneto->setSecondWaitEnded();
	return 0;
}

/******For Waiting Command Counter**********/
//KSY: 전북대 소스 함수부분 startAction 함수 부분을 구현되어 있는 부분
void CMagneto::runNextAction()
{
	currentSubAction++;

	// 마지막 action 인지 체크 
	if (currentSubAction == actionList[currentAction].actions.size()){
		currentSubAction = 0;
		currentAction++;

		if (currentAction == actionList.size()){
			// start flag 를 끔으로써, emergency stop 이 호출되지 않도록 설정
			isStarted = false;

			stop();
			return;
		}
	}

	ActionData action = actionList[currentAction];
	ActionBinary ab = action.actions[currentSubAction];

	int cmd = ab.cmd;
	int slaveNo = ab.args[0];
	int cmdPos = 0;
	int velocity = 0;

	// 3개의 command 만 추가적인 arg 가 존재한다.
	// args 의 size 로 체크를 해도 됨(3개인 경우)
	if (cmd == ActionCmd::MOVE_ABS || cmd == ActionCmd::MOVE_DEC || cmd == ActionCmd::MOVE_INC)
	{
		cmdPos = ab.args[1];
		velocity = ab.args[2];
	}

	// Command 에 따라 motor driver 에 명령을 한다.
	switch (cmd)
	{
		case ActionCmd::HOME:
			if (FAS_MoveOriginSingleAxis(comPortNo, slaveNo) != FMM_OK) {
				driverErrCnt++;
				FileManager::errorLog(L"runNextAction FAS_MoveOriginSingleAxis HOME error", getSerialNumber(), driverErrCnt);
			}
		break;
	
		case ActionCmd::MOVE_ABS:
			if (FAS_MoveSingleAxisAbsPos(comPortNo, slaveNo, cmdPos, velocity) != FMM_OK) {
				driverErrCnt++;
				FileManager::errorLog(L"runNextAction FAS_MoveSingleAxisAbsPos ABS error", getSerialNumber(), driverErrCnt);
			}
			break;
		case ActionCmd::MOVE_INC:
			if (FAS_MoveSingleAxisIncPos(comPortNo, slaveNo, cmdPos, velocity) != FMM_OK) {
				driverErrCnt++;
				FileManager::errorLog(L"runNextAction FAS_MoveSingleAxisIncPos INC error", getSerialNumber(), driverErrCnt);
			}
			break;
		case ActionCmd::MOVE_DEC:
			if (FAS_MoveSingleAxisIncPos(comPortNo, slaveNo, cmdPos, velocity) != FMM_OK) {
				driverErrCnt++;
				FileManager::errorLog(L"runNextAction FAS_MoveSingleAxisIncPos DEC error", getSerialNumber(), driverErrCnt);
			}
			break;

		case ActionCmd::PCR:
			// 미구현
			break;
		case ActionCmd::HEATING:
			// 제거
			break;
		case ActionCmd::MAGNET_ON:

			if (FAS_SetIOOutput(comPortNo, slaveNo, SERVO_OUT_BITMASK_USEROUT0, 0) != FMM_OK) {
				driverErrCnt++;
				FileManager::errorLog(L"runNextAction FAS_SetIOOutput MAGNET_ON error", getSerialNumber(), driverErrCnt);
			}
			break;
		case ActionCmd::MAGNET_OFF:

			if (FAS_SetIOOutput(comPortNo, slaveNo, 0, SERVO_OUT_BITMASK_USEROUT0) != FMM_OK) {
				driverErrCnt++;
				FileManager::errorLog(L"runNextAction FAS_SetIOOutput MAGNET_OFF error", getSerialNumber(), driverErrCnt);
			}
			break;
		case ActionCmd::SECOND_WAIT:
		{
			secondwaitCounter = ab.args[0];
			isSecondWaitEnd = false;
			CWinThread *thread1 = ::AfxBeginThread(secondwaitThread, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
			thread1->m_bAutoDelete = TRUE;
			thread1->ResumeThread();
			break;
		}
		case ActionCmd::WAIT:
		{
			waitCounter = ab.args[0];
			isWaitEnd = false;

			CWinThread *thread = ::AfxBeginThread(waitThread, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
			thread->m_bAutoDelete = TRUE;
			thread->ResumeThread();
			break;
		}
	}
}

void CMagneto::resetAction()
{
	currentAction = 0;
	currentSubAction = -1;
	isWaitEnd = false;
	isSecondWaitEnd = false;
	driverErrCnt = 0;
	waitCounter = 0;
	secondwaitCounter = 0;
	
}

HWND CMagneto::getSafeHwnd(){
	return hwnd;
}

int CMagneto::getWaitingTime(){
	return waitCounter;
}

void CMagneto::setWaitEnded(){
	isWaitEnd = true;
}

bool CMagneto::isWaitEnded(){
	return isWaitEnd;
}

int CMagneto::getSecondWaitingTime(){
	return secondwaitCounter;
}
void CMagneto::setSecondWaitEnded(){
	isSecondWaitEnd = true;
}
bool CMagneto::isSecondWaitEnded(){

	return isSecondWaitEnd;
}

bool CMagneto::isIdle(){
	return !isStarted;
}

bool CMagneto::isCompileEnded(){
	return isCompileEnd;
}

LPARAM CMagneto::getCurrentAction(){
	return MAKELPARAM(currentAction, currentSubAction);
}

MotorType::Enum CMagneto::getCurrentMotor(){
	return (MotorType::Enum)actionList[currentAction].actions[currentSubAction].args[0];
}

//ActionCmd
ActionCmd::Enum CMagneto::getCurrentActionCmd(){
	return (ActionCmd::Enum)actionList[currentAction].actions[currentSubAction].cmd;
}
int CMagneto::getCurrentfilter()
{
	return protcolBinary[currentAction].arg;
}
int CMagneto::getCurrentCmd()
{
	return protcolBinary[currentAction].cmd;
}

void CMagneto::start()
{
	if (!isStarted)
	{
		initDriverParameter();
		Sleep(500);
		resetAction();
		runNextAction();
		isStarted = true;
	}
}

bool CMagneto::runTask()
{
	// 210119 KBH remove unused code 
	//if (isLimitSwitchPushed()) {
	//	stop();
	//	return false;
	//}

	// 210119 KBH Motor Error check 
	if (motorIsStucked())
	{
		stop();
		return false;
	}

	ActionData action = actionList[currentAction];
	ActionBinary ab = action.actions[currentSubAction];

	// Motor position 값 통지
	long tempPos = 0;
	double cmdPos = 0.0, targetPos = 0.0;

	if (FAS_GetCommandPos(comPortNo, ab.args[0], &tempPos) != FMM_OK) 
	{
		driverErrCnt++;
		
		FileManager::errorLog(L"runTask - GetCommandPos error", getSerialNumber(), driverErrCnt);
	}

	cmdPos = pulse2mili((MotorType::Enum)ab.args[0], tempPos);
	if (ab.args.size() >= 2)
		targetPos = pulse2mili((MotorType::Enum)ab.args[0], ab.args[1]);

	MotorPos motorPos = { targetPos, cmdPos };

	::SendMessage(hwnd, WM_MOTOR_POS_CHANGED, reinterpret_cast<WPARAM>(&motorPos), getCurrentAction());

	// Action 의 종료 상태를 체크
	if (!isActionFinished())
		return true;

	runNextAction();

	return true;
}

void CMagneto::stop() {
	if (isStarted)
	{
		FAS_AllEmergencyStop(comPortNo);
	}

	resetAction();
	isStarted = false;
}

void CMagneto::setIsTargetTemp(bool isTargetTemp)
{
	this->isTargetTemp = isTargetTemp;
}

bool CMagneto::getIsStarted()
{
	return isStarted;
}

int CMagneto::getProtocolLength()
{
	return protcolBinary.size();
}

void CMagneto::Alarmreset()
{
	FAS_ServoEnable(comPortNo, MotorType::CHAMBER, 0);
	FAS_ServoEnable(comPortNo, MotorType::PUMPING, 0);
	FAS_ServoAlarmReset(comPortNo, MotorType::CHAMBER) != FMM_OK;

	FAS_ServoAlarmReset(comPortNo, MotorType::FILTER) != FMM_OK;

	FAS_ServoAlarmReset(comPortNo, MotorType::PUMPING) != FMM_OK;
}


bool CMagneto::isFilterActionFinished()
{
	DWORD dwAxisStatus;
	EZISERVO_AXISSTATUS stAxisStatus;

	int nRtn = FAS_GetAxisStatus(comPortNo, MotorType::FILTER, &dwAxisStatus);
	if (nRtn == FMM_OK)
	{
		stAxisStatus.dwValue = dwAxisStatus;
	}

	return stAxisStatus.FFLAG_MOTIONING;
}

void CMagneto::runFilterAction(int absPos)
{
	int position = 0;
	position = M_FILTER_INTERVAL_PULSE * (absPos)+M_FILTER_OFFSET; //+ M_FILTER_OFFSET; // (filter*M_FILTER_INTERVAL) + M_FILTER_OFFSET;

	if (FAS_MoveSingleAxisAbsPos(comPortNo, MotorType::FILTER, position, M_FILTER_SPEED) != FMM_OK) {
		driverErrCnt++;

		FileManager::errorLog(L"runFilterAction - FAS_MoveSingleAxisAbsPos error", getSerialNumber(), driverErrCnt);
	}
}

int CMagneto::getTotalActionNumber() {
	return actionList.size();
}

int CMagneto::getCurrentActionNumber() {
	return currentAction;
}