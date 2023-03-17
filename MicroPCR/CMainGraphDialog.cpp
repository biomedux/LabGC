﻿// CMainDialog.cpp: 구현 파일
//

#include "stdafx.h"

#include "CMainGraphDialog.h"
#include "afxdialogex.h"
#include "resource.h"

#include "PasswordDialog.h"
#include "SetupDialog.h"
#include "DeviceSetup.h"
#include "FileManager.h"
#include "ProgressThread.h"
#include "ConfirmDialog.h"

#include <numeric>


// CMainDialog 대화 상자

IMPLEMENT_DYNAMIC(CMainGraphDialog, CDialogEx)

CMainGraphDialog::CMainGraphDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_MAIN_GRAPH, pParent)
	, lastSelectedProtocol(L"")
	, isProtocolLoaded(false)
	, isConnected(false)
	, isStarted(false)
	, magnetoProtocolIdx(0)
	, isFirstDraw(false)
	, isCompletePCR(false)
	, isTargetArrival(false)
	, m_prevTargetTemp(25)
	, m_currentTargetTemp(25)
	, m_kp(0.0)
	, m_ki(0.0)
	, m_kd(0.0)
	, m_cArrivalDelta(0.5)
	, maxCycles(40)
	, compensation(0)
	, integralMax(INTGRALMAX)
	, displayDelta(0.0f)
	, flRelativeMax(FL_RELATIVE_MAX)
	, m_timerCounter(0)
	, totalLeftSec(0)
	, leftSec(0)
	, m_currentActionNumber(-1)
	, targetTempFlag(false)
	, m_timeOut(0)
	, m_leftGotoCount(-1)
	, filterIndex(0)
	, filterRunning(false)
	, shotCounter(0)
	, ledControl_wg(1)
	, ledControl_r(1)
	, ledControl_g(1)
	, ledControl_b(1)
	, emergencyStop(false)
	, freeRunning(false)
	, freeRunningCounter(0)
	, currentCycle(0)
	, recordingCount(0)
	, logStopped(false)
	, useFam(false)
	, useHex(false)
	, useRox(false)
	, useCy5(false)
	, m_strStylesPath(L"./")
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	XTPSkinManager()->SetApplyOptions(XTPSkinManager()->GetApplyOptions() | xtpSkinApplyMetrics);
	XTPSkinManager()->LoadSkin(m_strStylesPath + _T("QuicksilverR.cjstyles"));

}

CMainGraphDialog::~CMainGraphDialog()
{
	if (device != NULL)
		delete device;
	if (m_Timer != NULL)
		delete m_Timer;
	if (magneto != NULL)
		delete magneto;
}

void CMainGraphDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CUSTOM_RESULT_TABLE, resultTable);

	DDX_Control(pDX, IDC_COMBO_PROTOCOLS, protocolList);
	DDX_Control(pDX, IDC_COMBO_DEVICE_LIST, deviceList);
	DDX_Control(pDX, IDC_PROGRESS_STATUS, progressStatus);
	// progressStatus
}

BEGIN_MESSAGE_MAP(CMainGraphDialog, CDialogEx)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTON_START, &CMainGraphDialog::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_SETUP, &CMainGraphDialog::OnBnClickedButtonSetup)
	ON_LBN_SELCHANGE(IDC_COMBO_PROTOCOLS, &CMainGraphDialog::OnLbnSelchangeComboProtocols)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CMainGraphDialog::OnBnClickedButtonConnect)
	ON_WM_TIMER()
	ON_MESSAGE(WM_MMTIMER, OnmmTimer)

	ON_BN_CLICKED(IDC_BUTTON_FILTER_FAM, &CMainGraphDialog::OnBnClickedButtonFilterFam)
	ON_BN_CLICKED(IDC_BUTTON_FILTER_HEX, &CMainGraphDialog::OnBnClickedButtonFilterHex)
	ON_BN_CLICKED(IDC_BUTTON_FILTER_ROX, &CMainGraphDialog::OnBnClickedButtonFilterRox)
	ON_BN_CLICKED(IDC_BUTTON_FILTER_CY5, &CMainGraphDialog::OnBnClickedButtonFilterCy5)
END_MESSAGE_MAP()

// CMainDialog 메시지 처리기
BOOL CMainGraphDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	SetDlgItemText(IDC_EDIT_CONNECTI_STATUS, L"Disconnected");
	SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Idle..");
	// 210910 KBH set Default Gender
	((CButton*)GetDlgItem(IDC_RADIO_GENDER_M))->SetCheck(True);

	progressStatus.SetRange(0, 100);

	SetIcon(m_hIcon, TRUE);	 // set Large Icon
	SetIcon(m_hIcon, FALSE); // set Small Icon

	// Initialize the chart
	initChart();

	// Initialize the connection information
	initConnection();

	// initialize the device list
	initPCRDevices();

	// Initialize UI
	loadConstants();
	loadProtocolList();

	initResultTable();

	// 210910 KBH initialize database table
	initDatabaseTable();

#ifdef EMULATOR
	AfxMessageBox(L"이 프로그램은 에뮬레이터 장비용 입니다.");
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CMainGraphDialog::OnBnClickedButtonSetup()
{
	CPasswordDialog passwordDialog;

	int res = passwordDialog.DoModal();
	if (res == IDOK) {

		// biomedux setup menu
		if (passwordDialog.resultType == PASSWORD_RESULT_BIOMEDUX) {
			SetupDialog dlg;
			dlg.DoModal();


			// Reload the protocol list
			loadProtocolList();
			loadConstants();
			initResultTable();
		}
		else if (passwordDialog.resultType == PASSWORD_RESULT_DEVICE_SETUP) {
			// Check connection state
			if (isConnected) {
				AfxMessageBox(L"Can't setup the device when the device is running.");
				return;
			}

			CDeviceSetup dlg;
			dlg.DoModal();
		}
	}
}

BOOL CMainGraphDialog::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
			return TRUE;

		if (pMsg->wParam == VK_ESCAPE)
			return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CMainGraphDialog::initChart() {
	CAxis* axis;
	axis = m_Chart.AddAxis(kLocationBottom);
	axis->m_TitleFont.lfWidth = 20;
	axis->m_TitleFont.lfHeight = 15;
	axis->SetTitle(L"PCR Cycles");
	axis->SetRange(0, maxCycles);
	axis = m_Chart.AddAxis(kLocationLeft);
	axis->m_TitleFont.lfWidth = 20;
	axis->m_TitleFont.lfHeight = 15;
	
	// 210910 KBH chage tick range
	//axis->SetTitle(L"Sensor Value");

	//axis->SetTickCount(8); // 210203 KBH tick count : 8
	//axis->SetRange(-512, 4096);	// 210203 KBH Y-range lower : 0 -> -512
	
	//210830 KJD Setting Axis and m_Chart
	axis->SetRange(-100, 1600);
	axis->SetTickCount(3);
	axis->m_ytickPos[0] = 0;
	axis->m_ytickPos[1] = 500;
	axis->m_ytickPos[2] = 1000;
	axis->m_ytickPos[3] = 1500;
	m_Chart.m_UseMajorVerticalGrids = TRUE;
	m_Chart.m_UseMajorHorizontalGrids = TRUE;
	m_Chart.m_MajorGridLineStyle = PS_DOT;
	m_Chart.m_BackgroundColor = GetSysColor(COLOR_3DFACE);

	// Load bitmap
	offImg.LoadBitmapW(IDB_BITMAP_OFF);
	famImg.LoadBitmapW(IDB_BITMAP_FAM);
	hexImg.LoadBitmapW(IDB_BITMAP_HEX);
	roxImg.LoadBitmapW(IDB_BITMAP_ROX);
	cy5Img.LoadBitmapW(IDB_BITMAP_CY5);

	SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_FAM, offImg);
	SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_HEX, offImg);
	SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_ROX, offImg);
	SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_CY5, offImg);

	// 210910 KBH calc graph rect (IDC_GROUP_CONNECTION(right, top), IDC_GROUP_CT(right, top) ->  Graph(left, top, right, bottom)
	// static position -> dynamic position 
	
	GetDlgItem(IDC_STATIC_PLOT)->GetWindowRect(&m_graphRect);
	ScreenToClient(&m_graphRect);

	CPaintDC dc(this); 
	dc.DPtoLP((LPPOINT)&m_graphRect, 2);
}

