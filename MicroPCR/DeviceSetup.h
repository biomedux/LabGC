#pragma once


// CDeviceSetup 대화 상자

#include "Magneto.h"
#include "DeviceConnect.h"

class CDeviceSetup : public CDialogEx
{
	DECLARE_DYNAMIC(CDeviceSetup)

public:
	CDeviceSetup(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CDeviceSetup();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_DEVICE_SETUP };
#endif

private:
	CDeviceConnect* device;
	CMagneto* magneto;

	CComboBox deviceList;
	CComboBox portList;

	void initConnection();
	void initDeviceList();

	CString makeChecksum(CString lineData);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonSetMagnetoSerial();
	afx_msg void OnBnClickedButtonMakeHexFile();
	afx_msg void OnBnClickedButtonResetDevice();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
