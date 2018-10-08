// Form_Result.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "MicroPCR.h"
#include "Form_Result.h"
#include "afxdialogex.h"


// CForm_Result 대화 상자입니다.

IMPLEMENT_DYNAMIC(CForm_Result, CDialog)

CForm_Result::CForm_Result(CWnd* pParent /*=NULL*/)
: CDialog(CForm_Result::IDD, pParent)
{

}

CForm_Result::~CForm_Result()
{
}

void CForm_Result::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CForm_Result, CDialog)
	ON_WM_PAINT()
//	ON_WM_TIMER()
END_MESSAGE_MAP()


// CForm_Result 메시지 처리기입니다.


void CForm_Result::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CDialog::OnPaint()을(를) 호출하지 마십시오.

	CRect graphRect;

	int oldMode = dc.SetMapMode(MM_LOMETRIC);

	graphRect.SetRect(0, 0, 562, 322);

	dc.DPtoLP((LPPOINT)&graphRect, 2);

	CDC *dc2 = CDC::FromHandle(dc.m_hDC);
	m_Chart.OnDraw(dc2, graphRect, graphRect);

	dc.SetMapMode(oldMode);




	CDialog::OnPaint();
}


BOOL CForm_Result::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
			return TRUE;

		if (pMsg->wParam == VK_ESCAPE)
			return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}