void CMainGraphDialog::OnPaint() {
	CPaintDC dc(this); // device context for painting
					   // TODO: 여기에 메시지 처리기 코드를 추가합니다.
					   // 그리기 메시지에 대해서는 CDialogEx::OnPaint()을(를) 호출하지 마십시오.

	if (IsIconic())
	{
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else {
		CRect graphRect(m_graphRect.left, m_graphRect.top, m_graphRect.right, m_graphRect.bottom);

		int oldMode = dc.SetMapMode(MM_LOMETRIC);
		//graphRect.SetRect(15, 130, 470, 500); // 210910 KBH static position -> dynamic position 

		dc.DPtoLP((LPPOINT)&graphRect, 2);
		CDC* dc2 = CDC::FromHandle(dc.m_hDC);
		m_Chart.OnDraw(dc2, graphRect, graphRect);
		dc.SetMapMode(oldMode);
	}

}

HCURSOR CMainGraphDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static const int RESULT_TABLE_COLUMN_WIDTHS[2] = { 100, 130 };

void CMainGraphDialog::initResultTable() {
	resultTable.SetListMode(true);

	resultTable.DeleteAllItems();

	resultTable.SetRowCount(5);
	resultTable.SetColumnCount(2);
	resultTable.SetFixedRowCount(1);
	resultTable.SetFixedColumnCount(1);
	resultTable.SetEditable(false);
	resultTable.SetRowResize(false);
	resultTable.SetColumnResize(false);

	// 초기 gridControl 의 table 값들을 설정해준다.
	DWORD dwTextStyle = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;

	for (int col = 0; col < resultTable.GetColumnCount(); col++) {
		GV_ITEM item;
		item.mask = GVIF_TEXT | GVIF_FORMAT;
		item.row = 0;
		item.col = col;

		item.nFormat = DT_CENTER | DT_WORDBREAK;
		item.strText = RESULT_TABLE_COLUMNS[col];

		if (col == 0) {
			resultTable.SetRowHeight(col, 25);
		}

		// resultTable.SetItemFont(item.row, item.col, )
		resultTable.SetItem(&item);
		resultTable.SetColumnWidth(col, RESULT_TABLE_COLUMN_WIDTHS[col]);
	}

	vector<CString> labels;

	if (currentProtocol.useFam) {
		labels.push_back(currentProtocol.labelFam);
	}

	if (currentProtocol.useHex) {
		labels.push_back(currentProtocol.labelHex);
	}

	if (currentProtocol.useRox) {
		labels.push_back(currentProtocol.labelRox);
	}

	if (currentProtocol.useCY5) {
		labels.push_back(currentProtocol.labelCY5);
	}

	for (int i = 0; i < labels.size(); ++i) {
		GV_ITEM item;
		item.mask = GVIF_TEXT | GVIF_FORMAT;
		item.row = i + 1;
		item.col = 0;
		item.nFormat = DT_CENTER | DT_WORDBREAK;

		item.strText = labels[i];

		// resultTable.SetItemFont(item.row, item.col, )
		resultTable.SetItem(&item);
	}
}

void CMainGraphDialog::initProtocol() {
	useFam = currentProtocol.useFam;
	useHex = currentProtocol.useHex;
	useRox = currentProtocol.useRox;
	useCy5 = currentProtocol.useCY5;

	if (currentProtocol.useFam) {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_FAM, famImg);
		GetDlgItem(IDC_BUTTON_FILTER_FAM)->EnableWindow();
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_FAM, offImg);
		GetDlgItem(IDC_BUTTON_FILTER_FAM)->EnableWindow(FALSE);
	}

	if (currentProtocol.useHex) {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_HEX, hexImg);
		GetDlgItem(IDC_BUTTON_FILTER_HEX)->EnableWindow();
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_HEX, offImg);
		GetDlgItem(IDC_BUTTON_FILTER_HEX)->EnableWindow(FALSE);
	}

	if (currentProtocol.useRox) {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_ROX, roxImg);
		GetDlgItem(IDC_BUTTON_FILTER_ROX)->EnableWindow();
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_ROX, offImg);
		GetDlgItem(IDC_BUTTON_FILTER_ROX)->EnableWindow(FALSE);
	}

	if (currentProtocol.useCY5) {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_CY5, cy5Img);
		GetDlgItem(IDC_BUTTON_FILTER_CY5)->EnableWindow();
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_CY5, offImg);
		GetDlgItem(IDC_BUTTON_FILTER_CY5)->EnableWindow(FALSE);
	}

	calcTotalTime();
}

void CMainGraphDialog::loadProtocolList() {
	protocolList.ResetContent();

	FileManager::loadProtocols(protocols);

	int idx = 0;
	for (int i = 0; i < protocols.size(); ++i) {
		protocolList.AddString(protocols[i].protocolName);

		if (lastSelectedProtocol.Compare(protocols[i].protocolName) == 0) {
			idx = i;
		}
	}

	if (protocolList.GetCount() > 0) {
		protocolList.SetCurSel(idx);
		// Load the data from protocol
		currentProtocol = protocols[idx];
		initProtocol();
		isProtocolLoaded = true;

		// Enable the window if the device is connected
		if (isConnected) {
			GetDlgItem(IDC_BUTTON_START)->EnableWindow();
		}

		// Getting the magneto data and check magneto data
		loadMagnetoProtocol();
	}
	else {
		AfxMessageBox(L"You need to make the protocol first.");
	}
}
void CMainGraphDialog::initState()
{
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	if (!magneto->isIdle()) {
		magneto->stop();
	}
	
	cleanupTask();

	KillTimer(Magneto::TimerRuntaskID);
}
void CMainGraphDialog::loadMagnetoProtocol() {
	// Getting the magneto data and check magneto data
	CString magnetoProtocolRes = magneto->loadProtocolFromData(currentProtocol.magnetoData);

	if (magneto->isCompileSuccess(magnetoProtocolRes)) {
		// initialize the protocol
		vector<ActionBeans> treeList;
		magneto->generateActionList(treeList);

		int maxActions = magneto->getTotalActionNumber();
		progressStatus.SetRange(0, maxActions);
	}
}

void CMainGraphDialog::loadConstants() {
	FileManager::loadConstants(maxCycles, compensation, integralMax, displayDelta, flRelativeMax, pids);

	// if there is no pids, make initial pid data
	if (pids.empty()) {
		PID pid1(25.0f, 95.0f, 0.0f, 0.0f, 0.0f);
		PID pid2(95.0f, 60.0f, 0.0f, 0.0f, 0.0f);
		PID pid3(60.0f, 72.0f, 0.0f, 0.0f, 0.0f);
		PID pid4(72.0f, 95.0f, 0.0f, 0.0f, 0.0f);
		PID pid5(95.0f, 50.0f, 0.0f, 0.0f, 0.0f);
		pids.push_back(pid1);
		pids.push_back(pid2);
		pids.push_back(pid3);
		pids.push_back(pid4);
		pids.push_back(pid5);
	}

	CAxis* axis = m_Chart.GetAxisByLocation(kLocationBottom);
	axis->SetRange(0, maxCycles);
	//InvalidateRect(&CRect(15, 130, 470, 500)); // 210910 KBH using graphRect 
	InvalidateRect(&m_graphRect, FALSE);

}

void CMainGraphDialog::OnLbnSelchangeComboProtocols() {
	int selectedIdx = protocolList.GetCurSel();
	protocolList.GetLBText(selectedIdx, lastSelectedProtocol);

	// Load the data from protocol
	currentProtocol = protocols[selectedIdx];
	initProtocol();
	initResultTable();

	loadMagnetoProtocol();
}

