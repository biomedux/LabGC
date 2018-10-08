// ProtocolEditorDialog.cpp: 구현 파일
//

#include "stdafx.h"
#include "ProtocolEditorDialog.h"
#include "afxdialogex.h"
#include "resource.h"
#include "FileManager.h"

// CProtocolEditorDialog 대화 상자

IMPLEMENT_DYNAMIC(CProtocolEditorDialog, CDialogEx)

CProtocolEditorDialog::CProtocolEditorDialog(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_PROTOCOL_EDITOR, pParent)
	, protocolType(0)
	, currentLabel(0)
	, isEdit(false)
{
	// Add the default item
	Action action;
	action.Label = L"1";
	action.Temp = 95.0f;
	action.Time = 60.0f;
	currentProtocol.actionList.push_back(action);
}

CProtocolEditorDialog::~CProtocolEditorDialog()
{
}

void CProtocolEditorDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CUSTOM_PROTOCOL_EDITOR, protocolTable);
	DDX_Radio(pDX, IDC_RADIO_PROTOCOL_LABEL, (int &)protocolType);
}


BEGIN_MESSAGE_MAP(CProtocolEditorDialog, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_PROTOCOL_FILTER_FAM, &CProtocolEditorDialog::OnBnClickedButtonProtocolFilterFam)
	ON_BN_CLICKED(IDC_BUTTON_PROTOCOL_FILTER_HEX, &CProtocolEditorDialog::OnBnClickedButtonProtocolFilterHex)
	ON_BN_CLICKED(IDC_BUTTON_PROTOCOL_FILTER_ROX, &CProtocolEditorDialog::OnBnClickedButtonProtocolFilterRox)
	ON_BN_CLICKED(IDC_BUTTON_PROTOCOL_FILTER_CY5, &CProtocolEditorDialog::OnBnClickedButtonProtocolFilterCy5)
	ON_BN_CLICKED(IDC_BUTTON_PROTOCOL_SAVE, &CProtocolEditorDialog::OnBnClickedButtonProtocolSave)
	ON_BN_CLICKED(IDC_BUTTON_PROTOCOL_CANCEL, &CProtocolEditorDialog::OnBnClickedButtonProtocolCancel)
	ON_BN_CLICKED(IDC_BUTTON_ADD_PROTOCOL, &CProtocolEditorDialog::OnBnClickedButtonAddProtocol)
END_MESSAGE_MAP()


// CProtocolEditorDialog 메시지 처리기


BOOL CProtocolEditorDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Load bitmap
	offImg.LoadBitmapW(IDB_BITMAP_OFF);
	famImg.LoadBitmapW(IDB_BITMAP_FAM);
	hexImg.LoadBitmapW(IDB_BITMAP_HEX);
	roxImg.LoadBitmapW(IDB_BITMAP_ROX);
	cy5Img.LoadBitmapW(IDB_BITMAP_CY5);

	// Setting the filter image
	if (currentProtocol.useFam) {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_FAM, famImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_FAM, offImg);
	}

	if (currentProtocol.useHex) {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_HEX, hexImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_HEX, offImg);
	}

	if (currentProtocol.useRox) {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_ROX, roxImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_ROX, offImg);
	}

	if (currentProtocol.useCY5) {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_CY5, cy5Img);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_CY5, offImg);
	}

	// Protocol name setting
	SetDlgItemText(IDC_EDIT_PROTOCOL_NAME, currentProtocol.protocolName);

	// if the editing mode, disable protcol name edit field
	if (isEdit) {
		GetDlgItem(IDC_EDIT_PROTOCOL_NAME)->EnableWindow(FALSE);
	}
	else {
		GetDlgItem(IDC_EDIT_PROTOCOL_NAME)->EnableWindow(TRUE);
	}

	initProtocolTable();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CProtocolEditorDialog::setProtocol(Protocol& protocol) {
	currentProtocol = protocol;
	isEdit = true;
}

void CProtocolEditorDialog::OnBnClickedButtonProtocolFilterFam()
{
	currentProtocol.useFam = !currentProtocol.useFam;

	if (currentProtocol.useFam) {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_FAM, famImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_FAM, offImg);
	}
}


void CProtocolEditorDialog::OnBnClickedButtonProtocolFilterHex()
{
	currentProtocol.useHex = !currentProtocol.useHex;

	if (currentProtocol.useHex) {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_HEX, hexImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_HEX, offImg);
	}
}


void CProtocolEditorDialog::OnBnClickedButtonProtocolFilterRox()
{
	currentProtocol.useRox = !currentProtocol.useRox;

	if (currentProtocol.useRox) {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_ROX, roxImg);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_ROX, offImg);
	}
}


void CProtocolEditorDialog::OnBnClickedButtonProtocolFilterCy5()
{
	currentProtocol.useCY5 = !currentProtocol.useCY5;

	if (currentProtocol.useCY5) {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_CY5, cy5Img);
	}
	else {
		SET_BUTTON_IMAGE(IDC_BUTTON_PROTOCOL_FILTER_CY5, offImg);
	}
}


