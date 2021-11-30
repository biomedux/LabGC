﻿#pragma once

#include "Chart.h"
#include "mmtimers.h"

#include "Magneto.h"
#include "DeviceConnect.h"

#include ".\gridctrl_src\gridctrl.h"

// CMainDialog 대화 상자

#define TIMEOUT_CONST 1000
#define PORT 65535 // 211130 KBH Socket Server Port

class CMainGraphDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CMainGraphDialog)

private:
	CString m_strStylesPath;
	HICON m_hIcon;
	CMMTimers* m_Timer;
	CDeviceConnect* device;
	CMagneto* magneto;

	CGridCtrl resultTable;
	CXYChart m_Chart;

	vector< double > sensorValuesFam;
	vector< double > sensorValuesHex;
	vector< double > sensorValuesRox;
	vector< double > sensorValuesCy5;

	CComboBox protocolList;
	CComboBox deviceList;
	CProgressCtrl progressStatus;
	vector<Protocol> protocols;
	vector<PID> pids;

	CString lastSelectedProtocol;
	Protocol currentProtocol;

	CBitmap offImg;
	CBitmap famImg;
	CBitmap hexImg;
	CBitmap roxImg;
	CBitmap cy5Img;

	void loadProtocolList();
	void loadMagnetoProtocol();

	void calcTotalTime();
	void initPCRDevices();
	void initConnection();
	void initResultTable();
	void initState();	// 210120 KBH initialize state 

	bool isProtocolLoaded;
	bool isConnected;
	bool isStarted;

	int magnetoProtocolIdx;

	int currentCmd;
	bool isFanOn, isLedOn;

	// Photodiode 값을 저장
	BYTE photodiode_h, photodiode_l;

	int m_prevTargetTemp;
	int m_currentTargetTemp;
	int m_timerCounter;
	int m_leftGotoCount;
	bool isFirstDraw;
	bool isCompletePCR;
	bool isTargetArrival;
	bool emergencyStop;
	bool freeRunning;
	int freeRunningCounter;

	float m_kp, m_ki, m_kd;
	void findPID();
	int m_timeOut;
	float m_cArrivalDelta;

	int maxCycles;
	int compensation;
	float integralMax;
	float displayDelta;
	float flRelativeMax;

	int totalLeftSec;
	int leftSec;
	int m_currentActionNumber;
	bool targetTempFlag;
	int filterIndex;
	bool filterRunning;
	int shotCounter;

	int ledControl_wg, ledControl_r, ledControl_g, ledControl_b;

	void loadConstants();
	void initValues();
	void timeTask();
	void cleanupTask();
	void PCREndTask();

	int currentCycle;
	void initChart();
	void initProtocol();
	void setChartValue();
	void clearChartValue();
	void setCTValue(CString dateTime, vector<double>& sensorValue, int resultIndex, int filterIndex);

	// For log
	CStdioFile m_recFile, m_recPDFile;
	int recordingCount;
	DWORD recStartTime;
	void initLog();
	void clearLog();
	bool logStopped;

	bool useFam;
	bool useHex;
	bool useRox;
	bool useCy5;

	CRect m_graphRect; // 211117 KBH graph plot Rect 

	// 211130 KBH winsocket2 parameters (for connecting curve fitting server)
	WSADATA m_wsdata;
	SOCKET m_socket;
	SOCKADDR_IN m_addr;

	// 211130 KBH winsocket2 functions (for connecting curve fitting server)
	void initWinSocket(); // run server process and start socket
	void CurveFit(vector<double>& sensorValues);
	void closeWinSocket(); // it is not used 

public:
	CMainGraphDialog(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CMainGraphDialog();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_MAIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonSetup();
	afx_msg void OnLbnSelchangeComboProtocols();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual LRESULT OnmmTimer(WPARAM wParam, LPARAM lParam);

	afx_msg void OnBnClickedButtonFilterFam();
	afx_msg void OnBnClickedButtonFilterHex();
	afx_msg void OnBnClickedButtonFilterRox();
	afx_msg void OnBnClickedButtonFilterCy5();
};
