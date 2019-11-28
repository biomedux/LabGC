// CDeviceSetup.cpp: 구현 파일
//

#include "stdafx.h"
#include "DeviceSetup.h"
#include "afxdialogex.h"
#include "resource.h"
#include "FileManager.h"

// CDeviceSetup 대화 상자

IMPLEMENT_DYNAMIC(CDeviceSetup, CDialogEx)

CDeviceSetup::CDeviceSetup(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_DEVICE_SETUP, pParent)
	, device(NULL)
	, magneto(NULL)
{
}

CDeviceSetup::~CDeviceSetup()
{
	if (device != NULL)
		delete device;
	if (magneto != NULL)
		delete magneto;
}

void CDeviceSetup::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_COMBO_MAGNETO_COMPORT, portList);
	DDX_Control(pDX, IDC_COMBO_RESET_DEVICES, deviceList);
}


BEGIN_MESSAGE_MAP(CDeviceSetup, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_SET_MAGNETO_SERIAL, &CDeviceSetup::OnBnClickedButtonSetMagnetoSerial)
	ON_BN_CLICKED(IDC_BUTTON_MAKE_HEX_FILE, &CDeviceSetup::OnBnClickedButtonMakeHexFile)
	ON_BN_CLICKED(IDC_BUTTON_RESET_DEVICE, &CDeviceSetup::OnBnClickedButtonResetDevice)
END_MESSAGE_MAP()


// CDeviceSetup 메시지 처리기

BOOL CDeviceSetup::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	((CEdit*)GetDlgItem(IDC_EDIT_MAGNETO_SERIAL_NUMBER))->SetLimitText(6);
	((CEdit*)GetDlgItem(IDC_EDIT_PCR_SERIAL_NUMBER))->SetLimitText(6);

	initConnection();
	initDeviceList();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CDeviceSetup::initConnection(){
	device = new CDeviceConnect(GetSafeHwnd());
	magneto = new CMagneto();
	magneto->setHwnd(GetSafeHwnd());
}

void CDeviceSetup::initDeviceList() {
	// Getting the portList
	portList.ResetContent();
	vector<CString> portData;
	magneto->searchPortByReg(portData);

	for (int i = 0; i < portData.size(); ++i) {
		portList.AddString(portData[i]);
	}

	if (portList.GetCount() != 0) {
		portList.SetCurSel(0);
	}

	deviceList.ResetContent();

	int deviceNums = device->GetDevices();
	for (int i = 0; i < deviceNums; ++i) {
		CString deviceSerial = device->GetDeviceSerial(i);

		// Remove the QuPCR string
		deviceList.AddString(deviceSerial.Mid(5));
	}

	if (deviceList.GetCount() != 0) {
		deviceList.SetCurSel(0);
	}
}

void CDeviceSetup::OnBnClickedButtonSetMagnetoSerial()
{
	CString serialNumber;
	GetDlgItemText(IDC_EDIT_MAGNETO_SERIAL_NUMBER, serialNumber);

	if (serialNumber.GetLength() != 6) {
		AfxMessageBox(L"시리얼 넘버를 6자리로 입력하세요.");
	}
	else {
		int targetSerialNumber = _ttoi(serialNumber);
		// Getting the device index
		int selectedIdx = portList.GetCurSel();

		CString tempPortNumber;
		portList.GetLBText(selectedIdx, tempPortNumber);
		tempPortNumber.Replace(L"COM", L"");
		int comPortNumber = _ttoi(tempPortNumber);

		DriverStatus::Enum res = magneto->connect(comPortNumber);

		if (res == DriverStatus::CONNECTED)
		{
			long currentSerialNumber = magneto->getSerialNumber();

			CString temp;
			temp.Format(L"현재 장비의 Serial number 는 ""%06d"" 입니다.\n""%06d"" 로 변경하시겠습니까?", currentSerialNumber, targetSerialNumber);
			AfxMessageBox(temp);

			bool res = magneto->setSerialNumber(targetSerialNumber);

			if (res) {
				AfxMessageBox(L"변경되었습니다.");
			}
			else {
				AfxMessageBox(L"변경에 실패하였습니다. 장비를 체크하세요.");
			}

			magneto->disconnect();
		}
		else {
			AfxMessageBox(L"Magneto 장비와 연결에 실패하였습니다. 상태를 체크하세요.");
		}
	}
}


#define QuPCR_TEXT	L"03510075005000430052"
#define HEADER_SIZE 9

CString CDeviceSetup::makeChecksum(CString lineData) {
	CString result, tmp;
	CString unicodeData = lineData.Mid(1);
	int unicodeVal, sum = 0;

	for (int i = 0; i < unicodeData.GetLength(); i += 2) {
		tmp = unicodeData.Mid(i, 2);
		// CString to int data
		sscanf((CStringA)tmp, "%x", &unicodeVal);
		sum += unicodeVal;
	}

	int checksum = sum % 0x100;
	checksum = 0x100 - checksum;

	result.Format(L"%02X", checksum);
	return result;
}

