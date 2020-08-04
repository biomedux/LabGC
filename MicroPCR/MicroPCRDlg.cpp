
// MicroPCRDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "MicroPCR.h"
#include "MicroPCRDlg.h"
#include "ConvertTool.h"
#include "FileManager.h"
#include "PIDManagerDlg.h"

#include <locale.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

HWND hCommWnd;

// CMicroPCRDlg 대화 상자

CMicroPCRDlg::CMicroPCRDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMicroPCRDlg::IDD, pParent)
	, m_cMaxActions(1000)
	, m_cTimeOut(1000)
	, m_cArrivalDelta(0.5)
	, m_sProtocolName(L"")
	, m_currentActionNumber(-1)
	, m_blinkCounter(0)
	, m_timerCounter(0)
	, blinkFlag(false)
	, isStarted(false)
	, isCompletePCR(false)
	, isTargetArrival(false)
	, isFirstDraw(false)
	, m_startTime2(0)
	, m_prevTargetTemp(25)
	, m_currentTargetTemp(25)
	, m_timeOut(0)
	, m_leftGotoCount(-1)
	, m_cGraphYMin(0)
	, m_cGraphYMax(4096)
	, ledControl_wg(1)
	, ledControl_r(1)
	, ledControl_g(1)
	, ledControl_b(1)
	, currentCmd(CMD_READY)
	, m_kp(0.0)
	, m_ki(0.0)
	, m_kd(0.0)
	, isFanOn(false)
	, isLedOn(false)
	, m_cIntegralMax(INTGRALMAX)
	, loadedPID(L"")
	, targetTempFlag(false)
	, freeRunning(false)
	, freeRunningCounter(0)
	, m_cCompensation(0)
	, emergencyStop(false)
	, protocol_size(0)
	, isStarted2(false)
	, isTempGraphOn(false)
	, isRecording(false)
	, m_recordingCount(0)
	, m_recStartTime(0)
	, m_cycleCount2(0)
	, pSQLite3(NULL)
	, szErrMsg(NULL)
	, mag_connected(false)
	, mag_started(false)
	, file_created(false)
	, filter_working(false)
	, filter_workFinished(false)
	, filter_index(0)
	, shotTaskTimer(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMicroPCRDlg::~CMicroPCRDlg()
{
	// 생성한 객체를 소멸자에서 제거해준다.
	if( device != NULL )
		delete device;
	if( m_Timer != NULL )
		delete m_Timer;
	if (pSQLite3 != NULL)
		sqlite3_close(pSQLite3);
	if (szErrMsg != NULL)
		sqlite3_free(szErrMsg);
}

void CMicroPCRDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CUSTOM_PID_TABLE, m_cPidTable);
	DDX_Text(pDX, IDC_EDIT_MAX_ACTIONS, m_cMaxActions);
	DDV_MinMaxInt(pDX, m_cMaxActions, 1, 10000);
	DDX_Text(pDX, IDC_EDIT_TIME_OUT, m_cTimeOut);
	DDV_MinMaxInt(pDX, m_cTimeOut, 1, 10000);
	DDX_Text(pDX, IDC_EDIT_ARRIVAL_DELTA, m_cArrivalDelta);
	DDV_MinMaxFloat(pDX, m_cArrivalDelta, 0, 10.0);
	DDX_Text(pDX, IDC_EDIT_GRAPH_Y_MIN, m_cGraphYMin);
	DDV_MinMaxInt(pDX, m_cGraphYMin, 0, 4096);
	DDX_Text(pDX, IDC_EDIT_Y_MAX, m_cGraphYMax);
	DDV_MinMaxInt(pDX, m_cGraphYMax, 0, 4096);
	DDX_Text(pDX, IDC_EDIT_INTEGRAL_MAX, m_cIntegralMax);
	DDV_MinMaxFloat(pDX, m_cIntegralMax, 0.0, 10000.0);
	DDX_Text(pDX, IDC_EDIT_COMPENSATION, m_cCompensation);
	DDV_MinMaxByte(pDX, m_cCompensation, 0, 200);
	DDX_Control(pDX, IDC_PROGRESS_BAR, progressBar);
	DDX_Control(pDX, IDC_TREE_PROTOCOL, actionTreeCtrl);
	DDX_Control(pDX, IDC_TAB, m_Tab);
}

BEGIN_MESSAGE_MAP(CMicroPCRDlg, CDialog)
	ON_MESSAGE(WM_MOTOR_POS_CHANGED, OnMotorPosChanged)
	ON_MESSAGE(WM_MMTIMER, OnmmTimer)
	ON_WM_QUERYDRAGICON()
	ON_WM_DEVICECHANGE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB, &CMicroPCRDlg::OnTcnSelchangeTab)
	ON_BN_CLICKED(IDC_BUTTON_ENTER_PID_MANAGER, &CMicroPCRDlg::OnBnClickedButtonEnterPidManager)
	ON_BN_CLICKED(IDC_BUTTON_CONSTANTS_APPLY, &CMicroPCRDlg::OnBnClickedButtonConstantsApply)
	ON_BN_CLICKED(IDC_CHECK_TEMPERATURE, &CMicroPCRDlg::OnBnClickedCheckTemperature)
	ON_WM_TIMER()
	ON_WM_MOVING()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_RESULT, &CMicroPCRDlg::OnBnClickedButtonResult)
	ON_BN_CLICKED(IDC_BUTTON_TEST, &CMicroPCRDlg::OnBnClickedButtonTest)
END_MESSAGE_MAP()


// CMicroPCRDlg 메시지 처리기