BOOL CProtocolEditorDialog::PreTranslateMessage(MSG* pMsg)
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


void CProtocolEditorDialog::OnBnClickedButtonProtocolSave()
{
	// Load saved protocols
	vector<Protocol> protocols;
	bool res = FileManager::loadProtocols(protocols);
	CString protocolName;
	GetDlgItemText(IDC_EDIT_PROTOCOL_NAME, protocolName);

	// don't check protocol name if the editing mode.
	if (!isEdit) {
		if (protocolName.IsEmpty()) {
			AfxMessageBox(L"Please input the protocol name.");
			return;
		}

		// When the saved data is exist, check the protocol name.
		if (res) {
			// Check protocol name
			for (int i = 0; i < protocols.size(); ++i) {
				if (protocolName.Compare(protocols[i].protocolName) == 0) {
					AfxMessageBox(L"This protocol name is already exist.");
					return;
				}
			}
		}
	}

	// Check the filter status, need to select 1 filter at least.
	if (!(currentProtocol.useFam || currentProtocol.useHex || currentProtocol.useRox || currentProtocol.useCY5)) {
		AfxMessageBox(L"Please select the filter at least 1.");
		return;
	}
	
	CString message;

	// Save new protocol
	// Load the grid data and apply the protocol class
	for (int row = 1; row < protocolTable.GetRowCount(); ++row) {
		CString label = protocolTable.GetItemText(row, 0);
		CString temp = protocolTable.GetItemText(row, 1);
		CString time = protocolTable.GetItemText(row, 2);
		
		if (label.Compare(L"GOTO") == 0) {
			// GOTO
			if (time.IsEmpty()) {
				message.Format(L"%d of row's loop data is empty.", row);
				AfxMessageBox(message);
				return;
			}
			else {
				int res = _ttoi(time);

				if (res == 0) {
					message.Format(L"%d of row's loop data is invalid.", row);
					AfxMessageBox(message);
					return;
				}
			}
		}
		else if (label.Compare(L"SHOT") == 0) {
			// No need to check
		}
		else {
			// Label
			if (temp.IsEmpty()) {
				message.Format(L"%d of row's temp data is empty.", row);
				AfxMessageBox(message);
				return;
			}
			else {
				int res = _ttoi(temp);

				if (!(res >= 4 && res <= 104)) {
					message.Format(L"%d of row's temp data is invalid(4~104).", row);
					AfxMessageBox(message);
					return;
				}
			}

			if (time.IsEmpty()) {
				message.Format(L"%d of row's time data is empty.", row);
				AfxMessageBox(message);
				return;
			}
			else {
				int res = _ttoi(time);

				// _ttoi function return the 0 value if the string is not number.
				if (time.Compare(L"0") != 0) {
					if (!(res >= 0 && res <= 3000)) {
						message.Format(L"%d of row's time data is invalid(1~3000).", row);
						AfxMessageBox(message);
						return;
					}
				}
			}
		}

		// Update new data
		currentProtocol.actionList[row - 1].Temp = _ttof(temp);
		currentProtocol.actionList[row - 1].Time = _ttof(time);
	}

	// Save protocol into file
	currentProtocol.protocolName = protocolName;
	if (isEdit) {
		// Change the protocol by new data
		int idx = -1;
		for (int i = 0; i < protocols.size(); ++i) {
			if (protocolName.Compare(protocols[i].protocolName) == 0) {
				idx = i;
				break;
			}
		}

		if (idx == -1) {
			AfxMessageBox(L"Failed! can't find previous protocol name.");
			return;
		}

		protocols[idx] = currentProtocol;
	}
	else {
		protocols.push_back(currentProtocol);
	}
	res = FileManager::saveProtocols(protocols);

	if (res) {
		AfxMessageBox(L"Success!");
		OnOK();
	}
	else {
		AfxMessageBox(L"Failed! Unknown error!");
	}
}


void CProtocolEditorDialog::OnBnClickedButtonProtocolCancel()
{
	OnCancel();
}