void CMainGraphDialog::calcTotalTime() {
	totalLeftSec = 0;

	vector<Action> tempActions;

	// Deep copy
	for (int i = 0; i < currentProtocol.actionList.size(); ++i) {
		tempActions.push_back(Action());
		tempActions[i].Label = currentProtocol.actionList[i].Label;
		tempActions[i].Temp = currentProtocol.actionList[i].Temp;
		tempActions[i].Time = currentProtocol.actionList[i].Time;
	}

	for (int i = 0; i < tempActions.size(); ++i) {
		Action action = tempActions[i];

		if (action.Label.Compare(L"GOTO") == 0) {
			CString gotoLabel;
			gotoLabel.Format(L"%d", (int)action.Temp);
			int remain = (int)action.Time - 1;
			tempActions[i].Time = (double)remain;

			if (remain != -1) {
				int gotoIndex = -1;
				for (int j = 0; j < tempActions.size(); ++j) {
					if (tempActions[j].Label.Compare(gotoLabel) == 0) {
						gotoIndex = j;
					}
				}

				i = gotoIndex - 1;
			}
		}
		else if (action.Label.Compare(L"SHOT") != 0) {
			totalLeftSec += (int)action.Time;
		}
	}

	int second = totalLeftSec % 60;
	int minute = totalLeftSec / 60;
	// int hour = minute / 60;
	// minute = minute - hour * 60;

	CString totalTime;

	if (minute == 0)
		totalTime.Format(L"%02ds", second);
	else
		totalTime.Format(L"%02dm %02ds", minute, second);

	// Display total remaining time
	SetDlgItemText(IDC_STATIC_PROGRESS_REMAINING_TIME, totalTime);
}

void CMainGraphDialog::initConnection() {
	m_Timer = new CMMTimers(1, GetSafeHwnd());
	device = new CDeviceConnect(GetSafeHwnd());
	magneto = new CMagneto();
}

void CMainGraphDialog::initPCRDevices() {
	deviceList.ResetContent();

	int deviceNums = device->GetDevices();
	for (int i = 0; i < deviceNums; ++i) {
		CString deviceSerial = device->GetDeviceSerial(i);

		// Remove the QuPCR string
		deviceList.AddString(deviceSerial.Mid(5));
	}

	if (deviceList.GetCount() != 0) {
		deviceList.SetCurSel(0);
	}
}

void CMainGraphDialog::OnBnClickedButtonConnect()
{
	CString buttonState;
	GetDlgItemText(IDC_BUTTON_CONNECT, buttonState);

	// Disable the buttons
	GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(FALSE);

	if (buttonState.Compare(L"Connect") == 0) {
		// Getting the device index
		int selectedIdx = deviceList.GetCurSel();

		if (selectedIdx != -1) {
			CString deviceSerial;
			deviceList.GetLBText(selectedIdx, deviceSerial);
			int usbSerial = _ttoi(deviceSerial);

			// Getting the com port list
			vector<CString> portList;
			magneto->searchPortByReg(portList);

			bool result = false;
			// Check all magneto devices
			for (int i = 0; i < portList.size(); ++i) {
				// Getting the serial number from magneto.
				CString tempPortNumber = portList[i];
				
				tempPortNumber.Replace(L"COM", L"");
				int comPortNumber = _ttoi(tempPortNumber);

				DriverStatus::Enum res = magneto->connect(comPortNumber);

				if (res == DriverStatus::CONNECTED)
				{
					long serialNumber = magneto->getSerialNumber();

					if (serialNumber == usbSerial) {
						result = true;
						break;
					}
					else {
						magneto->disconnect();
					}
				}
			}

			if (result) {
				// Found the same serial number device.
				CStringA pcrSerial;
				pcrSerial.Format("QuPCR%06d", usbSerial);
				char serialBuffer[20];
				sprintf(serialBuffer, "%s", pcrSerial);

				BOOL res = device->OpenDevice(LS4550EK_VID, LS4550EK_PID, serialBuffer, TRUE);

				if (res) {
					// Connection processing
					isConnected = true;
					
					SetDlgItemText(IDC_EDIT_CONNECTI_STATUS, L"Connected");
					SetDlgItemText(IDC_BUTTON_CONNECT, L"Disconnect");
					GetDlgItem(IDC_COMBO_DEVICE_LIST)->EnableWindow(FALSE);

					CString prevTitle;
					GetWindowText(prevTitle);
					prevTitle.Format(L"%s - %s", prevTitle, deviceSerial);
					SetWindowText(prevTitle);

					if (isProtocolLoaded) {
						GetDlgItem(IDC_BUTTON_START)->EnableWindow();
					}
				}
				else {
					AfxMessageBox(L"Magneto is connected but PCR is failed to connect(Unknown error).");
				}
			}
			else {
				AfxMessageBox(L"PCR 과 일치하는 Magneto 장비를 찾을 수 없습니다.");
			}
		}
		else {
			AfxMessageBox(L"Please select the device first.");
		}

	}
	else {
		isConnected = false;
		if (magneto->isConnected()) {
			magneto->disconnect();
		}

		device->CloseDevice();

		CString prevTitle;
		GetWindowText(prevTitle);
		CString left = prevTitle.Left(prevTitle.Find(L")") + 1);
		SetWindowText(left);

		SetDlgItemText(IDC_EDIT_CONNECTI_STATUS, L"Disconnected");
		SetDlgItemText(IDC_BUTTON_CONNECT, L"Connect");
		GetDlgItem(IDC_COMBO_DEVICE_LIST)->EnableWindow();
		GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	}

	GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(TRUE);
}

void CMainGraphDialog::OnBnClickedButtonStart()
{
	if (!magneto->isCompileEnded()) {
		AfxMessageBox(L"Magneto data is not exist. Please setting the magneto protocol.");
		return;
	}
	
	// Add Confirm Dialog
	CString message;
	
	message = !isStarted ? L"프로토콜을 시작하겠습니까?" : L"프로토콜을 중지하겠습니까?";

	ConfirmDialog dialog(message);


	if (dialog.DoModal() != IDOK) {
		return;
	}

	// 210910 KBH  deactivate Controls what inside group Info
	GetDlgItem(IDC_EDIT_USER_ID)->SendMessage(EM_SETREADONLY, true, 0);

	GetDlgItem(IDC_EDIT_USER_NAME)->SendMessage(EM_SETREADONLY, true, 0);
	GetDlgItem(IDC_EDIT_USER_AGE)->SendMessage(EM_SETREADONLY, true, 0);
	GetDlgItem(IDC_EDIT_INSPECTOR)->SendMessage(EM_SETREADONLY, true, 0);
	GetDlgItem(IDC_EDIT_SAMPLE_TYPE)->SendMessage(EM_SETREADONLY, true, 0);
	GetDlgItem(IDC_DATE_SAMPLE)->EnableWindow(false);
	GetDlgItem(IDC_RADIO_GENDER_M)->EnableWindow(false);
	GetDlgItem(IDC_RADIO_GENDER_F)->EnableWindow(false);


// 210203 KBH chip connection check -> 210910 KBH remove chip connection check logic
//#ifndef EMULATOR
//	if (!isStarted)
//	{
//		RxBuffer rx;
//		TxBuffer tx;
//		float currentTemp = 0.0f;
//
//		memset(&rx, 0, sizeof(RxBuffer));
//		memset(&tx, 0, sizeof(TxBuffer));
//
//		tx.cmd = CMD_READY;
//
//		BYTE senddata[65] = { 0, };
//		BYTE readdata[65] = { 0, };
//		memcpy(senddata, &tx, sizeof(TxBuffer));
//
//		device->Write(senddata);
//
//		device->Read(&rx);
//
//		memcpy(readdata, &rx, sizeof(RxBuffer));
//		memcpy(&currentTemp, &(rx.chamber_temp_1), sizeof(float));
//
//		if (currentTemp <= 10.0f)
//		{
//			message = L"Low temperature! Chip connection check!";
//			AfxMessageBox(message);
//			return;
//		}
//	}
//#endif

	// Disable start button
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);

	if (isConnected && isProtocolLoaded) {
		isStarted = !isStarted;
		if (isStarted) {
			progressStatus.SetPos(0);
			GetDlgItem(IDC_COMBO_PROTOCOLS)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(FALSE);
			SetDlgItemText(IDC_BUTTON_START, L"Stop");
			SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Preparing the Magneto..");

			// init all values
			logStopped = false;
			initValues();
			calcTotalTime();
			initLog();

			// initialize the values
			currentCmd = CMD_PCR_RUN;

			m_prevTargetTemp = 25;

			m_currentTargetTemp = (BYTE)currentProtocol.actionList[0].Temp;

			findPID();

			clearChartValue();

			magneto->start();
			SetTimer(Magneto::TimerRuntaskID, Magneto::TimerRuntaskDuration, NULL);

			// Enable stop button
			GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
		}
		else {
			// PCREndTask();
			// Stop the magneto if running
			
			if (!magneto->isIdle()) {
				magneto->stop();
			}

			cleanupTask();

			KillTimer(Magneto::TimerRuntaskID);
		}
	}
	else {
		AfxMessageBox(L"Error occured!");
	}
}

void CMainGraphDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == Magneto::TimerRuntaskID)
	{
		if (!magneto->runTask()) {
			KillTimer(Magneto::TimerRuntaskID);
			// 210119 KBH remove unused code 
			//AfxMessageBox(L"Limit 스위치가 설정되어 task 가 종료되었습니다.\n기기를 확인하세요.");

			// 210119 KBH Motor Stucked
			AfxMessageBox(L"motor가 stuck 되었습니다.\n기기를 확인하세요.");
			initState();	// 210120 KBH state initialize 
			return;
		}

		// Progress
		progressStatus.SetPos(magneto->getCurrentActionNumber());

		// Check magneto command
		if (magneto->getCurrentCmd() == ProtocolCmd::GO) {
			if (magneto->getCurrentfilter() == 1) {
				SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Washing 1...");
			}
			else if (magneto->getCurrentfilter() == 2) {
				SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Washing 2...");
			}
			else if (magneto->getCurrentfilter() == 5) {
				SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Elution...");
			}
			else if (magneto->getCurrentfilter() == 6) {
				SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Lysis...");
			}
//			210118 KBH remobe progress status "Mixing"
//			else if (magneto->getCurrentfilter() == 10) {
//				SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Mixing...");
//			}
			else if (magneto->getCurrentfilter() == 12) {
				SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Moving to PCR...");
			}
		}

		if (magneto->isIdle()) {
			progressStatus.SetPos(magneto->getTotalActionNumber());
			KillTimer(Magneto::TimerRuntaskID);
			SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"PCR in progress...");
			m_Timer->startTimer(TIMER_DURATION, FALSE);
			return;
		}
	}
	else if (nIDEvent == Magneto::TimerCleanupTaskID) {
		if (!magneto->runTask()) {
			KillTimer(Magneto::TimerRuntaskID);
			// 210119 KBH remove unused code 
			//AfxMessageBox(L"Limit 스위치가 설정되어 task 가 종료되었습니다.\n기기를 확인하세요.");

			// 210119 KBH Motor Stucked
			AfxMessageBox(L"motor가 stuck 되었습니다.\n기기를 확인하세요.");
			initState();
			return;
		}

		if (magneto->isIdle()) {
			KillTimer(Magneto::TimerCleanupTaskID);
			PCREndTask();
			// Reload original magneto protocol
			loadMagnetoProtocol();
			return;
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

LRESULT CMainGraphDialog::OnmmTimer(WPARAM wParam, LPARAM lParam) {
	if (isStarted)
	{
		int errorPrevent = 0;
		timeTask();
	}

	RxBuffer rx;
	TxBuffer tx;
	float currentTemp = 0.0;

	memset(&rx, 0, sizeof(RxBuffer));
	memset(&tx, 0, sizeof(TxBuffer));

	tx.cmd = currentCmd;
	tx.currentTargetTemp = (BYTE)m_currentTargetTemp;
	tx.ledControl = 1;
	tx.led_wg = ledControl_wg;
	tx.led_r = ledControl_r;
	tx.led_g = ledControl_g;
	tx.led_b = ledControl_b;
	tx.compensation = compensation;
	tx.currentCycle = currentCycle;

	// pid 값을 buffer 에 복사한다.
	memcpy(&(tx.pid_p1), &(m_kp), sizeof(float));
	memcpy(&(tx.pid_i1), &(m_ki), sizeof(float));
	memcpy(&(tx.pid_d1), &(m_kd), sizeof(float));

	// integral max 값을 buffer 에 복사한다.
	memcpy(&(tx.integralMax_1), &(integralMax), sizeof(float));

	BYTE senddata[65] = { 0, };
	BYTE readdata[65] = { 0, };
	memcpy(senddata, &tx, sizeof(TxBuffer));

	device->Write(senddata);

	if (device->Read(&rx) == 0)
		return FALSE;

	memcpy(readdata, &rx, sizeof(RxBuffer));

	// 기기로부터 받은 온도 값을 받아와서 저장함.
	// convert BYTE pointer to float type for reading temperature value.
	memcpy(&currentTemp, &(rx.chamber_temp_1), sizeof(float));

	// 기기로부터 받은 Photodiode 값을 받아와서 저장함.
	photodiode_h = rx.photodiode_h;
	photodiode_l = rx.photodiode_l;

	if (currentCmd == CMD_FAN_OFF) {
		currentCmd = CMD_READY;
	}
	else if (currentCmd == CMD_PCR_STOP) {
		currentCmd = CMD_READY;
	}

	if (currentTemp < 0.0)
		return FALSE;

	// 150918 YJ added, For falling stage routine
	if (targetTempFlag && !freeRunning)
	{
		// target temp 이하가 되는 순간부터 freeRunning 으로 
		if (currentTemp <= m_currentTargetTemp)
		{
			freeRunning = true;
			freeRunningCounter = 0;
		}
	}

	if (freeRunning)
	{
		freeRunningCounter++;
		// 기기와 다르게 3초 후부터 arrival 로 인식하도록 변경
		if (freeRunningCounter >= (3000 / TIMER_DURATION))
		{
			targetTempFlag = false;
			freeRunning = false;
			freeRunningCounter = 0;
			isTargetArrival = true;
		}
	}

	CString tempString;
	tempString.Format(L"%.1f ℃", currentTemp);
	SetDlgItemText(IDC_EDIT_CURRENT_TEMP, tempString);

	if (fabs(currentTemp - m_currentTargetTemp) < m_cArrivalDelta && !targetTempFlag)
		isTargetArrival = true;

	// Check the error from device
	static bool onceShow = true;
	if (rx.currentError == ERROR_ASSERT && onceShow) {
		onceShow = false;
		AfxMessageBox(L"Software error occured!\nPlease contact to developer");
	}
	else if (rx.currentError == ERROR_OVERHEAT && onceShow) {
		onceShow = false;
		emergencyStop = true;
		// PCREndTask();
		cleanupTask();
	}

	// logging
	if (!logStopped) {
		CString values;
		double currentTime = (double)(timeGetTime() - recStartTime);
		recordingCount++;
		values.Format(L"%6d	%8.0f	%3.1f\n", recordingCount, currentTime, currentTemp);
		m_recFile.WriteString(values);

		// for log message per 1 sec
		/*
		if (recordingCount % 20 == 0) {
			int elapsed_time = (int)((double)(timeGetTime() - recStartTime) / 1000.);
			int min = elapsed_time / 60;
			int sec = elapsed_time % 60;
			CString elapseTime, lineTime, totalTime;
			elapseTime.Format(L"%dm %ds", min, sec);

			min = leftSec / 60;
			sec = leftSec % 60;

			// current left protocol time
			if (min == 0)
				lineTime.Format(L"%ds", sec);
			else
				lineTime.Format(L"%dm %ds", min, sec);

			CString tempStr;
			tempStr.Format(L"%3.1f", currentTemp);

			double lights = (double)(photodiode_h & 0x0f)*256. + (double)(photodiode_l);

			CString log;
			log.Format(L"cmd: %d, targetTemp: %3.1f, temp: %s, elapsed time: %s, line Time: %s, protocol Time: %s, device TargetArr: %d, mfc TargetArr: %d, free Running: %d, free Running Counter: %d, ArrivalDelta: %3.1f, tempFlag: %d, photodiode: %3.1f\n",
				currentCmd, m_currentTargetTemp, tempStr, elapseTime, lineTime, totalTime, (int)rx.targetArrival, (int)isTargetArrival, (int)freeRunning, (int)freeRunningCounter, m_cArrivalDelta, (int)targetTempFlag, lights);
			FileManager::log(log);
		}
		*/
	}

	return FALSE;
}

void CMainGraphDialog::findPID()
{
	if (fabs(m_prevTargetTemp - m_currentTargetTemp) < .5)
		return; // if target temp is not change then do nothing 1->.5 correct

	double dist = 10000;
	int paramIdx = 0;

	for (UINT i = 0; i < pids.size(); i++)
	{
		double tmp = fabs(m_prevTargetTemp - pids[i].startTemp) + fabs(m_currentTargetTemp - pids[i].targetTemp);

		if (tmp < dist)
		{
			dist = tmp;
			paramIdx = i;
		}
	}

	m_kp = pids[paramIdx].kp;
	m_ki = pids[paramIdx].ki;
	m_kd = pids[paramIdx].kd;
}

void CMainGraphDialog::initValues() {
	if (isStarted) {
		currentCmd = CMD_PCR_STOP;
	}

	magnetoProtocolIdx = 0;
	m_currentActionNumber = -1;
	m_leftGotoCount = -1;
	leftSec = 0;
	totalLeftSec = 0;
	m_timerCounter = 0;
	m_timeOut = 0;
	isTargetArrival = false;
	filterIndex = 0;
	filterRunning = false;
	shotCounter = 0;

	ledControl_wg = 1;
	ledControl_r = 1;
	ledControl_g = 1;
	ledControl_b = 1;

	m_kp = 0;
	m_ki = 0;
	m_kd = 0;
	emergencyStop = false;
	freeRunning = false;
	freeRunningCounter = 0;
	currentCycle = 0;
	recordingCount = 0;

	m_prevTargetTemp = m_currentTargetTemp = 25;
}

void CMainGraphDialog::timeTask() {
	m_timerCounter++;

	// 1s 마다 실행되도록 설정
	if (m_timerCounter == (1000 / TIMER_DURATION))
	{
		m_timerCounter = 0;

		CString debug;
		debug.Format(L"Current cmd : %d, current action : %d target temp : %.1f, current left sec : %d\n", currentCmd, m_currentActionNumber, m_currentTargetTemp, leftSec);
		::OutputDebugString(debug);

		if (leftSec == 0) {
			m_currentActionNumber++;

			if ((m_currentActionNumber) >= currentProtocol.actionList.size())
			{
				::OutputDebugString(L"complete!\n");
				isCompletePCR = true;
				// PCREndTask();
				cleanupTask();
				return;
			}

			debug.Format(L"label : %s filterIndex : %d\n", currentProtocol.actionList[m_currentActionNumber].Label, filterIndex);
			::OutputDebugString(debug);

			// If the current protocol is SHOT
			if (currentProtocol.actionList[m_currentActionNumber].Label.Compare(L"SHOT") == 0)
			{
				if (filterIndex == 0) {
					// Check the this filter is used.
					if (!currentProtocol.useFam) {
						filterIndex = 1;
					}
				}
				if (filterIndex == 1) {
					// Check the this filter is used.
					if (!currentProtocol.useHex) {
						filterIndex = 2;
					}
				}
				if (filterIndex == 2) {
					// Check the this filter is used.
					if (!currentProtocol.useRox) {
						filterIndex = 3;
					}
				}
				if (filterIndex == 3) {
					// Check the this filter is used.
					if (!currentProtocol.useCY5) {
						filterIndex = 4;
					}
				}

				// 4 is finished
				if (filterIndex == 4) {
					filterIndex = 0;
					currentCycle++;

					CString values;
					double currentTime = (double)(timeGetTime() - recStartTime);
					double sensorValue = 0.0, sensorValue2 = 0.0, sensorValue3 = 0.0, sensorValue4 = 0.0;

					if (currentProtocol.useFam) {
						sensorValue = sensorValuesFam[currentCycle];
					}
					if (currentProtocol.useHex) {
						sensorValue2 = sensorValuesHex[currentCycle];
					}
					if (currentProtocol.useRox) {
						sensorValue3 = sensorValuesRox[currentCycle];
					}
					if (currentProtocol.useCY5) {
						sensorValue4 = sensorValuesCy5[currentCycle];
					}

					values.Format(L"%6d	%8.0f	%3.1f	%3.1f	%3.1f	%3.1f\n", currentCycle, currentTime, sensorValue, sensorValue2, sensorValue3, sensorValue4);
					m_recPDFile.WriteString(values);
				}
				else {
					// Run the filter
					if (filterRunning) {
						// Maybe finished..
						if (magneto->isFilterActionFinished() == false) {
							int* led;
							vector<double>* sensorValues;

							if (filterIndex == 0) {
								led = &ledControl_b;
								sensorValues = &sensorValuesFam;
							}
							else if (filterIndex == 1) {
								led = &ledControl_wg;
								sensorValues = &sensorValuesHex;
							}
							else if (filterIndex == 2) {
								led = &ledControl_g;
								sensorValues = &sensorValuesRox;
							}
							else if (filterIndex == 3) {
								led = &ledControl_r;
								sensorValues = &sensorValuesCy5;
							}

							// Turn on the led
							*led = 0;

							// Shot sequence
							shotCounter++;
							if (shotCounter >= 2) {
								// Getting the photodiode data
								double lights = (double)(photodiode_h & 0x0f) * 256. + (double)(photodiode_l);
								sensorValues->push_back(lights);

								setChartValue();

								debug.Format(L"filter value : %d, %d, %f\n", photodiode_h, photodiode_l, lights);
								::OutputDebugString(debug);

								// Turn off led
								*led = 1;
								shotCounter = 0;

								// Next filter
								filterIndex++;
								filterRunning = false;
							}
						}
					}
					else {
						filterRunning = true;
						magneto->runFilterAction(filterIndex);
					}
					m_currentActionNumber--;	// For checking the shot command 
				}
			}
			// If the current protocol is GOTO
			else if (currentProtocol.actionList[m_currentActionNumber].Label.Compare(L"GOTO") == 0)
			{
				if (m_leftGotoCount < 0)
					m_leftGotoCount = (int)currentProtocol.actionList[m_currentActionNumber].Time;

				if (m_leftGotoCount == 0)
					m_leftGotoCount = -1;
				else
				{
					m_leftGotoCount--;

					// GOTO 문의 target label 값을 넣어줌
					CString tmpStr;
					tmpStr.Format(L"%d", (int)currentProtocol.actionList[m_currentActionNumber].Temp);

					int pos = 0;
					for (pos = 0; pos < currentProtocol.actionList.size(); ++pos)
						if (tmpStr.Compare(currentProtocol.actionList[pos].Label) == 0)
							break;

					m_currentActionNumber = pos - 1;
				}
			}
			// Label
			else {
				m_prevTargetTemp = m_currentTargetTemp;
				m_currentTargetTemp = (int)currentProtocol.actionList[m_currentActionNumber].Temp;

				targetTempFlag = m_prevTargetTemp > m_currentTargetTemp;

				isTargetArrival = false;
				leftSec = (int)(currentProtocol.actionList[m_currentActionNumber].Time);
				m_timeOut = TIMEOUT_CONST * 10;

				// find the proper pid values.
				findPID();
			}
		}
		else { // The action is in progress.
			if (!isTargetArrival)
			{
				m_timeOut--;

				if (m_timeOut == 0)
				{
					AfxMessageBox(L"The target temperature cannot be reached!!");
					// PCREndTask();
					cleanupTask();
				}
			}
			else {
				leftSec--;
				totalLeftSec--;

				CString leftTime;
				int min = totalLeftSec / 60;
				int sec = totalLeftSec % 60;

				if (min == 0)
					leftTime.Format(L"%02ds", sec);
				else
					leftTime.Format(L"%02dm %02ds", min, sec);
				SetDlgItemText(IDC_STATIC_PROGRESS_REMAINING_TIME, leftTime);
			}
		}
	}
}

void CMainGraphDialog::cleanupTask() {
	// Stop timer
	m_Timer->stopTimer();

	// 210120 KBH Magneto AlarmReset
	magneto->Alarmreset();	

	// Start cleanup timer
	// Setting the home command
	CString magnetoProtocolRes = magneto->loadProtocolFromData(L"home");

	if (magneto->isCompileSuccess(magnetoProtocolRes)) {
		// initialize the protocol
		vector<ActionBeans> treeList;
		magneto->generateActionList(treeList);
	}

	magneto->start();
	SetTimer(Magneto::TimerCleanupTaskID, Magneto::TimerCleanupTaskDuration, NULL);
}

void CMainGraphDialog::PCREndTask() {
	while (true)
	{
		RxBuffer rx;
		TxBuffer tx;

		memset(&rx, 0, sizeof(RxBuffer));
		memset(&tx, 0, sizeof(TxBuffer));

		tx.cmd = CMD_PCR_STOP;

		BYTE senddata[65] = { 0, };
		BYTE readdata[65] = { 0, };
		memcpy(senddata, &tx, sizeof(TxBuffer));

		device->Write(senddata);

		device->Read(&rx);

		memcpy(readdata, &rx, sizeof(RxBuffer));

		if (rx.state == STATE_READY) break;

		Sleep(TIMER_DURATION);
	}

	initValues();
	clearLog();

	isStarted = false;

	progressStatus.SetPos(0);
	GetDlgItem(IDC_COMBO_PROTOCOLS)->EnableWindow();
	GetDlgItem(IDC_BUTTON_SETUP)->EnableWindow();
	GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow();
	GetDlgItem(IDC_BUTTON_START)->EnableWindow();
	SetDlgItemText(IDC_BUTTON_START, L"Start");
	SetDlgItemText(IDC_STATIC_PROGRESS_STATUS, L"Idle..");
	SetDlgItemText(IDC_EDIT_CT_FAM, L"");
	SetDlgItemText(IDC_EDIT_CT_HEX, L"");
	SetDlgItemText(IDC_EDIT_CT_ROX, L"");
	SetDlgItemText(IDC_EDIT_CT_CY5, L"");
	SetDlgItemText(IDC_EDIT_CURRENT_TEMP, L"");

	if (!emergencyStop)
	{
		if (isCompletePCR) {
			// Getting the current date time
			CTime cTime = CTime::GetCurrentTime();
			CString dateTime;
			dateTime.Format(L"%04d.%02d.%02d %02d:%02d:%02d", cTime.GetYear(), cTime.GetMonth(), cTime.GetDay(),
				cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());

			// 210910 KBH : sqlite3에 사용될 변수들
			// 공주 보건소 CT&NG ct value and result 
			CString ct_value, ng_value, ct_result, ng_result;
			// Info data 
			CString user_id, user_name, user_age, gender, inspector, sample_type, sample_date;
			// sql query and dummy 
			CString sql_query, dummy;

			// result index
			int resultIndex = 0;

			if (currentProtocol.useFam) {
				setCTValue(dateTime, sensorValuesFam, resultIndex++, 0, ct_value, ct_result);
			}
			if (currentProtocol.useHex) {
				setCTValue(dateTime, sensorValuesHex, resultIndex++, 1, ng_value, ng_result);
			}
			if (currentProtocol.useRox) {
				setCTValue(dateTime, sensorValuesRox, resultIndex++, 2, dummy, dummy);
			}
			if (currentProtocol.useCY5) {
				setCTValue(dateTime, sensorValuesCy5, resultIndex++, 3, dummy, dummy);
			}
			
			// 210913 KBH get result table rect
			//InvalidateRect(&CRect(226, 655, 456, 768));
			CRect table_rect;
			GetDlgItem(IDC_CUSTOM_RESULT_TABLE)->GetWindowRect(&table_rect);
			InvalidateRect(&table_rect);

			GetDlgItemText(IDC_EDIT_USER_ID, user_id);
			GetDlgItemText(IDC_EDIT_USER_NAME, user_name);
			GetDlgItemText(IDC_EDIT_USER_AGE, user_age);
			GetDlgItemText(IDC_EDIT_INSPECTOR, inspector);
			GetDlgItemText(IDC_EDIT_SAMPLE_TYPE, sample_type);
			((CDateTimeCtrl*)GetDlgItem(IDC_DATE_SAMPLE))->GetTime(cTime);
			sample_date = cTime.Format("%Y-%m-%d");
			gender = ((CButton*)GetDlgItem(IDC_RADIO_GENDER_M))->GetCheck() ? L"M" : L"F";

			sql_query.Format(L"\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"", user_id, user_name, user_age, gender, inspector, sample_type, sample_date, ct_value, ng_value, ct_result, ng_result);
			insertFieldValue(sql_query);
			AfxMessageBox(L"PCR ended!!");
		}
		else AfxMessageBox(L"PCR incomplete!!");
	}
	else
	{
		AfxMessageBox(L"Emergency stop!(overheating)");
	}

	// 210910 KBH  activate Controls what inside group Info
	GetDlgItem(IDC_EDIT_USER_ID)->SendMessage(EM_SETREADONLY, false, 0);

	GetDlgItem(IDC_EDIT_USER_NAME)->SendMessage(EM_SETREADONLY, false, 0);
	GetDlgItem(IDC_EDIT_USER_AGE)->SendMessage(EM_SETREADONLY, false, 0);
	GetDlgItem(IDC_EDIT_INSPECTOR)->SendMessage(EM_SETREADONLY, false, 0);
	GetDlgItem(IDC_EDIT_SAMPLE_TYPE)->SendMessage(EM_SETREADONLY, false, 0);
	GetDlgItem(IDC_DATE_SAMPLE)->EnableWindow();
	GetDlgItem(IDC_RADIO_GENDER_M)->EnableWindow();
	GetDlgItem(IDC_RADIO_GENDER_F)->EnableWindow();


	emergencyStop = false;
	isCompletePCR = false;

	// reset the remaining time
	calcTotalTime();
}

void CMainGraphDialog::setChartValue() {
	if (isStarted) {
		m_Chart.DeleteAllData();

		double* dataFam = new double[sensorValuesFam.size() * 2];
		double* dataHex = new double[sensorValuesHex.size() * 2];
		double* dataRox = new double[sensorValuesRox.size() * 2];
		double* dataCy5 = new double[sensorValuesCy5.size() * 2];

		vector<double> copySensorValuesFam, copySensorValuesHex, copySensorValuesRox, copySensorValuesCy5;
		double meanFam = 0.0, meanHex = 0.0, meanRox = 0.0, meanCy5 = 0.0;

		int maxY = 0;

		if (useFam)
		{
			// Calculate the mean value first
			if (sensorValuesFam.size() >= 11) {
				double sum = std::accumulate(sensorValuesFam.begin()+1, sensorValuesFam.begin() + 11, 0.0);
				meanFam = sum / 10;
			}
			else {
				double sum = std::accumulate(sensorValuesFam.begin(), sensorValuesFam.end(), 0.0);
				meanFam = sum / sensorValuesFam.size();
			}

			// Copy data
			for (int i = 0; i < sensorValuesFam.size(); ++i) {
				if (i >= 1) {
					copySensorValuesFam.push_back(sensorValuesFam[i] - meanFam);
				}
				else {
					copySensorValuesFam.push_back(sensorValuesFam[i]);
				}
			}

			int	nDims_fam = 2, dims_fam[2] = { 2, copySensorValuesFam.size() };
			for (int i = 0; i < copySensorValuesFam.size(); ++i)
			{
				dataFam[i] = i;
				dataFam[i + copySensorValuesFam.size()] = copySensorValuesFam[i];
			}
			m_Chart.SetDataColor(m_Chart.AddData(dataFam, nDims_fam, dims_fam), RGB(0, 0, 255));

			int tempMax = *max_element(copySensorValuesFam.begin(), copySensorValuesFam.end());

			if (maxY < tempMax) {
				maxY = tempMax;
			}
		}

		if (useHex)
		{
			// Calculate the mean value first
			if (sensorValuesHex.size() >= 11) {
				double sum = std::accumulate(sensorValuesHex.begin()+1, sensorValuesHex.begin() + 11, 0.0);
				meanHex = sum / 10;
			}
			else {
				double sum = std::accumulate(sensorValuesHex.begin(), sensorValuesHex.end(), 0.0);
				meanHex = sum / sensorValuesHex.size();
			}

			// Copy data
			for (int i = 0; i < sensorValuesHex.size(); ++i) {
				if (i >= 1) {
					copySensorValuesHex.push_back(sensorValuesHex[i] - meanHex);
				}
				else {
					copySensorValuesHex.push_back(sensorValuesHex[i]);
				}
			}

			int	nDims_hex = 2, dims_hex[2] = { 2, copySensorValuesHex.size() };
			for (int i = 0; i < copySensorValuesHex.size(); ++i)
			{
				dataHex[i] = i;
				dataHex[i + copySensorValuesHex.size()] = copySensorValuesHex[i];
			}
			m_Chart.SetDataColor(m_Chart.AddData(dataHex, nDims_hex, dims_hex), RGB(0, 255, 0));

			int tempMax = *max_element(copySensorValuesHex.begin(), copySensorValuesHex.end());

			if (maxY < tempMax) {
				maxY = tempMax;
			}
		}

		if (useRox)
		{
			// Calculate the mean value first
			if (sensorValuesRox.size() >= 11) {
				double sum = std::accumulate(sensorValuesRox.begin()+1, sensorValuesRox.begin() + 11, 0.0);
				meanRox = sum / 10;
			}
			else {
				double sum = std::accumulate(sensorValuesRox.begin(), sensorValuesRox.end(), 0.0);
				meanRox = sum / sensorValuesRox.size();
			}

			// Copy data
			for (int i = 0; i < sensorValuesRox.size(); ++i) {
				if (i >= 1) {
					copySensorValuesRox.push_back(sensorValuesRox[i] - meanRox);
				}
				else {
					copySensorValuesRox.push_back(sensorValuesRox[i]);
				}
			}

			int	nDims_rox = 2, dims_rox[2] = { 2, copySensorValuesRox.size() };
			for (int i = 0; i < copySensorValuesRox.size(); ++i)
			{
				dataRox[i] = i;
				dataRox[i + copySensorValuesRox.size()] = copySensorValuesRox[i];
			}
			m_Chart.SetDataColor(m_Chart.AddData(dataRox, nDims_rox, dims_rox), RGB(0, 128, 0));

			int tempMax = *max_element(copySensorValuesRox.begin(), copySensorValuesRox.end());

			if (maxY < tempMax) {
				maxY = tempMax;
			}
		}

		if (useCy5)
		{
			// Calculate the mean value first
			if (sensorValuesCy5.size() >= 11) {
				double sum = std::accumulate(sensorValuesCy5.begin()+1, sensorValuesCy5.begin() + 11, 0.0);
				meanCy5 = sum / 10;
			}
			else {
				double sum = std::accumulate(sensorValuesCy5.begin(), sensorValuesCy5.end(), 0.0);
				meanCy5 = sum / sensorValuesCy5.size();
			}

			// Copy data
			for (int i = 0; i < sensorValuesCy5.size(); ++i) {
				if (i >= 1) {
					copySensorValuesCy5.push_back(sensorValuesCy5[i] - meanCy5);
				}
				else {
					copySensorValuesCy5.push_back(sensorValuesCy5[i]);
				}
			}

			int	nDims_cy5 = 2, dims_cy5[2] = { 2, copySensorValuesCy5.size() };
			for (int i = 0; i < copySensorValuesCy5.size(); ++i)
			{
				dataCy5[i] = i;
				dataCy5[i + copySensorValuesCy5.size()] = copySensorValuesCy5[i];
			}
			m_Chart.SetDataColor(m_Chart.AddData(dataCy5, nDims_cy5, dims_cy5), RGB(255, 0, 0));

			int tempMax = *max_element(copySensorValuesCy5.begin(), copySensorValuesCy5.end());

			if (maxY < tempMax) {
				maxY = tempMax;
			}
		}

		maxY += displayDelta;

		// Not use the range when the value is changed.
		CAxis* axis = m_Chart.GetAxisByLocation(kLocationLeft);
		// Not used maxY now
		//axis->SetRange(-displayDelta, maxY);	// 210203 KBH Fixed Y range 

		//InvalidateRect(&CRect(15, 130, 470, 500)); // 210910 KBH using graphRect 
		InvalidateRect(&m_graphRect, FALSE);

		Invalidate(FALSE);
	}
}

void CMainGraphDialog::clearChartValue() {
	sensorValuesFam.clear();
	sensorValuesHex.clear();
	sensorValuesRox.clear();
	sensorValuesCy5.clear();

	sensorValuesFam.push_back(1.0);
	sensorValuesHex.push_back(1.0);
	sensorValuesRox.push_back(1.0);
	sensorValuesCy5.push_back(1.0);

	m_Chart.DeleteAllData();

	CAxis* axis = m_Chart.GetAxisByLocation(kLocationLeft);
	// 210910 KBH remove update y axis ticks 
	//axis->SetTickCount(8); // 210203 KBH tick count : 8
	//axis->SetRange(-512, 4096);  // 210118 KBH Y-range lower : 0 -> -512

	//InvalidateRect(&CRect(15, 130, 470, 500)); // 210910 KBH using graphRect 
	InvalidateRect(&m_graphRect, FALSE);

}

static CString filterTable[4] = { L"FAM", L"HEX", L"ROX", L"CY5" };

// 210910 KBH append parameters (Ct value and result text) 
void CMainGraphDialog::setCTValue(CString dateTime, vector<double>& sensorValue, int resultIndex, int filterIndex, CString& val, CString& rst) {
	// save the result and setting the result
	CString result = L"FAIL";
	CString ctText = L"";
	CString filterLabel[4] = { currentProtocol.labelFam, currentProtocol.labelHex, currentProtocol.labelRox, currentProtocol.labelCY5 };

	// ignore the data when the data is over the 10
	int idx = sensorValue.size();

	// If the idx is under the 10, fail
	if (idx >= 11) {
		// BaseMean value
		float baseMean = 0.0;

		// 230202 KBH Change baseMean calculation range (1 ~ 11 -> 4 ~ 16)
		for (int i = 4; i < 16; ++i) {
			baseMean += sensorValue[i];
		}
		baseMean /= 12.;

		float threshold = 0.697 * flRelativeMax / 10.;
		float logThreshold = log(threshold);
		float ct;

		// Getting the log threshold from file
		float tempLogThreshold = FileManager::getFilterValue(filterIndex);

		// Success to load
		if (tempLogThreshold > 0.0) {
			logThreshold = tempLogThreshold;
		}

		for (int i = 0; i < sensorValue.size(); ++i) {
			if (log(sensorValue[i] - baseMean) > logThreshold) {
				idx = i;
				break;
			}
		}

		if (idx >= sensorValue.size() || idx <= 0) {
			// 210910 KBH : Change "Not detected" to "Negative"
			//result = L"Not detected"; 
			result = L"Negative";
		}
		else {
			double resultRange[4] = { currentProtocol.ctFam, currentProtocol.ctHex, currentProtocol.ctRox, currentProtocol.ctCY5 };

			float cpos = idx + 1;
			float cval = log(sensorValue[idx] - baseMean);
			float delta = cval - log(sensorValue[idx - 1] - baseMean);
			ct = cpos - (cval - logThreshold) / delta;
			ctText.Format(L"%.2f", ct);

			if (ct >= 16 && ct <= 40)
			{
				result = resultRange[filterIndex] <= ct ? L"Negative" : L"Positive";
			}
			else // Error
			{
				result = L"Error";
			}

			// Setting the CT text
			int ID_LIST[4] = { IDC_EDIT_CT_FAM, IDC_EDIT_CT_HEX, IDC_EDIT_CT_ROX, IDC_EDIT_CT_CY5 };
			SetDlgItemText(ID_LIST[filterIndex], ctText);
		}
	}

	vector<History> historyList;
	FileManager::loadHistory(historyList);

	History history(dateTime, filterLabel[filterIndex], filterTable[filterIndex], ctText, result);
	historyList.push_back(history);
	FileManager::saveHistory(historyList);

	GV_ITEM item;
	item.mask = GVIF_TEXT | GVIF_FORMAT;
	item.row = resultIndex + 1;
	item.col = 1;
	item.nFormat = DT_CENTER | DT_WORDBREAK;

	item.strText = result;

	// resultTable.SetItemFont(item.row, item.col, )
	resultTable.SetItem(&item);

	// 210910 KBH : return ctText and result (database)
	val = ctText;
	rst = result;
}

// 200804 KBH change log file name 
void CMainGraphDialog::initLog() {
	long serialNumber = magneto->getSerialNumber();
	CreateDirectory(L"./Record/", NULL);

	CString fileName, fileName2;
	CTime time = CTime::GetCurrentTime();
	CString currentTime = time.Format(L"%Y%m%d-%H%M-%S");

	// change file name
	//fileName = time.Format(L"./Record/%Y%m%d-%H%M-%S.txt");
	//fileName2 = time.Format(L"./Record/pd%Y%m%d-%H%M-%S.txt");

	fileName.Format(L"./Record/log%06ld-%s.txt", serialNumber, currentTime);
	fileName2.Format(L"./Record/log%06ld-pd%s.txt", serialNumber, currentTime);
	

	m_recFile.Open(fileName, CStdioFile::modeCreate | CStdioFile::modeWrite);
	m_recFile.WriteString(L"Number	Time	Temperature\n");

	m_recPDFile.Open(fileName2, CStdioFile::modeCreate | CStdioFile::modeWrite);
	m_recPDFile.WriteString(L"Cycle	Time	FAM	HEX	ROX	CY5\n");

	recStartTime = timeGetTime();
}

void CMainGraphDialog::clearLog() {
	logStopped = true;

	if (m_recFile != NULL) {
		m_recFile.Close();
	}
	if (m_recPDFile != NULL) {
		m_recPDFile.Close();
	}
}

void CMainGraphDialog::OnBnClickedButtonFilterFam()
{
	useFam = !useFam;
	if (useFam) {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_FAM, famImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_FAM, offImg);
	}

	setChartValue();
	//InvalidateRect(&CRect(15, 130, 470, 500)); // 210910 KBH using graphRect 
	InvalidateRect(&m_graphRect, FALSE);

}


void CMainGraphDialog::OnBnClickedButtonFilterHex()
{
	useHex = !useHex;
	if (useHex) {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_HEX, hexImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_HEX, offImg);
	}

	setChartValue();
	//InvalidateRect(&CRect(15, 130, 470, 500)); // 210910 KBH using graphRect 
	InvalidateRect(&m_graphRect, FALSE);

}


void CMainGraphDialog::OnBnClickedButtonFilterRox()
{
	useRox = !useRox;
	if (useRox) {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_ROX, roxImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_ROX, offImg);
	}

	setChartValue();
	//InvalidateRect(&CRect(15, 130, 470, 500)); // 210910 KBH using graphRect 
	InvalidateRect(&m_graphRect, FALSE);

}


