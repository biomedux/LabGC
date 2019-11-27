#pragma once


// CDeviceSetup 대화 상자

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

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
};
