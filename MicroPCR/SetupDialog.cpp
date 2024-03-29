// SetupDialog.cpp: 구현 파일
//

#include "stdafx.h"
#include "SetupDialog.h"
#include "afxdialogex.h"
#include "resource.h"
#include "ProtocolEditorDialog.h"
#include "FileManager.h"

// SetupDialog 대화 상자

IMPLEMENT_DYNAMIC(SetupDialog, CDialogEx)

SetupDialog::SetupDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_SETUP, pParent)
	, maxCycles(40)
	, compensation(0)
	, integralMax(INTGRALMAX)
	, displayDelta(0.0f)
	, flRelativeMax(FL_RELATIVE_MAX)
{

}

SetupDialog::~SetupDialog()
{
}

void SetupDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CUSTOM_PID_SETUP_TABLE, pidTable);
	DDX_Control(pDX, IDC_CUSTOM_TEST_HISTORY_TABLE, historyTable);
	DDX_Control(pDX, IDC_LIST_PROTOCOL, protocolList);

	DDX_Text(pDX, IDC_EDIT_MAX_CYCLES, maxCycles);
	DDV_MinMaxInt(pDX, maxCycles, 1, 80);
	DDX_Text(pDX, IDC_EDIT_COMPENSATION, compensation);
	DDV_MinMaxInt(pDX, compensation, 0, 200);
	DDX_Text(pDX, IDC_EDIT_INTEGRAL_MAX, integralMax);
	DDV_MinMaxFloat(pDX, integralMax, 0.0, 10000.0);
	DDX_Text(pDX, IDC_EDIT_DISPLAY_DELTA, displayDelta);
	DDV_MinMaxFloat(pDX, displayDelta, 0.0, 10000.0);
	DDX_Text(pDX, IDC_EDIT_FL_RELATIVE_MAX, flRelativeMax);
	DDV_MinMaxFloat(pDX, flRelativeMax, 0.0, 10000.0);
}


BEGIN_MESSAGE_MAP(SetupDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_CHANGES, &SetupDialog::OnBnClickedButtonSaveChanges)
	ON_BN_CLICKED(IDC_BUTTON_ADD_PROTOCOL, &SetupDialog::OnBnClickedButtonAddProtocol)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_PROTOCOL, &SetupDialog::OnBnClickedButtonEditProtocol)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_PROTOCOL, &SetupDialog::OnBnClickedButtonDeleteProtocol)
END_MESSAGE_MAP()


// SetupDialog 메시지 처리기

BOOL SetupDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	initConstants();
	initPidTable();
	loadProtocolList();
	initHistoryTable();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void SetupDialog::initConstants() {
	FileManager::loadConstants(maxCycles, compensation, integralMax, displayDelta, flRelativeMax, pids);

	UpdateData(FALSE);

	// if there is no pids, make initial pid data
	if (pids.empty()) {
		PID pid1(25.0f, 95.0f, 0.0f, 0.0f, 0.0f);
		PID pid2(95.0f, 60.0f, 0.0f, 0.0f, 0.0f);
		PID pid3(60.0f, 72.0f, 0.0f, 0.0f, 0.0f);
		PID pid4(72.0f, 95.0f, 0.0f, 0.0f, 0.0f);
		PID pid5(95.0f, 50.0f, 0.0f, 0.0f, 0.0f);
		pids.push_back(pid1);
		pids.push_back(pid2);
		pids.push_back(pid3);
		pids.push_back(pid4);
		pids.push_back(pid5);
	}
}

static const int PID_TABLE_COLUMN_WIDTHS[6] = { 88, 120, 120, 75, 75, 75 };

void SetupDialog::initPidTable() {
	pidTable.SetListMode(true);

	pidTable.DeleteAllItems();

	pidTable.SetSingleRowSelection();
	pidTable.SetSingleColSelection();
	pidTable.SetRowCount(6);
	pidTable.SetColumnCount(PID_CONSTANTS_MAX + 1);
	pidTable.SetFixedRowCount(1);
	pidTable.SetFixedColumnCount(1);
	pidTable.SetEditable(true);
	pidTable.SetColumnResize(false);

	// 초기 gridControl 의 table 값들을 설정해준다.
	DWORD dwTextStyle = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;

	for (int col = 0; col < pidTable.GetColumnCount(); col++) {
		GV_ITEM Item;
		Item.mask = GVIF_TEXT | GVIF_FORMAT;
		Item.row = 0;
		Item.col = col;

		if (col > 0) {
			Item.nFormat = DT_LEFT | DT_WORDBREAK;
			Item.strText = PID_TABLE_COLUMNS[col - 1];
		}

		pidTable.SetItem(&Item);
		pidTable.SetColumnWidth(col, PID_TABLE_COLUMN_WIDTHS[col]);
	}

	for (int i = 0; i < pids.size(); ++i) {
		float *temp[5] = { &(pids[i].startTemp), &(pids[i].targetTemp),
			&(pids[i].kp), &(pids[i].kd), &(pids[i].ki) };
		for (int j = 0; j < PID_CONSTANTS_MAX + 1; ++j) {
			GV_ITEM item;
			item.mask = GVIF_TEXT | GVIF_FORMAT;
			item.row = i + 1;
			item.col = j;
			item.nFormat = DT_LEFT | DT_WORDBREAK;

			// 첫번째 column 은 PID 1 으로 표시
			if (j == 0)
				item.strText.Format(L"PID #%d", i + 1);
			else
				item.strText.Format(L"%.4f", *temp[j - 1]);

			pidTable.SetItem(&item);
		}
	}
}