void CMainGraphDialog::OnBnClickedButtonFilterCy5()
{
	useCy5 = !useCy5;
	if (useCy5) {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_CY5, cy5Img);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_FILTER_CY5, offImg);
	}

	setChartValue();
	//InvalidateRect(&CRect(15, 130, 470, 500)); // 210910 KBH using graphRect 
	InvalidateRect(&m_graphRect, FALSE);

}


// 210910 KBH : encoding function to utf8 (sqlite3 is using only utf8 format)
static char* utf8_encode(const char* src)
{
	size_t len;
	char* result;
	wchar_t* tempBuf;
	if (src == NULL)  return NULL;

	len = strlen(src);
	result = (char*)malloc(len * 3 + 1);

	if (result == NULL)  return NULL;

	tempBuf = (wchar_t*)alloca((len + 1) * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, src, -1, tempBuf, (int)len);

	tempBuf[len] = 0;
	{
		wchar_t* s = tempBuf;  BYTE* d = (BYTE*)result;
		while (*s)
		{
			int U = *s++;
			if (U < 0x80) {
				*d++ = (BYTE)U;
			}
			else if (U < 0x800) {
				*d++ = 0xC0 + ((U >> 6) & 0x3F);
				*d++ = 0x80 + (U & 0x003F);
			}
			else {
				*d++ = 0xE0 + (U >> 12);
				*d++ = 0x80 + ((U >> 6) & 0x3F);
				*d++ = 0x80 + (U & 0x3F);
			}
		}
		*d = 0;
	}
	return result;
}