void CProtocolEditorDialog::OnBnClickedButtonAddProtocol()
{
	UpdateData();

	// Check previous table data
	int currentRowCount = protocolTable.GetRowCount();
	
	CString label, temp = L"95", time = L"60";

	// Label type, just adding the label
	if (protocolType == 0) {
		currentLabel++;
		label.Format(L"%d", currentLabel);
	}
	else if (protocolType == 1) {	// GOTO
		int targetLabel = protocolTable.GetFocusCell().row;

		if (targetLabel == -1) {
			AfxMessageBox(L"Please select target label.");
			return;
		}
		
		// Check the valid target
		CString selectedLabel = protocolTable.GetItemText(protocolTable.GetFocusCell().row, 0);

		if (selectedLabel.Compare(L"GOTO") == 0 || selectedLabel.Compare(L"SHOT") == 0) {
			AfxMessageBox(L"Please select the number label.");
			return;
		}

		// Check the already exist GOTO label
		for (int i = protocolTable.GetFocusCell().row; i < currentRowCount; ++i) {
			CString checkLabel = protocolTable.GetItemText(i, 0);

			if (checkLabel.Compare(L"GOTO") == 0) {
				AfxMessageBox(L"Duplicated GOTO state. Please select the correct label.");
				return;
			}
		}

		label = L"GOTO";
		temp = selectedLabel;
		time = L"5";	// Default value
	}
	else if (protocolType == 2) {	// SHOT
		// Check the already has the shot
		for (int i = 0; i<currentProtocol.actionList.size(); ++i) {
			if (currentProtocol.actionList[i].Label.Compare(L"SHOT") == 0) {
				AfxMessageBox(L"SHOT is already exist.");
				return;
			}
		}

		label = L"SHOT";
		temp = L"";
		time = L"";
	}

	// Add the protocol
	Action action;
	action.Label = label;
	action.Temp = _ttoi(temp);
	action.Time = _ttoi(time);

	currentProtocol.actionList.push_back(action);

	// Add the new grid data
	protocolTable.SetRowCount(currentRowCount + 1);

	for (int j = 0; j < 3; ++j) {
		GV_ITEM item;
		item.mask = GVIF_TEXT | GVIF_FORMAT;
		item.row = currentRowCount;
		item.col = j;
		item.nFormat = DT_LEFT | DT_WORDBREAK;

		if (j == 0)
			item.strText.Format(label);
		else if (j == 1) 
			item.strText.Format(temp);
		
		else if (j == 2) 
			item.strText.Format(time);
		

		protocolTable.SetItem(&item);
	}

	// Add the read only option for GOTO SHOT
	if (label.Compare(L"GOTO") == 0) {
		protocolTable.SetItemState(currentRowCount, 1, protocolTable.GetItemState(currentRowCount, 1) | GVIS_READONLY);
	}
	else if (label.Compare(L"SHOT") == 0) {
		protocolTable.SetItemState(currentRowCount, 1, protocolTable.GetItemState(currentRowCount, 1) | GVIS_READONLY);
		protocolTable.SetItemState(currentRowCount, 2, protocolTable.GetItemState(currentRowCount, 2) | GVIS_READONLY);
	}
}

static const int PROTOCOL_TABLE_COLUMN_WIDTHS[3] = { 133, 133, 132 };

void CProtocolEditorDialog::initProtocolTable() {
	protocolTable.SetListMode(true);

	protocolTable.DeleteAllItems();

	protocolTable.SetSingleRowSelection();
	protocolTable.SetSingleColSelection();
	protocolTable.SetColumnCount(3);
	protocolTable.SetFixedRowCount(1);
	protocolTable.SetFixedColumnCount(1);
	protocolTable.SetEditable(true);
	protocolTable.SetColumnResize(false);

	// 초기 gridControl 의 table 값들을 설정해준다.
	DWORD dwTextStyle = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;

	for (int col = 0; col < protocolTable.GetColumnCount(); col++) {
		GV_ITEM item;
		item.mask = GVIF_TEXT | GVIF_FORMAT;
		item.row = 0;
		item.col = col;

		if (col >= 0) {
			item.nFormat = DT_LEFT | DT_WORDBREAK;
			item.strText = PROTOCOL_TABLE_COLUMNS[col];
		}

		protocolTable.SetItem(&item);
		protocolTable.SetColumnWidth(col, PROTOCOL_TABLE_COLUMN_WIDTHS[col]);
	}

	protocolTable.SetRowCount(currentProtocol.actionList.size()+1);

	// Add the default item if there is no item exist.
	for (int i = 0; i < currentProtocol.actionList.size(); ++i) {
		for (int j = 0; j < 3; ++j) {
			GV_ITEM item;
			item.mask = GVIF_TEXT | GVIF_FORMAT;
			item.row = i + 1;
			item.col = j;
			item.nFormat = DT_LEFT | DT_WORDBREAK;

			if (j == 0) {
				// if the label is number
				if (!(currentProtocol.actionList[i].Label.Compare(L"GOTO") == 0 || currentProtocol.actionList[i].Label.Compare(L"SHOT") == 0)) {
					currentLabel = _ttoi(currentProtocol.actionList[i].Label);
				}
				item.strText.Format(currentProtocol.actionList[i].Label);
			}
			else if (j == 1) {
				if (currentProtocol.actionList[i].Label.Compare(L"SHOT") == 0) {
					item.strText = L"";
				}
				else {
					item.strText.Format(L"%.f", currentProtocol.actionList[i].Temp);
				}
			}
			else if (j == 2) {
				if (currentProtocol.actionList[i].Label.Compare(L"SHOT") == 0) {
					item.strText = L"";
				}
				else {
					item.strText.Format(L"%.f", currentProtocol.actionList[i].Time);
				}
			}

			protocolTable.SetItem(&item);
		}
	}
}