void CDeviceSetup::OnBnClickedButtonMakeHexFile()
{
	// Getting the input serial number
	CString serialNumber;
	GetDlgItemText(IDC_EDIT_PCR_SERIAL_NUMBER, serialNumber);

	if (serialNumber.GetLength() != 6) {
		AfxMessageBox(L"시리얼 넘버를 6자리로 입력하세요.");
	}
	else {
		// Setting the directory
		BROWSEINFO brInfo;
		TCHAR pathBuffer[512];
		CString title;

		title.Format(L"펌웨어 파일이 저장될 폴더를 선택하세요.\nFirmware[%s].hex 파일로 저장됩니다.", serialNumber);

		::ZeroMemory(&brInfo, sizeof(BROWSEINFO));
		::ZeroMemory(pathBuffer, 512);

		brInfo.hwndOwner = GetSafeHwnd();
		brInfo.lpszTitle = title;
		brInfo.ulFlags = BIF_NEWDIALOGSTYLE | BIF_EDITBOX | BIF_RETURNONLYFSDIRS;

		LPITEMIDLIST folderList = ::SHBrowseForFolder(&brInfo);
		bool res = ::SHGetPathFromIDList(folderList, pathBuffer);

		if (res) {
			CString savePath;
			CString targetSerialNumber = L"";

			savePath.Format(L"%s\\Firmware[%s].hex", pathBuffer, serialNumber);

			// Make the unicode type serial number
			for (int i = 0; i < serialNumber.GetLength(); ++i) {
				CString tmp;
				tmp.Format(L"%04x", serialNumber.GetAt(i));
				targetSerialNumber += tmp;
			}

			CString firmwareData;
			FileManager::loadFirmwareFile(firmwareData);

			// String to vector
			vector<CString> lines;
			CString token;
			CString slidingLine;

			int count = 0;
			while (AfxExtractSubString(token, firmwareData, count, L'\n')) {
				lines.push_back(token);
				count++;
			}

			CString targetText = QuPCR_TEXT;
			CString prev;

			for (int i = 0; i < lines.size(); ++i) {
				CString line = lines[i];

				if (line.Left(3).Compare(L":10") == 0) {
					slidingLine += line.Mid(9, 32);

					if (slidingLine.GetLength() > 32) {
						CString test;
						test.Format(L"%s - %d\n", slidingLine, slidingLine.Find(targetText));
						::OutputDebugString(test);
						// Check the QuPCR Text in the data
						if (slidingLine.Find(targetText) != -1) {
							// Find the Serial section and change to new serial number
							int idx = line.Mid(9, 32).Find(targetText.Right(4)) + 4;
							int leftCount = (32 - (idx)) / 4;

							CString newLine1 = line.Left(9 + idx);

							// all serial number exists in this line
							if (leftCount == 6) {
								newLine1 += targetSerialNumber;

								// Make the checksum
								newLine1 += makeChecksum(newLine1);

								// Change the line
								lines[i] = newLine1 + L"\r";
							}
							else {
								CString newLine2 = lines[i + 1].Left(9);

								newLine1 += targetSerialNumber.Left(leftCount * 4);
								newLine2 += targetSerialNumber.Mid(leftCount * 4);
								newLine2 += lines[i + 1].Mid(9 + ((6 - leftCount) * 4));

								// Remove checksum
								newLine2 = newLine2.Left(newLine2.GetLength() - 3);

								// Make check sum
								newLine1 += makeChecksum(newLine1);
								newLine2 += makeChecksum(newLine2);

								// Change the lines
								lines[i] = newLine1 + L"\r";
								lines[i + 1] = newLine2 + L"\r";
							}
						}

						// Remove previous data
						prev = slidingLine;
						slidingLine = slidingLine.Mid(32);
					}
				}

				lines[i] = lines[i].Left(lines[i].GetLength() - 1);
			}

			FileManager::saveFirmwareFile(savePath, lines);
		}
	}
}


void CDeviceSetup::OnBnClickedButtonResetDevice()
{
	// Getting the device index
	int selectedIdx = deviceList.GetCurSel();

	if (selectedIdx != -1) {
		// Connection with PCR
		BOOL res = device->OpenDevice(LS4550EK_VID, LS4550EK_PID, device->GetDeviceSerialForConnection(selectedIdx), TRUE);

		if (res) {
			RxBuffer rx;
			TxBuffer tx;

			memset(&rx, 0, sizeof(RxBuffer));
			memset(&tx, 0, sizeof(TxBuffer));

			tx.cmd = CMD_BOOTLOADER;

			BYTE senddata[65] = { 0, };
			BYTE readdata[65] = { 0, };
			memcpy(senddata, &tx, sizeof(TxBuffer));

			device->Write(senddata);

			device->Read(&rx);

			memcpy(readdata, &rx, sizeof(RxBuffer));

			Sleep(TIMER_DURATION);
		}
		else {
			AfxMessageBox(L"해당 장비와 연결에 실패하였습니다. 장비 상태를 확인하세요.");
		}
	}
}


BOOL CDeviceSetup::PreTranslateMessage(MSG* pMsg)
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