BOOL CMicroPCRDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	pDlg = this;
	
	// siri 15-12-03
	SetDlgItemText(IDC_EDIT_Y_TARGET, L"0.000");
	SetDlgItemText(IDC_EDIT_Y_CURRENT, L"0.000");
	SetDlgItemText(IDC_EDIT_MAGNET_TARGET, L"0.000");
	SetDlgItemText(IDC_EDIT_MAGNET_CURRENT, L"0.000");
	SetDlgItemText(IDC_EDIT_LOAD_TARGET, L"0.000");
	SetDlgItemText(IDC_EDIT_LOAD_CURRENT, L"0.000");

	// Tab control init
	UpdateData(TRUE);

	m_Tab.InsertItem(0, L"Home");
	m_Tab.InsertItem(1, L"Result");

	CRect rect;
	m_Tab.GetClientRect(&rect);

	m_form_main.Create(IDD_FORM_MAIN, &m_Tab);
	m_form_main.SetWindowPos(NULL, 5, 25, rect.Width() - 10, rect.Height() - 30, SWP_SHOWWINDOW | SWP_NOZORDER);

	m_form_result.Create(IDD_FORM_RESULT, &m_Tab);
	m_form_result.SetWindowPos(NULL, 5, 25, rect.Width() - 10, rect.Height() - 30, SWP_NOZORDER);

	m_pwndShow = &m_form_main;

	UpdateData(FALSE);

	

	// 150725 PID Check
	// 이전에 불러온 PID 값이 있는지 확인한다.
	CString recentPath;
	vector< CString > pidList;
	FileManager::loadRecentPath(FileManager::PID_PATH, recentPath);
	FileManager::enumFiles( L"./PID/", pidList );

	do
	{
		if( recentPath.IsEmpty() )
		{
			if( pidList.empty() )
				AfxMessageBox(L"저장된 PID Parameter 가 존재하지 않습니다.\n생성하는 창으로 이동됩니다.");
			else
				AfxMessageBox(L"최근 사용한 PID Parameter 가 존재하지 않습니다.\n선택하는 창으로 이동됩니다.");
	
			OnBnClickedButtonEnterPidManager();
			break;
		}
		else
		{
			// recentPath 를 불러오면서 문제가 생긴 경우, recentPath 값을 
			// 지움으로써 위의 if 문이 동작하도록 한다.
			if( !FileManager::loadPID( recentPath, pids ) )
			{
				DeleteFile(RECENT_PID_PATH);
				recentPath.Empty();
			}
			else
			{
				loadedPID = recentPath;
				SetDlgItemText(IDC_EDIT_LOADED_PID, loadedPID);
				break;
			}
		}
	} while(true);

	initPidTable();
	loadPidTable();
	loadConstants();

	// Chart Settings
	CAxis *axis;
	axis = (&m_form_result)->m_Chart.AddAxis(kLocationBottom);
	axis->SetTitle(L"PCR Cycles");
	axis->SetRange(0, 40);

	axis = (&m_form_result)->m_Chart.AddAxis(kLocationLeft);
	axis->SetTitle(L"Sensor Value");
	axis->SetRange(m_cGraphYMin, m_cGraphYMax);
	
	sensorValues_fam.push_back(1.0);
	sensorValues_hex.push_back(1.0);
	sensorValues_rox.push_back(1.0);
	sensorValues_cy5.push_back(1.0);

	device = new CDeviceConnect( GetSafeHwnd() );
	m_Timer = new CMMTimers(1, GetSafeHwnd());

	// 연결 시도
	BOOL status = device->CheckDevice();

	if( status )
	{
		(&m_form_main)->SetDlgItemText(IDC_EDIT_PCR_STATUS, L"Connected");
		(&m_form_main)->isConnected = true;

		CString recentPath;
		FileManager::loadRecentPath(FileManager::PROTOCOL_PATH, recentPath);
		if( !recentPath.IsEmpty() )
			(&m_form_main)->readProtocol(recentPath);
		else
			AfxMessageBox(L"No Recent Protocol File! Please Read Protocol!");
	}
	else
	{
		(&m_form_main)->SetDlgItemText(IDC_EDIT_PCR_STATUS, L"Disconnected");
		(&m_form_main)->isConnected = false;
	}

	(&m_form_main)->enableWindows();

	// 151203 siri for magneto
	magneto = new CMagneto();

	// Load magneto protocol
	// [미해결] MagnetoProtocol.txt 없으면 에러뜸
	CString res = magneto->loadProtocol(L"MagnetoProtocol.txt");

	if (magneto->isCompileSuccess(res))	// 성공한 경우, file 명으로 protocol 이름을 설정한다.
	{
		protocol_size = magneto->getProtocolLength();
		progressBar.SetRange(0, protocol_size);

		// 성공한 경우, Action Protocol List 를 띄워 주기 위한 actionList 를 생성하고
		// Tree 에 표시할 String List 를 불러온다.
		vector<ActionBeans> treeList;
		magneto->generateActionList(treeList);

		actionTreeCtrl.DeleteAllItems();
		
		for (UINT i = 0; i < treeList.size(); ++i){
			HTREEITEM root = actionTreeCtrl.InsertItem(treeList[i].parentAction, TVI_ROOT, TVI_LAST);

			for (UINT j = 0; j < treeList[i].childAction.size(); ++j)
				actionTreeCtrl.InsertItem(treeList[i].childAction[j], root, TVI_LAST);
			actionTreeCtrl.Expand(root, TVE_EXPAND);
		}

		// 맨 위에 선택하도록 
		HTREEITEM item = actionTreeCtrl.GetRootItem();
		actionTreeCtrl.SelectItem(item);

		connectMagneto();
	}
	else
	{
		AfxMessageBox(res);
	}
	
	hCommWnd = this -> m_hWnd;

	CString temp;
	temp.Format(L"%s", theApp.m_lpCmdLine);
	if (temp.Compare(L"admin") == 0)
	{
		SetWindowPos(NULL, 100, 100, 1165, 571, SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(NULL, 100, 100, 585, 385, SWP_NOZORDER);
	}

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CMicroPCRDlg::OnPaint()
{
	CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트

	if (IsIconic())
	{
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CMicroPCRDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CMicroPCRDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{
	// 연결 시도
	BOOL status = device->CheckDevice();
	/**/
	if( status )
	{
		// 중복 connection 막기
		if ((&m_form_main)->isConnected)
			return TRUE;

		SetDlgItemText(IDC_EDIT_PCR_STATUS, L"Connected");

		(&m_form_main)->isConnected = true;

		CString recentPath;
		FileManager::loadRecentPath(FileManager::PROTOCOL_PATH, recentPath);

		if( !recentPath.IsEmpty() )
			(&m_form_main)->readProtocol(recentPath);
		else
			AfxMessageBox(L"No Recent Protocol File! Please Read Protocol!");
		if (isStarted2)
			m_Timer->startTimer(TIMER_DURATION, FALSE);
	}
	else
	{
		SetDlgItemText(IDC_EDIT_PCR_STATUS, L"Disconnected");
		(&m_form_main)->isConnected = false;

		m_Timer->stopTimer();
	}

	(&m_form_main)->enableWindows();

	return TRUE;
}

BOOL CMicroPCRDlg::PreTranslateMessage(MSG* pMsg)
{
	if( pMsg -> message == WM_KEYDOWN )
	{
		if( pMsg -> wParam == VK_RETURN )
			return TRUE;

		if( pMsg -> wParam == VK_ESCAPE )
			return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CMicroPCRDlg::Serialize(CArchive& ar)
{
	// Constants 값을 저장할 때 사용함.
	if (ar.IsStoring())
	{
		ar << m_cMaxActions << m_cTimeOut << m_cArrivalDelta << m_cGraphYMin << m_cGraphYMax << m_cIntegralMax << m_cCompensation;
	}
	else	// Constants 값을 파일로부터 불러올 때 사용한다.
	{
		ar >> m_cMaxActions >> m_cTimeOut >> m_cArrivalDelta >> m_cGraphYMin >> m_cGraphYMax >> m_cIntegralMax >> m_cCompensation;

		UpdateData(FALSE);
	}
}

static const int PID_TABLE_COLUMN_WIDTHS[6] = { 88, 120, 120, 75, 75, 75 };

// pid table 을 grid control 을 그리기 위해 설정.
void CMicroPCRDlg::initPidTable()
{
	// grid control 을 여러 값들을 설정한다.
	m_cPidTable.SetListMode(true);

	m_cPidTable.DeleteAllItems();

	m_cPidTable.SetSingleRowSelection();
	m_cPidTable.SetSingleColSelection();
	m_cPidTable.SetRowCount(1);
	m_cPidTable.SetColumnCount(PID_CONSTANTS_MAX+1);
    m_cPidTable.SetFixedRowCount(1);
    m_cPidTable.SetFixedColumnCount(1);
	m_cPidTable.SetEditable(true);

	// 초기 gridControl 의 table 값들을 설정해준다.
	DWORD dwTextStyle = DT_RIGHT|DT_VCENTER|DT_SINGLELINE;

    for (int col = 0; col < m_cPidTable.GetColumnCount(); col++) { 
		GV_ITEM Item;
        Item.mask = GVIF_TEXT|GVIF_FORMAT;
        Item.row = 0;
        Item.col = col;

        if (col > 0) {
                Item.nFormat = DT_LEFT|DT_WORDBREAK;
                Item.strText = PID_TABLE_COLUMNS[col-1];
        }

        m_cPidTable.SetItem(&Item);
		m_cPidTable.SetColumnWidth(col, PID_TABLE_COLUMN_WIDTHS[col]);
    }
}

// 파일로부터 pid 값을 불러와서 table 에 그려준다.
void CMicroPCRDlg::loadPidTable()
{
	m_cPidTable.SetRowCount(pids.size()+1);

	for(UINT i=0; i<pids.size(); ++i){
		float *temp[5] = { &(pids[i].startTemp), &(pids[i].targetTemp), 
			&(pids[i].kp), &(pids[i].kd), &(pids[i].ki)};
		for(int j=0; j<PID_CONSTANTS_MAX+1; ++j){
			GV_ITEM item;
			item.mask = GVIF_TEXT|GVIF_FORMAT;
			item.row = i+1;
			item.col = j;
			item.nFormat = DT_LEFT|DT_WORDBREAK;

			// 첫번째 column 은 PID 1 으로 표시
			if( j == 0 )
				item.strText.Format(L"PID #%d", i+1);
			else
				item.strText.Format(L"%.4f", *temp[j-1]);
			
			m_cPidTable.SetItem(&item);
		}
	}
}

void CMicroPCRDlg::loadConstants()
{
	CFile file;

	if( file.Open(CONSTANTS_PATH, CFile::modeRead) ){
		CArchive ar(&file, CArchive::load);
		Serialize(ar);
		ar.Close();
		file.Close();
	}
	else{
		AfxMessageBox(L"Constants 파일이 없어 값이 초기화되었습니다.\n다시 값을 설정해주세요.");
		saveConstants();
	}
}

void CMicroPCRDlg::saveConstants()
{
	CFile file;

	file.Open(CONSTANTS_PATH, CFile::modeCreate|CFile::modeWrite);
	CArchive ar(&file, CArchive::store);
	Serialize(ar);
	ar.Close();
	file.Close();
}

void CMicroPCRDlg::OnBnClickedButtonConstantsApply()
{
	if( !UpdateData() ) return;

	if( !pids.empty() )
	{
		pids.clear();
	
		for (int row = 1; row < m_cPidTable.GetRowCount(); row++)
		{
			CString startTemp = m_cPidTable.GetItemText(row, 1);
			CString targetTemp = m_cPidTable.GetItemText(row, 2);
			CString kp = m_cPidTable.GetItemText(row, 3);
			CString kd = m_cPidTable.GetItemText(row, 4);
			CString ki = m_cPidTable.GetItemText(row, 5);
	
			pids.push_back( PID( _wtof(startTemp), _wtof(targetTemp), _wtof(kp), _wtof(ki), _wtof(kd) ) );
		}

		// 변경된 값에 따라 저장해준다.
		FileManager::savePID( loadedPID, pids );
	}

	// PID 값을 제외한 나머지 Constants 들을 저장한다.
	saveConstants();

	CAxis *axis = (&m_form_result)->m_Chart.GetAxisByLocation(kLocationLeft);
	axis->SetRange(m_cGraphYMin, m_cGraphYMax);

	// 동작 중일 경우, 새로 변경된 값으로 pid 를 설정한다.
	if( isStarted ) findPID();
}

void CMicroPCRDlg::OnBnClickedButtonPcrStart()
{
	if ((&m_form_main)->m_totalActionNumber < 1)
	{
		AfxMessageBox(L"The OnBnClickedButtonPcrStart is no action list. Please load task file!!");
		return;
	}
	
	if( !isStarted )
	{
		progressBar.SetPos(0);
		progressBar.SetRange(0, (&m_form_main)->pcr_progressbar_max);

		initValues();

		isStarted = true;

		currentCmd = CMD_PCR_RUN;

		m_prevTargetTemp = 25;

		m_currentTargetTemp = (BYTE)actions[0].Temp;
		
		findPID();

		if (!isRecording)
			OnBnClickedButtonPcrRecord();

		isFirstDraw = false;
		clearSensorValue();

		// Log 파일을 사용하기 위한 폴더 생성
		CreateDirectory(L"./Log/", NULL);
	}
	else
	{
		PCREndTask();
	}
}


// PID 를 관리할 수 있는 Manager Dialog 를 생성한다.
void CMicroPCRDlg::OnBnClickedButtonEnterPidManager()
{
	static CPIDManagerDlg dlg;

	if( dlg.DoModal() == IDOK )
	{
		loadedPID = dlg.selectedPID;
		SetDlgItemText(IDC_EDIT_LOADED_PID, loadedPID);
		FileManager::saveRecentPath(FileManager::PID_PATH, loadedPID);
		if( !FileManager::loadPID(loadedPID, pids) ){
			AfxMessageBox(L"PID 를 생성하는데 문제가 발생하였습니다.\n개발자에게 문의하세요.");
		}
	}

	if( loadedPID.IsEmpty() )
		AfxMessageBox(L"PID 가 선택되지 않으면 PCR 을 시작할 수 없습니다.");

	if( !dlg.isHasPidList() ){
		loadedPID.Empty();
		SetDlgItemText(IDC_EDIT_LOADED_PID, loadedPID);
		pids.clear();
	}

	// 프로토콜이 비어있으면 Start 버튼 비활성화
//	GetDlgItem(IDC_BUTTON_PCR_START)->EnableWindow(!loadedPID.IsEmpty());

	initPidTable();
	loadPidTable();
}

void CMicroPCRDlg::blinkTask()
{
	m_blinkCounter++;
	if( m_blinkCounter > (500/TIMER_DURATION) )
	{
		m_blinkCounter = 0;

		if( isStarted )
		{
			blinkFlag = !blinkFlag;
			if( blinkFlag )
				(&m_form_main)->m_cProtocolList.SetTextBkColor(RGB(240, 200, 250));
			else
				(&m_form_main)->m_cProtocolList.SetTextBkColor(RGB(255, 255, 255));

			(&m_form_main)->m_cProtocolList.RedrawItems(m_currentActionNumber, m_currentActionNumber);
		}
		else
		{
			(&m_form_main)->m_cProtocolList.SetTextBkColor(RGB(255, 255, 255));
			(&m_form_main)->m_cProtocolList.RedrawItems(0, (&m_form_main)->m_totalActionNumber);
		}
	}
}

void CMicroPCRDlg::findPID()
{
	if ( fabs(m_prevTargetTemp - m_currentTargetTemp) < .5 ) 
		return; // if target temp is not change then do nothing 1->.5 correct
	
	double dist	 = 10000;
	int paramIdx = 0;
	
	for ( UINT i = 0; i < pids.size(); i++ )
	{
		double tmp = fabs(m_prevTargetTemp - pids[i].startTemp) + fabs(m_currentTargetTemp - pids[i].targetTemp);

		if ( tmp < dist )
		{
			dist = tmp;
			paramIdx = i;
		}
	}

	m_kp = pids[paramIdx].kp;
	m_ki = pids[paramIdx].ki;
	m_kd = pids[paramIdx].kd;
}

bool CMicroPCRDlg::shotTask(int *led_id)
{
	*led_id = 0;	// LED ON

	shotTaskTimer++;
	if (shotTaskTimer >= 2)
	{
		double lights = (double)(photodiode_h & 0x0f)*255. + (double)(photodiode_l);
		lights_arr[filter_index] = lights;

		switch (filter_index)
		{
		case 0:
			sensorValues_fam.push_back(lights);
			break;
		
		case 1:
			sensorValues_hex.push_back(lights);
			break;
		
		case 2:
			sensorValues_rox.push_back(lights);
			break;
		
		case 3:
			sensorValues_cy5.push_back(lights);
			break;
		}

		shotTaskTimer = 0;
		
		*led_id = 1;	// LED OFF

		return true;
	}
	return false;
}

int pcr_progress_num = 0;
void CMicroPCRDlg::timeTask()
{
	// elapse time 
	int elapsed_time = (int)((double)(timeGetTime()-m_startTime2)/1000.);
	int hour = elapsed_time / (60*60);
	int min = elapsed_time/60;
	int sec = elapsed_time%60;
	
	m_ElapsedTime.Format(L"%.2d:%.2d:%.2d", hour, min, sec);
	(&m_form_main)->SetDlgItemText(IDC_EDIT_ELAPSED_TIME, m_ElapsedTime);


	// timer counter increased
	m_timerCounter++;

	// 1s 마다 실행되도록 설정
	if (m_timerCounter == (1000 / TIMER_DURATION))
	{
		m_timerCounter = 0;

		if ((&m_form_main)->m_nLeftSec == 0)
		{
			m_currentActionNumber++;

			progressBar.SetPos(pcr_progress_num);
			
			// clear previous blink
			(&m_form_main)->m_cProtocolList.SetTextBkColor(RGB(255, 255, 255));
			(&m_form_main)->m_cProtocolList.RedrawItems(0, (&m_form_main)->m_totalActionNumber);

			if ((m_currentActionNumber) >= (&m_form_main)->m_totalActionNumber)
			{
				isCompletePCR = true;
				PCREndTask();
				return;
			}

			CString m_Label = actions[m_currentActionNumber].Label;
			// shot 기능 추가
			if (m_Label.Compare(L"SHOT") == 0)
			{
				m_currentActionNumber--;

				if (filter_flag[filter_index] && !filter_working)
				{
					magneto->runFilterAction(filter_index);
					filter_working = true;
					return;
				}

				if (filter_working)
				{
					if ((magneto->isFilterActionFinished() == false))
					{
						filter_workFinished = true;
						filter_working = false;
					}
					else
					{
						return;
					}
				}
				
				if (filter_workFinished)	// 필터 다 돌아갔을때 작업 처리
				{					
					bool shot_flag;
					switch (filter_index)
					{
						case 0:	// FAM
							shot_flag = shotTask(&ledControl_b);
							break;

						case 1:	// HEX
							shot_flag = shotTask(&ledControl_wg);
							break;

						case 2:	// ROX
							shot_flag = shotTask(&ledControl_g);
							break;

						case 3:	// CY5
							shot_flag = shotTask(&ledControl_r);
							break;
					}
					if (shot_flag == false) return;
				}
				
				if (filter_index < 3)
				{
					filter_index++;
					filter_workFinished = false;
					return;
				}
				else
				{
					addSensorValue();

					if (isRecording)
					{
						CString values;

						m_cycleCount++;
						double m_Time = (double)(timeGetTime() - m_recStartTime);
						values.Format(L"%6d	%8.0f	%3.1f	%3.1f	%3.1f	%3.1f\n", m_cycleCount, m_Time, lights_arr[0], lights_arr[1], lights_arr[2], lights_arr[3] );
						m_recPDFile.WriteString(values);

						values.Format(L"%6d, %8.0f, %3.1f, %3.1f, %3.1f, %3.1f", m_cycleCount, m_Time, lights_arr[0], lights_arr[1], lights_arr[2], lights_arr[3]);
						insertFieldValue(_T("PHOTODIODE"), _T("pd_cycle, pd_time, pd_value_fam, pd_value_hex, pd_value_rox, pd_value_cy5"), values);
					}

					memset(lights_arr, 0, 4);
					filter_index = 0;
					filter_workFinished = false;
					m_currentActionNumber++;
				}
			}

			else if (m_Label.Compare(L"GOTO") != 0)
			{
				m_prevTargetTemp = m_currentTargetTemp;

				m_currentTargetTemp = (int)actions[m_currentActionNumber].Temp;

				// 150828 YJ added for target temperature comparison by case
				targetTempFlag = m_prevTargetTemp > m_currentTargetTemp;

				isTargetArrival = false;

				(&m_form_main)->m_nLeftSec = (int)actions[m_currentActionNumber].Time;

				m_timeOut = m_cTimeOut * 10;

				int min = (&m_form_main)->m_nLeftSec / 60;
				int sec = (&m_form_main)->m_nLeftSec % 60;

				// current left protocol time
				CString leftTime;
				if (min == 0)
					leftTime.Format(L"%ds", sec);
				else
					leftTime.Format(L"%dm %ds", min, sec);
				(&m_form_main)->m_cProtocolList.SetItemText(m_currentActionNumber, 2, leftTime);

				findPID();	// find the proper pid values.
			}
			else	// is GOTO
			{
				if (m_leftGotoCount < 0)
					m_leftGotoCount = (int)actions[m_currentActionNumber].Time;

				if (m_leftGotoCount == 0)
					m_leftGotoCount = -1;
				else
				{
					m_leftGotoCount--;
					CString leftGoto;
					leftGoto.Format(L"%d", m_leftGotoCount);

					(&m_form_main)->m_cProtocolList.SetItemText(m_currentActionNumber, 2, leftGoto);

					// GOTO 문의 target label 값을 넣어줌
					CString tmpStr;
					tmpStr.Format(L"%d", (int)actions[m_currentActionNumber].Temp);

					int pos = 0;
					for (pos = 0; pos < (&m_form_main)->m_totalActionNumber; ++pos)
					{
						if (tmpStr.Compare(actions[pos].Label) == 0) break;
					}
					m_currentActionNumber = pos - 1;
				}
			}
			pcr_progress_num++;
		}
		else	// action 이 진행중인 경우, PCR 시간이 남은 경우
		{
			if (!isTargetArrival)	// 타임아웃 내에 목표온도에 도달 못하면 에러로 간주, PCR 종료
			{
				m_timeOut--;

				if (m_timeOut == 0)
				{
					AfxMessageBox(L"The target temperature cannot be reached!!");
					PCREndTask();
				}
			}
			else	// 목표 온도에 도달한 경우 time 카운트
			{
				(&m_form_main)->m_nLeftSec--;
				(&m_form_main)->m_nLeftTotalSec--;

				int min = (&m_form_main)->m_nLeftSec / 60;
				int sec = (&m_form_main)->m_nLeftSec % 60;

				// current left protocol time
				CString leftTime;
				if (min == 0)	leftTime.Format(L"%ds", sec);
				else			leftTime.Format(L"%dm %ds", min, sec);
				(&m_form_main)->m_cProtocolList.SetItemText(m_currentActionNumber, 2, leftTime);

				// total left protocol time
				min = (&m_form_main)->m_nLeftTotalSec / 60;
				sec = (&m_form_main)->m_nLeftTotalSec % 60;

				if (min == 0)	leftTime.Format(L"%ds", sec);
				else			leftTime.Format(L"%dm %ds", min, sec);
				(&m_form_main)->SetDlgItemText(IDC_EDIT_LEFT_PROTOCOL_TIME, leftTime);
			}
		}		
	}
}

void CMicroPCRDlg::PCREndTask()
{
	if (isRecording)
		OnBnClickedButtonPcrRecord();

	initValues();

	// 151020 YJ
	m_Timer->stopTimer();

	while( true )
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

		if( rx.state == STATE_READY ) break;

		Sleep(TIMER_DURATION);
	}

	m_Timer->startTimer(TIMER_DURATION, FALSE);

	(&m_form_main)->SetDlgItemText(IDC_BUTTON_START_PCR, L"START");
	isStarted = false;

	if (!emergencyStop)
	{
		if (isCompletePCR) AfxMessageBox(L"PCR ended!!");
		else AfxMessageBox(L"PCR incomplete!!");
	}
	else
	{
		AfxMessageBox(L"Emergency stop!(overheating)");
	}

	CTime c_time = CTime::GetCurrentTime();
	db_info_end = c_time.Format(L"%H:%M:%S");
	db_info_total.Format(L"%s", m_ElapsedTime);
	db_info_pcr = CTimeSpan(c_time - pcr_start).Format(L"%H:%M:%S");

	CString values;
	values.Format(L"'%s', '%s', '%s', '%s', '%s', '%s', %d, %d, %d, %d", db_info_date, db_info_start, db_info_end, db_info_total, db_info_pcr, db_info_ext, (&m_form_main)->filters[0], (&m_form_main)->filters[1], (&m_form_main)->filters[2], (&m_form_main)->filters[3]);
	insertFieldValue(_T("INFORMATION"), _T("info_dates, info_time_start, info_time_end, info_time_total, info_time_pcr, info_time_extraction, info_filter_fam, info_filter_hex, info_filter_rox, info_filter_cy5"), values);

	(&m_form_main)->GetDlgItem(IDC_BUTTON_PCR_OPEN)->EnableWindow(TRUE);
	(&m_form_main)->GetDlgItem(IDC_BUTTON_FAM)->EnableWindow(TRUE);
	(&m_form_main)->GetDlgItem(IDC_BUTTON_ROX)->EnableWindow(TRUE);
	(&m_form_main)->GetDlgItem(IDC_BUTTON_HEX)->EnableWindow(TRUE);
	(&m_form_main)->GetDlgItem(IDC_BUTTON_CY5)->EnableWindow(TRUE);
	
	progressBar.SetPos(0);

	pcr_progress_num = 0;
	file_created = false;
	emergencyStop = false;
	isCompletePCR = false;
}

void CMicroPCRDlg::initValues()
{
	currentCmd = CMD_READY;
	m_currentActionNumber = -1;
	(&m_form_main)->m_nLeftSec = 0;
	(&m_form_main)->m_nLeftTotalSec = 0;
	m_timerCounter = 0;
//	m_startTime = 0;

	(&m_form_main)->m_nLeftTotalSec = (&m_form_main)->m_nLeftTotalSecBackup;
	m_leftGotoCount = -1;
	m_recordingCount = 0;
	m_recStartTime = 0;
	blinkFlag = false;
	m_timeOut = 0;
	m_blinkCounter = 0;
	ledControl_wg = 1;
	ledControl_r = 1;
	ledControl_g = 1;
	ledControl_b = 1;

	m_kp = 0;
	m_ki = 0;
	m_kd = 0;

	isTargetArrival = false;

	m_prevTargetTemp = m_currentTargetTemp = 25;

	filter_index = 0;
}

LRESULT CMicroPCRDlg::OnmmTimer(WPARAM wParam, LPARAM lParam)
{
	blinkTask();

	if (isStarted || mag_started)
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
	tx.compensation = m_cCompensation;

	// pid 값을 buffer 에 복사한다.
	memcpy(&(tx.pid_p1), &(m_kp), sizeof(float));
	memcpy(&(tx.pid_i1), &(m_ki), sizeof(float));
	memcpy(&(tx.pid_d1), &(m_kd), sizeof(float));

	// integral max 값을 buffer 에 복사한다.
	memcpy(&(tx.integralMax_1), &(m_cIntegralMax), sizeof(float));

	BYTE senddata[65] = { 0, };
	BYTE readdata[65] = { 0, };
	memcpy(senddata, &tx, sizeof(TxBuffer));

	device->Write(senddata);

	if( device->Read(&rx) == 0 )
		return FALSE;

	memcpy(readdata, &rx, sizeof(RxBuffer));

	// 기기로부터 받은 온도 값을 받아와서 저장함.
	// convert BYTE pointer to float type for reading temperature value.
	memcpy(&currentTemp, &(rx.chamber_temp_1), sizeof(float));

	// 기기로부터 받은 Photodiode 값을 받아와서 저장함.
	photodiode_h = rx.photodiode_h;
	photodiode_l = rx.photodiode_l;

	if( currentCmd == CMD_FAN_OFF )
	{
		currentCmd = CMD_READY;
	}

	if( currentTemp < 0.0 )
		return FALSE;

	// 150828 YJ added
	// 150904 YJ changed, for waiting device flag
	// 150918 YJ deleted, bug by usb communication
	/*
	if( targetTempFlag && rx.targetArrival )
		targetTempFlag = false;
	*/

	// 150918 YJ added, For falling stage routine
	if( targetTempFlag && !freeRunning )
	{
		// target temp 이하가 되는 순간부터 freeRunning 으로 
		if( currentTemp <= m_currentTargetTemp )
		{
			freeRunning = true;
			freeRunningCounter = 0;
		}
	}

	if( freeRunning )
	{
		freeRunningCounter++;
		// 기기와 다르게 3초 후부터 arrival 로 인식하도록 변경
		if( freeRunningCounter >= (3000/TIMER_DURATION) )
		{
			targetTempFlag = false;
			freeRunning = false;
			freeRunningCounter = 0;
			isTargetArrival = true;
		}
	}

	if( fabs(currentTemp-m_currentTargetTemp) < m_cArrivalDelta && !targetTempFlag )
		isTargetArrival = true;

	CString tempStr;
	tempStr.Format(L"%3.1f", currentTemp);
	(&m_form_main)->SetDlgItemText(IDC_EDIT_CHAMBER_TEMP, tempStr);

	if (isTempGraphOn && isStarted)
		tempGraphDlg.addData(currentTemp);

	// Check the error from device
	static bool onceShow = true;
	if( rx.currentError == ERROR_ASSERT && onceShow ){
		onceShow = false;
		AfxMessageBox(L"Software error occured!\nPlease contact to developer");
	}
	else if( rx.currentError == ERROR_OVERHEAT && onceShow ){
		onceShow = false;
		emergencyStop = true;
		PCREndTask();
	}

	CString out;
	double lights = (double)(photodiode_h & 0x0f)*256. + (double)(photodiode_l);
	
	out.Format(L"%3.1f %d %d", lights, currentCmd, rx.state);
	(&m_form_main)->SetDlgItemText(IDC_EDIT_PHOTODIODE, out);

	// Save the recording data.
	if (isRecording)
	{
		CString values;
		double m_Time = (double)(timeGetTime() - m_recStartTime);

		m_recordingCount++;
		out.Format(L"%6d	%8.0f	%3.1f\n", m_recordingCount, m_Time, currentTemp);
		m_recFile.WriteString(out);
		//170106_1 KSY - 1
		/*
		m_cycleCount2++;
		out.Format(L"%6d	%8.0f	%3.1f\n", m_cycleCount2, m_Time, lights);
		m_recPDFile2.WriteString(out);

		values.Format(L"%8.0f, %6d, %3.1f, %6d, %3.1f", m_Time, m_recordingCount, currentTemp, m_cycleCount2, lights);
		insertFieldValue(_T("TEMPER_PD_RAW"), _T("temper_time, temper_cycle, temper_value, pd_raw_cycle, pd_raw_value"), values);
		*/
		//170106_1 KSY - 2
		// for log message per 1 sec
		if (m_recordingCount % 20 == 0)
		{
			int elapsed_time = (int)((double)(timeGetTime() - m_startTime2) / 1000.);
			int min = elapsed_time / 60;
			int sec = elapsed_time % 60;
			CString elapseTime, lineTime, totalTime;
			elapseTime.Format(L"%dm %ds", min, sec);

			min = (&m_form_main)->m_nLeftSec / 60;
			sec = (&m_form_main)->m_nLeftSec % 60;

			// current left protocol time
			if( min == 0 )
				lineTime.Format(L"%ds", sec);
			else
				lineTime.Format(L"%dm %ds", min, sec);
			
			// total left protocol time
			min = (&m_form_main)->m_nLeftTotalSec / 60;
			sec = (&m_form_main)->m_nLeftTotalSec % 60;

			if( min == 0 )
				totalTime.Format(L"%ds", sec);
			else
				totalTime.Format(L"%dm %ds", min, sec);

			CString log;
			log.Format(L"cmd: %d, targetTemp: %3.1f, temp: %s, elapsed time: %s, line Time: %s, protocol Time: %s, device TargetArr: %d, mfc TargetArr: %d, free Running: %d, free Running Counter: %d, ArrivalDelta: %3.1f, tempFlag: %d, photodiode: %3.1f\n", 
				currentCmd, m_currentTargetTemp, tempStr, elapseTime, lineTime, totalTime, (int)rx.targetArrival, (int)isTargetArrival, (int)freeRunning, (int)freeRunningCounter, m_cArrivalDelta, (int)targetTempFlag, lights);
			FileManager::log(log, magneto->getSerialNumber());
		}
	}

	return FALSE;
}

void CMicroPCRDlg::addSensorValue()
{
	// 기존에 저장된 차트를 지운 후, 
	// 새로 저장한 double 값 vector 에 저장하여
	// 이 값을 기반으로 다시 그림.

	(&m_form_result)->m_Chart.DeleteAllData();

	int size_fam = sensorValues_fam.size();
	int size_hex = sensorValues_hex.size();
	int size_rox = sensorValues_rox.size();
	int size_cy5 = sensorValues_cy5.size();
	double *data_fam = new double[size_fam * 2];
	double *data_hex = new double[size_hex * 2];
	double *data_rox = new double[size_rox * 2];
	double *data_cy5 = new double[size_cy5 * 2];
	
	if (filter_flag[0] == true)
	{
		int	nDims_fam = 2, dims_fam[2] = { 2, size_fam };
		for (int i = 0; i<size_fam; ++i)
		{
			data_fam[i] = i;
			data_fam[i + size_fam] = sensorValues_fam[i];
			//data_fam[i + size_fam] = 500.0;
		}
		(&m_form_result)->m_Chart.SetDataColor((&m_form_result)->m_Chart.AddData(data_fam, nDims_fam, dims_fam), RGB(0, 0, 255));
	}
	
	if (filter_flag[1] == true)
	{
		int	nDims_hex = 2, dims_hex[2] = { 2, size_hex };
		for (int i = 0; i < size_hex; ++i)
		{
			data_hex[i] = i;
			data_hex[i + size_hex] = sensorValues_hex[i];
			//data_hex[i + size_hex] = 1000.0;
		}
		(&m_form_result)->m_Chart.SetDataColor((&m_form_result)->m_Chart.AddData(data_hex, nDims_hex, dims_hex), RGB(0, 255, 0));
	}

	if (filter_flag[2] == true)
	{
		int	nDims_rox = 2, dims_rox[2] = { 2, size_rox };
		for (int i = 0; i < size_rox; ++i)
		{
			data_rox[i] = i;
			data_rox[i + size_rox] = sensorValues_rox[i];
			//data_rox[i + size_rox] = 1500.0;
		}
		(&m_form_result)->m_Chart.SetDataColor((&m_form_result)->m_Chart.AddData(data_rox, nDims_rox, dims_rox), RGB(0, 128, 0));
	}
	
	if (filter_flag[3] == true)
	{
		int	nDims_cy5 = 2, dims_cy5[2] = { 2, size_cy5 };
		for (int i = 0; i < size_cy5; ++i)
		{
			data_cy5[i] = i;
			data_cy5[i + size_cy5] = sensorValues_cy5[i];
			//data_cy5[i + size_cy5] = 2000.0;
		}
		(&m_form_result)->m_Chart.SetDataColor((&m_form_result)->m_Chart.AddData(data_cy5, nDims_cy5, dims_cy5), RGB(255, 0, 0));
	}

	InvalidateRect(&CRect(15, 350, 1155, 760));
	//(&m_form_result)->InvalidateRect(&CRect(0, 0, 562, 322));
	(&m_form_result)->Invalidate(FALSE);
}

void CMicroPCRDlg::clearSensorValue()
{
	sensorValues_fam.clear();
	sensorValues_hex.clear();
	sensorValues_rox.clear();
	sensorValues_cy5.clear();

	sensorValues_fam.push_back(1.0);
	sensorValues_hex.push_back(1.0);
	sensorValues_rox.push_back(1.0);
	sensorValues_cy5.push_back(1.0);

	(&m_form_result)->m_Chart.DeleteAllData();
	InvalidateRect(&CRect(15, 350, 1155, 760));
}

void CMicroPCRDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	CDialog::OnMoving(fwSide, pRect);

	if (isTempGraphOn)
	{
		CRect parent_rect, rect;
		tempGraphDlg.GetClientRect(&rect);
		GetWindowRect(&parent_rect);

		tempGraphDlg.SetWindowPos(this, parent_rect.right + 10, parent_rect.top,
			rect.Width(), rect.Height(), SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

void CMicroPCRDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (isTempGraphOn){
		CRect parent_rect, rect;
		tempGraphDlg.GetClientRect(&rect);
		GetWindowRect(&parent_rect);

		tempGraphDlg.SetWindowPos(this, parent_rect.right + 10, parent_rect.top,
			rect.Width(), rect.Height(), SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

void CMicroPCRDlg::connectMagneto()
{
	int magneto_flag_cnt = 0;

	if (!magneto->isConnected())
	{
		while (!mag_connected)
		{
			if (magneto_flag_cnt == 2)
			{
				(&m_form_main)->SetDlgItemText(IDC_EDIT_MAGNETO_STATUS, L"Disconnected");
				AfxMessageBox(L"Devices connection failure. please check devices. [magneto]");
				SendMessage(WM_CLOSE);	// 프로그램 종료
				break;
			}

			(&m_form_main)->SetDlgItemText(IDC_EDIT_MAGNETO_STATUS, L"Connecting...");

			vector<CString> list;
			magneto->searchPort(list);

			if (list.size() <= 0)
			{
				magneto_flag_cnt++;
				continue;				
			}
			else
			{
				for (UINT i = 0; i < list.size(); ++i)
				{
					CString comport = list[i];
					comport.Replace(L"COM", L"");

					DriverStatus::Enum res = magneto->connect(_ttoi(comport));


					if (res == DriverStatus::CONNECTED)
					{
						(&m_form_main)->SetDlgItemText(IDC_EDIT_MAGNETO_STATUS, L"Connected");
						(&m_form_main)->GetDlgItem(IDC_BUTTON_START_PCR)->EnableWindow(TRUE);
						magneto->setHwnd(GetSafeHwnd());
						mag_connected = true;
						break;
					}
					else if (res == DriverStatus::NOT_CONNECTED)
					{
						(&m_form_main)->SetDlgItemText(IDC_EDIT_MAGNETO_STATUS, L"Connecting...");
						magneto_flag_cnt++;
						continue;
					}
					else
					{
						(&m_form_main)->SetDlgItemText(IDC_EDIT_MAGNETO_STATUS, L"Too few slaves");
						AfxMessageBox(L"All motors are not connected! Please check your device.");

						SendMessage(WM_CLOSE);	// 프로그램 종료
					}
				}
			}
		}
	}
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	for (int i = 0; i < argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	printf("\n");
	return 0;
};

void CMicroPCRDlg::makeDatabaseTable(CString path)
{
	USES_CONVERSION;
	const char* m_Path = T2A(path);

	const char* m_SQLs[7];
	m_SQLs[0] = "CREATE TABLE INFORMATION		( _table_no	INTEGER DEFAULT 0,	info_dates	TEXT, info_time_start	TEXT, info_time_end	TEXT, info_time_total	TEXT, info_time_pcr	TEXT, info_time_extraction	TEXT, info_filter_fam	INTEGER, info_filter_hex	INTEGER, info_filter_rox	INTEGER, info_filter_cy5	INTEGER )";
	m_SQLs[1] = "CREATE TABLE MAGNETO_PROTOCOL	( _table_no	INTEGER DEFAULT 1,	mag_protocol	TEXT )";
	m_SQLs[2] = "CREATE TABLE PCR_PROTOCOL		( _table_no	INTEGER DEFAULT 2,	pcr_protoc_no	TEXT, pcr_protoc_temper	INTEGER, pcr_protoc_time	INTEGER )";
	m_SQLs[3] = "CREATE TABLE PCR_CONSTANT		( _table_no	INTEGER DEFAULT 3,	cst_max_action	INTEGER, cst_temp_cvrg_timeout	INTEGER, cst_t_temp_arrv_delta	REAL, cst_graph_min_y_axis	INTEGER, cst_graph_max_y_axis	INTEGER, cst_integral_max	INTEGER, cst_compensation	INTEGER )";
	m_SQLs[4] = "CREATE TABLE TEMPERATURE_PID	( _table_no	INTEGER DEFAULT 4,	pid_number	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, pid_start_temper	REAL, pid_target_temper	REAL, pid_kp	REAL, pid_kd	REAL, pid_ki	REAL )";
	m_SQLs[5] = "CREATE TABLE PHOTODIODE		(_table_no	INTEGER DEFAULT 5,	pd_cycle	INTEGER,	pd_time	REAL,	pd_value_fam	REAL DEFAULT 0,	pd_value_hex	REAL DEFAULT 0,	pd_value_rox	REAL DEFAULT 0,	pd_value_cy5	REAL DEFAULT 0 )";
	m_SQLs[6] = "CREATE TABLE TEMPER_PD_RAW		( _table_no	INTEGER DEFAULT 6,	temper_time	REAL,	temper_cycle	INTEGER,	temper_value	REAL,	pd_raw_cycle	INTEGER,	pd_raw_value	REAL)";
	int sql_length = sizeof(m_SQLs) / sizeof(const char*);
	
	int rst = sqlite3_open(m_Path, &pSQLite3);
	if (rst)
	{
		CString SQLERR;
		SQLERR.Format(L"Can't open database: %s", sqlite3_errmsg(pSQLite3));
		AfxMessageBox(SQLERR);

		sqlite3_close(pSQLite3);
		pSQLite3 = NULL;
	}
	else
	{
		// DB 파일 생성안하고 기존의 파일을 불러오면 기존 테이블 모두 지우고 다시 생성
		sqlite3_exec(pSQLite3, "DROP TABLE TEMPERATURE_PID",	callback, 0, &szErrMsg);
		sqlite3_exec(pSQLite3, "DROP TABLE MAGNETO_PROTOCOL",	callback, 0, &szErrMsg);
		sqlite3_exec(pSQLite3, "DROP TABLE INFORMATION",	callback,	0,	&szErrMsg);
		sqlite3_exec(pSQLite3, "DROP TABLE PCR_PROTOCOL",	callback,	0,	&szErrMsg);
		sqlite3_exec(pSQLite3, "DROP TABLE PCR_CONSTANT",	callback,	0,	&szErrMsg);
		sqlite3_exec(pSQLite3, "DROP TABLE TEMPER_PD_RAW",	callback,	0,	&szErrMsg);
		sqlite3_exec(pSQLite3, "DROP TABLE PHOTODIODE",	callback,	0,	&szErrMsg);
		
		// 테이블 생성
		for (int i = 0; i < sql_length; i++)
			rst = sqlite3_exec(pSQLite3, m_SQLs[i], callback, 0, &szErrMsg);	// create db table
	}
}

void CMicroPCRDlg::initDatabaseTable()
{
	CString values;

	// DB INFORMATION table init
	CTime c_time = CTime::GetCurrentTime();
	db_info_date = c_time.Format(L"%Y-%m-%d");
	db_info_start = c_time.Format(L"%H:%M:%S");
	

	// DB PCR_PROTOCOL table init
	for (int i = 0; i < (&m_form_main)->m_totalActionNumber; i++)
	{
		values.Format(L"'%s', %d, %d", actions[i].Label, (int)actions[i].Temp, (int)actions[i].Time);
		insertFieldValue(_T("PCR_PROTOCOL"), _T("pcr_protoc_no, pcr_protoc_temper, pcr_protoc_time"), values);
	}

	// DB PCR_CONSTANT table init
	values.Format(L"%d, %d, %f, %d, %d, %f, %d", m_cMaxActions, m_cTimeOut, m_cArrivalDelta, m_cGraphYMin, m_cGraphYMax, m_cIntegralMax, m_cCompensation);
	insertFieldValue(_T("PCR_CONSTANT"), _T("cst_max_action, cst_temp_cvrg_timeout, cst_t_temp_arrv_delta, cst_graph_min_y_axis, cst_graph_max_y_axis, cst_integral_max, cst_compensation"), values);

	// DB TEMPERATURE_PID table init
	for (int row = 1; row < m_cPidTable.GetRowCount(); row++)
	{
		CString startTemp = m_cPidTable.GetItemText(row, 1);
		CString targetTemp = m_cPidTable.GetItemText(row, 2);
		CString kp = m_cPidTable.GetItemText(row, 3);
		CString kd = m_cPidTable.GetItemText(row, 4);
		CString ki = m_cPidTable.GetItemText(row, 5);

		values.Format(L"%s, %s, %s, %s, %s", startTemp, targetTemp, kp, kd, ki);
		insertFieldValue(_T("TEMPERATURE_PID"), _T("pid_start_temper, pid_target_temper, pid_kp, pid_kd, pid_ki"), values);
	}

	// DB MAGNETO_PROTOCOL table init
	try
	{
		CStdioFile file;
		file.Open(L"MagnetoProtocol.txt", CStdioFile::modeRead);

		CString line;
		while (file.ReadString(line))
		{
			line.MakeLower();
			
			if (line.Compare(L"") == 0 || line.Compare(L" ") == 0) continue;

			values.Format(L"'%s'", line);
			insertFieldValue(_T("MAGNETO_PROTOCOL"), _T("mag_protocol"), values);
		}
	} catch (CFileException* e) {}
}

void CMicroPCRDlg::insertFieldValue(CString tableName, CString field, CString value)
{
	CString sql_temp;
	sql_temp.Format(_T("INSERT INTO %s ( %s ) values ( %s )"), tableName, field, value);
	
	USES_CONVERSION;
	const char* sql = T2A(sql_temp);

	sqlite3_exec(pSQLite3, sql, callback, 0, &szErrMsg);
}

void CMicroPCRDlg::OnBnClickedButtonStart()
{
	CString labgFilePath;

	filter_flag = (&m_form_main)->filters;
	if (!filter_flag[0] && !filter_flag[1] && !filter_flag[2] && !filter_flag[3]) 
	{
		AfxMessageBox(L"Invalid start options!!\nplease checking the at least one option.");
		return;
	}

	if (!file_created)
	{
		CFileDialog m_fDlg(FALSE, _T("*.labg"), _T(".labg"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR, _T("LabGenius files (*.labg)|*.labg|"), NULL);

		if (m_fDlg.DoModal() == IDOK)
		{
			labgFilePath = m_fDlg.GetPathName();

			if (m_fDlg.GetFileName().GetLength() > 5)
			{
				makeDatabaseTable(labgFilePath);	// labg(sqlite db) 파일 생성 및 테이블 생성
				initDatabaseTable();	// 테이블 내용 초기화

				file_created = true;

				(&m_form_main)->GetDlgItem(IDC_BUTTON_PCR_OPEN)->EnableWindow(FALSE);
				(&m_form_main)->GetDlgItem(IDC_BUTTON_FAM)->EnableWindow(FALSE);
				(&m_form_main)->GetDlgItem(IDC_BUTTON_ROX)->EnableWindow(FALSE);
				(&m_form_main)->GetDlgItem(IDC_BUTTON_HEX)->EnableWindow(FALSE);
				(&m_form_main)->GetDlgItem(IDC_BUTTON_CY5)->EnableWindow(FALSE);
			}
		}
	}

	if (!mag_started && !isStarted)
	{
		// 파일이 생성되었고, 모터, pcr 작동이 아니면 모터 작동
		if (file_created)
		{
			previousAction = 0;

			(&m_form_main)->SetDlgItemText(IDC_BUTTON_START_PCR, L"STOP");
			
			magneto->start();
			SetTimer(Magneto::TimerRuntaskID, Magneto::TimerRuntaskDuration, NULL);

			m_Timer->stopTimer();
			Sleep(TIMER_DURATION);
			m_Timer->startTimer(TIMER_DURATION, FALSE);

			mag_started = true;

			m_startTime2 = timeGetTime();
		}
	}

	else if (mag_started)
	{
		// 모터가 돌아가는 중이므로 모터 정지

		magneto->stop();
		KillTimer(Magneto::TimerRuntaskID);
		m_Timer->stopTimer();

		mag_started = false;
		file_created = false;
		(&m_form_main)->SetDlgItemText(IDC_BUTTON_START_PCR, L"START");
		(&m_form_main)->GetDlgItem(IDC_BUTTON_PCR_OPEN)->EnableWindow(TRUE);
		(&m_form_main)->GetDlgItem(IDC_BUTTON_FAM)->EnableWindow(TRUE);
		(&m_form_main)->GetDlgItem(IDC_BUTTON_ROX)->EnableWindow(TRUE);
		(&m_form_main)->GetDlgItem(IDC_BUTTON_HEX)->EnableWindow(TRUE);
		(&m_form_main)->GetDlgItem(IDC_BUTTON_CY5)->EnableWindow(TRUE);
	}

	else if (isStarted)
	{
		// 모터는 정지되어 있고, pcr이 돌아가고 있으므로 pcr 정지

		OnBnClickedButtonPcrStart();

		mag_started = false;
		file_created = false;
		(&m_form_main)->SetDlgItemText(IDC_BUTTON_START_PCR, L"PCR Run");
		(&m_form_main)->GetDlgItem(IDC_BUTTON_PCR_OPEN)->EnableWindow(TRUE);
		(&m_form_main)->GetDlgItem(IDC_BUTTON_FAM)->EnableWindow(TRUE);
		(&m_form_main)->GetDlgItem(IDC_BUTTON_ROX)->EnableWindow(TRUE);
		(&m_form_main)->GetDlgItem(IDC_BUTTON_HEX)->EnableWindow(TRUE);
		(&m_form_main)->GetDlgItem(IDC_BUTTON_CY5)->EnableWindow(TRUE);
	}
}

void CMicroPCRDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == Magneto::TimerRuntaskID)
	{
		int driverChack = magneto->runTask();
		if (driverChack == 1)
		{
			LPARAM test = magneto->getCurrentAction();

			int currentAction = LOWORD(test);
			int currentSubAction = HIWORD(test);
			selectTreeItem(currentAction, currentSubAction);

			KillTimer(Magneto::TimerRuntaskID);
			OnBnClickedButtonStart();
			AfxMessageBox(L"시작버튼을 다시 누르세요.\n[magneto driver error]");
			return;
		} 

		else if (driverChack == 2)
		{
			//OnBnClickedButtonStart();
			KillTimer(Magneto::TimerRuntaskID);
			AfxMessageBox(L"Limit 스위치가 설정되어 task 가 종료되었습니다.\n기기를 확인하세요.");
			return;
		}

		// 모든 magneto protocol 이 종료됨
		if (magneto->isIdle())
		{
			db_info_ext.Format(L"%s", m_ElapsedTime);

			mag_started = false;

			KillTimer(Magneto::TimerRuntaskID);

			pcr_start = CTime::GetCurrentTime();
			OnBnClickedButtonPcrStart();

			return;
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void CMicroPCRDlg::selectTreeItem(int rootIndex, int childIndex)
{
	int curRootIndex = 0, curChildIndex = 0;
	HTREEITEM rootItem = actionTreeCtrl.GetRootItem();

	// 먼저 rootIndex 에 맞춰 다음 root 를 선택한다.
	for (int i = 0; i < rootIndex; ++i)
		rootItem = actionTreeCtrl.GetNextSiblingItem(rootItem);

	// childIndex 에 맞춰 child 들 중 하나를 선택한다.
	HTREEITEM childItem = actionTreeCtrl.GetChildItem(rootItem);
	for (int i = 0; i < childIndex; ++i)
		childItem = actionTreeCtrl.GetNextItem(childItem, TVGN_NEXT);

	actionTreeCtrl.SelectItem(childItem);
	//actionTreeCtrl.SetFocus();
}

HRESULT CMicroPCRDlg::OnMotorPosChanged(WPARAM wParam, LPARAM lParam)
{
	CString target = L"", cmd = L"";

	MotorPos *motorPos = reinterpret_cast<MotorPos *>(wParam);
	int currentAction = LOWORD(lParam);
	int currentSubAction = HIWORD(lParam);
	double cmdPos = motorPos->cmdPos;
	double targetPos = motorPos->targetPos;
	int driverErr = motorPos->driverErr;
	
	target.Format(L"%0.3f", targetPos);
	cmd.Format(L"%0.3f", cmdPos);

	// 후에 배열을 이용한 정리 필요
	if (magneto->getCurrentMotor() == MotorType::PUMPING)
	{
		SetDlgItemText(IDC_EDIT_Y_TARGET, target);
		SetDlgItemText(IDC_EDIT_Y_CURRENT, cmd);
	}
	else if (magneto->getCurrentMotor() == MotorType::CHAMBER)
	{
		SetDlgItemText(IDC_EDIT_MAGNET_TARGET, target);
		SetDlgItemText(IDC_EDIT_MAGNET_CURRENT, cmd);
	}
	else if (magneto->getCurrentMotor() == MotorType::FILTER)
	{
		SetDlgItemText(IDC_EDIT_LOAD_TARGET, target);
		SetDlgItemText(IDC_EDIT_LOAD_CURRENT, cmd);
	}

	if (driverErr > 0)
	{
		
	}
	selectTreeItem(currentAction, currentSubAction);

	if( currentAction == previousAction )
	{
		previousAction++;
		progressBar.SetPos(previousAction);
	}

	return 0;
}


void CMicroPCRDlg::OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (m_pwndShow != NULL)
	{
		m_pwndShow->ShowWindow(SW_HIDE);
		m_pwndShow = NULL;
	}

	int index = m_Tab.GetCurSel();
	switch (index)
	{
	case 0:
		m_form_main.ShowWindow(SW_SHOW);
		m_pwndShow = &m_form_main;
		break;

	case 1:
		m_form_result.ShowWindow(SW_SHOW);
		m_pwndShow = &m_form_result;
		break;
	}

	*pResult = 0;
}


void CMicroPCRDlg::OnBnClickedCheckTemperature()
{
	CButton* check = (CButton*)GetDlgItem(IDC_CHECK_TEMPERATURE);
	isTempGraphOn = check->GetCheck();

	if (isTempGraphOn)
	{
		tempGraphDlg.Create(IDD_DIALOG_TEMP_GRAPH, this);

		CRect parent_rect, rect;
		tempGraphDlg.GetClientRect(&rect);
		GetWindowRect(&parent_rect);

		tempGraphDlg.SetWindowPos(this, parent_rect.right + 10, parent_rect.top,
			rect.Width(), rect.Height(), SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
	else
		tempGraphDlg.DestroyWindow();
}


void CMicroPCRDlg::OnBnClickedButtonPcrRecord()
{
	if (!isRecording)
	{
		CreateDirectory(L"./Record/", NULL);

		CString fileName, fileName2, fileName3;
		CTime time = CTime::GetCurrentTime();
		fileName = time.Format(L"./Record/%Y%m%d-%H%M-%S.txt");
		fileName2 = time.Format(L"./Record/pd%Y%m%d-%H%M-%S.txt");
		//170106_1 KSY - 1
		//fileName3 = time.Format(L"./Record/pdRaw%Y%m%d-%H%M-%S.txt");
		//170106_1 KSY - 2

		m_recFile.Open(fileName, CStdioFile::modeCreate | CStdioFile::modeWrite);
		m_recFile.WriteString(L"Number	Time	Temperature\n");

		m_recPDFile.Open(fileName2, CStdioFile::modeCreate | CStdioFile::modeWrite);
		m_recPDFile.WriteString(L"Cycle	Time	FAM	HEX	ROX	CY5\n");
		//170106_1 KSY - 1
		/*
		m_recPDFile2.Open(fileName3, CStdioFile::modeCreate | CStdioFile::modeWrite);
		m_recPDFile2.WriteString(L"Cycle	Time	Value\n");
		*/
		//170106_1 KSY - 2
		m_recordingCount = 0;
		m_cycleCount = 0;
		//170106_1 KSY - 1
		//m_cycleCount2 = 0;
		//170106_1 KSY - 2
		m_recStartTime = timeGetTime();
	}
	else
	{
		m_recFile.Close();
		m_recPDFile.Close();
		m_recPDFile2.Close();
	}

	isRecording = !isRecording;
}

void CMicroPCRDlg::OnBnClickedButtonResult()
{
	// static CResultDlg dlg;

	CFileFind finder;
	BOOL bWorking = finder.FindFile(_T("*.exe"));
	BOOL flag = FALSE;
	while (bWorking)
	{
		bWorking = finder.FindNextFile();

		if (finder.GetFileName().CompareNoCase(L"LabGenius_PCR_viewer.exe") == 0)
		{
			flag = TRUE;
			break;
		}
	}

	if (flag)
		ShellExecute(NULL, _T("open"), _T("LabGenius_PCR_viewer.exe"), NULL, NULL, SW_SHOW);
	else
		AfxMessageBox(L"\'LabGenius_PCR_viewer.exe\' is not found");
}

bool f_test = false;
void CMicroPCRDlg::OnBnClickedButtonTest()
{
	f_test = !f_test;
	if (f_test)
		SetWindowPos(NULL, 100, 100, 1165, 571, SWP_NOZORDER);
	else
		SetWindowPos(NULL, 100, 100, 585, 385, SWP_NOZORDER);
}
