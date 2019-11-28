#pragma once


// ConfirmDialog 대화 상자

class ConfirmDialog : public CDialogEx
{
	DECLARE_DYNAMIC(ConfirmDialog)

public:
	ConfirmDialog(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	ConfirmDialog(CString message, CWnd* pParent = nullptr);
	virtual ~ConfirmDialog();

	CString message;

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CONFIRM };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()

	virtual BOOL PreTranslateMessage(MSG* pMsg);
public:
	virtual BOOL OnInitDialog();
};