// 210910 KBH : create History table if not exists Hitory table 
void CMainGraphDialog::initDatabaseTable()
{
	USES_CONVERSION;
	sqlite3_stmt* stmt;
	const char* sql_query;

	sql_query = "CREATE TABLE IF NOT EXISTS History("
		"id integer not null primary key autoincrement,"
		"_datetime datetime default (datetime('now', 'localtime')),"
		"user_id text not null, user_name text not null, user_age int not null, gender text not null,"
		"inspector text not null, sample_type text not null, sample_date date default (date('now', 'localtime')),"
		"CTv text not null, NGv text not null, CT text not null, NG text not null);";
	// check if database file exists
	int rst = sqlite3_open("./testRecord.db", &database);

	if (rst != SQLITE_OK)
	{
		CString SQLERR;
		SQLERR.Format(L"Can't open database: %s", sqlite3_errmsg(database));
		AfxMessageBox(SQLERR);
		sqlite3_free(szErrMsg);
		sqlite3_close(database);
		database = NULL;

	}
	else
	{
		rst = sqlite3_exec(database, sql_query, NULL, NULL, &szErrMsg);
		if (rst != SQLITE_OK)
		{
			CString SQLERR;
			SQLERR.Format(L"Can't create table: %s", sqlite3_errmsg(database));
			AfxMessageBox(SQLERR);
			sqlite3_free(szErrMsg);
			sqlite3_close(database);
			database = NULL;
		}
	}
}

// 210910 KBH : Insert Value in Database
void CMainGraphDialog::insertFieldValue(CString values)
{
	int rst = sqlite3_open("./testRecord.db", &database);

	if (rst != SQLITE_OK)
	{
		CString SQLERR;
		SQLERR.Format(L"Can't open database: %s", sqlite3_errmsg(database));
		AfxMessageBox(SQLERR);
		sqlite3_free(szErrMsg);
		sqlite3_close(database);
		database = NULL;

	}
	else
	{
		CString sql_temp;
		sql_temp.Format(_T("INSERT INTO History (user_id, user_name, user_age, gender, inspector, sample_type, sample_date, CTv, NGv, CT, NG) values ( %s )"), values);

		USES_CONVERSION;
		const char* sql = T2A(sql_temp);

		sqlite3_exec(database, utf8_encode(sql), NULL, NULL, &szErrMsg);

		sqlite3_close(database);

	}

}