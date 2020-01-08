// CProgressDialog.cpp: 구현 파일
//

#include "stdafx.h"
#include "CProgressDialog.h"


// CProgressDialog 대화 상자

IMPLEMENT_DYNAMIC(CProgressDialog, CDialogEx)

CProgressDialog::CProgressDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_PROGRESS, pParent)
{
	Create(IDD_DIALOG_PROGRESS, NULL);
}

CProgressDialog::~CProgressDialog()
{
}

void CProgressDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS, progressCtrl);
}


BEGIN_MESSAGE_MAP(CProgressDialog, CDialogEx)
END_MESSAGE_MAP()


// CProgressDialog 메시지 처리기


BOOL CProgressDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	progressCtrl.SetMarquee(TRUE, 10);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}
