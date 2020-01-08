#pragma once
#include "resource.h"

// CProgressDialog 대화 상자

class CProgressDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CProgressDialog)

public:
	CProgressDialog(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CProgressDialog();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_PROGRESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	CProgressCtrl progressCtrl;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};
