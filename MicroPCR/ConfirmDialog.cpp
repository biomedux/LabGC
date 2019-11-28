// ConfirmDialog.cpp: 구현 파일
//

#include "stdafx.h"
#include "ConfirmDialog.h"
#include "afxdialogex.h"
#include "resource.h"

// ConfirmDialog 대화 상자

IMPLEMENT_DYNAMIC(ConfirmDialog, CDialogEx)

ConfirmDialog::ConfirmDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CONFIRM, pParent)
	, message(L"")
{

}

ConfirmDialog::ConfirmDialog(CString message, CWnd* pParent) 
	: CDialogEx(IDD_DIALOG_CONFIRM, pParent)
	, message(message)
{
}

ConfirmDialog::~ConfirmDialog()
{
}

void ConfirmDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(ConfirmDialog, CDialogEx)
END_MESSAGE_MAP()


// ConfirmDialog 메시지 처리기


BOOL ConfirmDialog::PreTranslateMessage(MSG* pMsg)
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


BOOL ConfirmDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetDlgItemText(IDC_STATIC_CONFIRM_MESSAGE, message);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}
