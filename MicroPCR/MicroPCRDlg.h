
// MicroPCRDlg.h : 헤더 파일
//

#pragma once
#include "DeviceConnect.h"
#include ".\Lib\sqlite3.h"
#include "afxcmn.h"
#include ".\gridctrl_src\gridctrl.h"

#include "UserDefs.h"
#include "mmtimers.h"

#include "TempGraphDlg.h"
#include "CommThread.h"

// For magneto
#include "Magneto.h"
#include "afxwin.h"

#include "Form_Main.h"
#include "Form_Result.h"

// CMicroPCRDlg 대화 상자
class CMicroPCRDlg : public CDialog
{
private:
	CDeviceConnect *device;
	CGridCtrl m_cPidTable;
	CMMTimers* m_Timer;
	
	vector< double > sensorValues_fam;
	vector< double > sensorValues_hex;
	vector< double > sensorValues_rox;
	vector< double > sensorValues_cy5;

	double lights_arr[4];

	int m_cGraphYMin;
	int m_cGraphYMax;
	bool isFirstDraw;
	void addSensorValue();
	void clearSensorValue();

private:
	vector< PID > pids;
	float m_kp, m_ki, m_kd;
	void findPID();
	int m_cTimeOut;
	float m_cArrivalDelta;

	void initPidTable();
	void loadPidTable();
	void loadConstants();
	void saveConstants();

	// for initialize
	void initValues();

	CString m_sProtocolName;
	
	int m_currentActionNumber;

	// for temporary
	void blinkTask();
	void timeTask();
	void PCREndTask();
	
	int m_blinkCounter, m_timerCounter;

	bool blinkFlag;
	bool isStarted;	// PCR 플래그
	bool isCompletePCR;
	bool isTargetArrival;

//	double m_startTime;
	double m_startTime2;

	double m_prevTargetTemp, m_currentTargetTemp;

	int m_timeOut;

	int m_leftGotoCount;
	int ledControl_wg, ledControl_r, ledControl_g, ledControl_b;

	
	// 버튼에 따라 현재 보낼 command 값을 설정한다.
	BYTE currentCmd;
	bool isFanOn, isLedOn;

	// Photodiode 값을 저장
	BYTE photodiode_h, photodiode_l;

	float m_cIntegralMax;

	CString loadedPID;

	// For target temp comparison
	bool targetTempFlag;
	bool freeRunning;
	int freeRunningCounter;
	bool emergencyStop;

	// 151203 siri For magneto
	CTreeCtrl actionTreeCtrl;
	CProgressCtrl progressBar;
	CMagneto *magneto;
	int previousAction;

	void selectTreeItem(int rootIndex, int childIndex);

	int protocol_size;

	bool isStarted2;	// 시작 버튼 상태

// 생성입니다.
public:
	CMicroPCRDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	virtual ~CMicroPCRDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_MICROPCR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	virtual LRESULT OnmmTimer(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	BYTE m_cCompensation;
	BOOL OnDeviceChange(UINT nEventType, DWORD dwData);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void Serialize(CArchive& ar);
	afx_msg void OnBnClickedButtonConstantsApply();
	afx_msg void OnBnClickedButtonFanControl();
	afx_msg void OnBnClickedButtonEnterPidManager();
	afx_msg void OnBnClickedButtonLedControl();
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HRESULT OnMotorPosChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckTemperature();
	afx_msg void OnBnClickedButtonResult();
	afx_msg void OnBnClickedButtonTest();

	void connectMagneto();
	void OnBnClickedButtonPcrStart();

	CTabCtrl m_Tab;
	CForm_Main m_form_main;
	CForm_Result m_form_result;
	CWnd* m_pwndShow;
	int m_cMaxActions;
	CTempGraphDlg tempGraphDlg;
	BOOL isTempGraphOn;
	void OnBnClickedButtonPcrRecord();

	bool isRecording;
	CStdioFile m_recFile, m_recPDFile, m_recPDFile2;
	int m_recordingCount, m_cycleCount, m_cycleCount2;
	DWORD m_recStartTime;

	CBitmap bmpRecNotWork;
	CBitmap bmpRecWork;

	// SQLite 선언 (Labg file)
	void CMicroPCRDlg::makeDatabaseTable(CString path);
	void CMicroPCRDlg::initDatabaseTable();
	void CMicroPCRDlg::insertFieldValue(CString tableName, CString field, CString value);

	sqlite3 *pSQLite3; // SQLite DB 객체 저장 변수
	char    *szErrMsg; // Error 발생시 메세지를 저장하는 변수

	bool mag_connected;
	bool mag_started;
	bool file_created;

	CString db_info_date;
	CString db_info_start;
	CString db_info_end;
	CString db_info_total;
	CString db_info_pcr;
	CString db_info_ext;
	CString m_ElapsedTime;
	
	CTime pcr_start;

	bool *filter_flag;
	int filter_index;
	bool filter_working;
	bool filter_workFinished;

	bool CMicroPCRDlg::shotTask(int *led_id);

	int shotTaskTimer;
};
