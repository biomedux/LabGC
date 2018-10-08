#pragma once

#include "afxcmn.h"

#include "UserDefs.h"


// CForm_Main 대화 상자입니다.

class CForm_Main : public CDialog
{
	DECLARE_DYNAMIC(CForm_Main)

public:
	CForm_Main(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CForm_Main();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_FORM_MAIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnBnClickedButtonFam();
	afx_msg void OnBnClickedButtonCy5();
	afx_msg void OnBnClickedButtonHex();
	afx_msg void OnBnClickedButtonRox();
	afx_msg void OnBnClickedButtonPcrOpen();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnStnClickedStaticProgressbar4();

	CBitmap offImg;
	CBitmap famImg;
	CBitmap hexImg;
	CBitmap roxImg;
	CBitmap cy5Img;

	bool filters[4];
//		 filters[0] = Fam filter
//		 filters[1] = Hex filter
//		 filters[2] = Rox filter
//		 filters[3] = Cy5 filter

	void readProtocol(CString path);
	void displayList();
	void enableWindows();
	CString getProtocolName(CString path);

	bool isConnected;
	int m_nLeftSec, m_nLeftTotalSec;
	int m_totalActionNumber;
	CString m_sProtocolName;
	CListCtrl m_cProtocolList;

	int m_nLeftTotalSecBackup;

	int pcr_progressbar_max;
	afx_msg void OnBnClickedBtnFlttest();
};