static const int HISTORY_TABLE_COLUMN_WIDTHS[6] = { 40, 130, 90, 90, 90, 90 };

void SetupDialog::initHistoryTable() {
	vector<History> historyList;
	FileManager::loadHistory(historyList);

	historyTable.SetListMode(true);

	historyTable.DeleteAllItems();

	historyTable.SetSingleRowSelection();
	historyTable.SetSingleColSelection();
	historyTable.SetRowCount(historyList.size() + 1);
	historyTable.SetColumnCount(6);
	historyTable.SetFixedRowCount(1);
	historyTable.SetFixedColumnCount(1);
	historyTable.SetEditable(false);
	historyTable.SetColumnResize(false);
	historyTable.SetRowResize(false);

	// 초기 gridControl 의 table 값들을 설정해준다.
	DWORD dwTextStyle = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;

	for (int col = 0; col < historyTable.GetColumnCount(); col++) {
		GV_ITEM Item;
		Item.mask = GVIF_TEXT | GVIF_FORMAT;
		Item.row = 0;
		Item.col = col;

		if (col > 0) {
			Item.nFormat = DT_LEFT | DT_WORDBREAK;
			Item.strText = HISTORY_TABLE_COLUMNS[col - 1];
		}

		historyTable.SetItem(&Item);
		historyTable.SetColumnWidth(col, HISTORY_TABLE_COLUMN_WIDTHS[col]);
	}

	for (int i = 0; i < historyList.size(); ++i) {
		CString *datas[5] = {&historyList[i].date, &historyList[i].target, &historyList[i].filter, &historyList[i].ctValue, &historyList[i].result};

		for (int j = 0; j < historyTable.GetColumnCount(); ++j) {
			GV_ITEM item;
			item.mask = GVIF_TEXT | GVIF_FORMAT;
			item.row = i + 1;
			item.col = j;
			item.nFormat = DT_LEFT | DT_WORDBREAK;

			// 첫번째 column 은 PID 1 으로 표시
			if (j == 0)
				item.strText.Format(L"#%d", i + 1);
			else
				item.strText.Format(L"%s", *datas[j-1]);

			historyTable.SetItem(&item);
		}
	}
}

void SetupDialog::loadProtocolList() {
	// Remove previous protocol list
	protocolList.ResetContent();

	FileManager::loadProtocols(protocols);

	for (int i = 0; i < protocols.size(); ++i) {
		protocolList.AddString(protocols[i].protocolName);
	}
}

void SetupDialog::OnBnClickedButtonSaveChanges()
{
	// Check the constant values
	BOOL res = UpdateData();

	if (res) {
		// Check the pid data
		for (int row = 1; row < pidTable.GetRowCount(); ++row) {
			CString startTemp = pidTable.GetItemText(row, 1);
			CString targetTemp = pidTable.GetItemText(row, 2);
			CString kp = pidTable.GetItemText(row, 3);
			CString kd = pidTable.GetItemText(row, 4);
			CString ki = pidTable.GetItemText(row, 5);

			if (startTemp.Compare(targetTemp) == 0) {
				AfxMessageBox(L"Start temp can't same the target temp.");
				return;
			}

			pids[row - 1].startTemp = _wtof(startTemp);
			pids[row - 1].targetTemp = _wtof(targetTemp);
			pids[row - 1].kp = _wtof(kp);
			pids[row - 1].ki = _wtof(ki);
			pids[row - 1].kd = _wtof(kd);
		}

		res = FileManager::saveConstants(maxCycles, compensation, integralMax, displayDelta, flRelativeMax, pids);
		if (res) {
			AfxMessageBox(L"Success!");
		}
		else {
			AfxMessageBox(L"Failed! Unknown error!");
		}
	}
	else {
		AfxMessageBox(L"Failed! Please input the valid value.");
	}
}


void SetupDialog::OnBnClickedButtonAddProtocol()
{
	CProtocolEditorDialog editorDialog;
	int res = editorDialog.DoModal();

	if (res == IDOK) {
		// Refresh the protocol list
		loadProtocolList();
	}
}


void SetupDialog::OnBnClickedButtonEditProtocol()
{
	// Check selected protocol is exist.
	int selectedIdx = protocolList.GetCurSel();

	if (selectedIdx == -1) {
		AfxMessageBox(L"Please select the protocol first.");
		return;
	}

	CProtocolEditorDialog editorDialog;
	editorDialog.setProtocol(protocols[selectedIdx]);
	int res = editorDialog.DoModal();

	if (res == IDOK) {
		// Refresh the protocol list
		loadProtocolList();
	}
}

void SetupDialog::OnBnClickedButtonDeleteProtocol()
{
	// Check selected protocol is exist.
	int selectedIdx = protocolList.GetCurSel();

	if (selectedIdx == -1) {
		AfxMessageBox(L"Please select the protocol first.");
		return;
	}

	CString protocolName = protocols[selectedIdx].protocolName;
	CString message;
	message.Format(L"%s 이 삭제되었습니다.", protocolName);

	protocols.erase(protocols.begin() + selectedIdx);

	// save protocol and reload
	FileManager::saveProtocols(protocols);
	
	loadProtocolList();

	AfxMessageBox(message);
}


BOOL SetupDialog::PreTranslateMessage(MSG* pMsg)
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
