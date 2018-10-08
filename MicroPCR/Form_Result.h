#pragma once
#include "Chart.h"


// CForm_Result 대화 상자입니다.

class CForm_Result : public CDialog
{
	DECLARE_DYNAMIC(CForm_Result)

public:
	CForm_Result(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CForm_Result();

	// 대화 상자 데이터입니다.
	enum { IDD = IDD_FORM_RESULT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnPaint();

	CXYChart m_Chart;

};
