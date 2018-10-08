// Form_Main.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "MicroPCR.h"
#include "Form_Main.h"
#include "ConvertTool.h"
#include "afxdialogex.h"
#include "FileManager.h"
#include "TempGraphDlg.h"
#include "MicroPCRDlg.h"



// CForm_Main 대화 상자입니다.

IMPLEMENT_DYNAMIC(CForm_Main, CDialog)

CForm_Main::CForm_Main(CWnd* pParent /*=NULL*/)
	: CDialog(CForm_Main::IDD, pParent)
	, isConnected(false)
	, m_nLeftSec(0)
	, m_nLeftTotalSec(0)
	, m_totalActionNumber(0)
	, pcr_progressbar_max(0)
{
	memset(filters, 0, 4);
}

CForm_Main::~CForm_Main()
{
	if (actions != NULL)
		delete[]actions;
}

void CForm_Main::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PCR_PROTOCOL, m_cProtocolList);
}


BEGIN_MESSAGE_MAP(CForm_Main, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_FAM, &CForm_Main::OnBnClickedButtonFam)
	ON_BN_CLICKED(IDC_BUTTON_CY5, &CForm_Main::OnBnClickedButtonCy5)
	ON_BN_CLICKED(IDC_BUTTON_HEX, &CForm_Main::OnBnClickedButtonHex)
	ON_BN_CLICKED(IDC_BUTTON_ROX, &CForm_Main::OnBnClickedButtonRox)
	ON_BN_CLICKED(IDC_BUTTON_PCR_OPEN, &CForm_Main::OnBnClickedButtonPcrOpen)
	ON_STN_CLICKED(IDC_STATIC_PROGRESSBAR4, &CForm_Main::OnStnClickedStaticProgressbar4)
	ON_BN_CLICKED(IDC_BUTTON_START_PCR, &CForm_Main::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BTN_FLTTEST, &CForm_Main::OnBnClickedBtnFlttest)
END_MESSAGE_MAP()


// CForm_Main 메시지 처리기입니다.


BOOL CForm_Main::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	SetDlgItemText(IDC_EDIT_PCR_STATUS, L"Disconnected");
	SetDlgItemText(IDC_EDIT_MAGNETO_STATUS, L"Disconnected");
	SetDlgItemText(IDC_EDIT_ELAPSED_TIME, L"00:00:00");
	SetDlgItemText(IDC_EDIT_PHOTODIODE, L"0");
	SetDlgItemText(IDC_EDIT_CHAMBER_TEMP, L"0");


	actions = new Action[pDlg->m_cMaxActions];

	CFont font;
	CRect rect;
	CString labels[3] = { L"No", L"Temp.", L"Time" };

	font.CreatePointFont(100, L"Arial", NULL);

	m_cProtocolList.SetFont(&font);
	m_cProtocolList.GetClientRect(&rect);

	for (int i = 0; i<3; ++i)
		m_cProtocolList.InsertColumn(i, labels[i], LVCFMT_CENTER, (rect.Width() / 3));

	offImg.LoadBitmapW(IDB_BITMAP_OFF);
	famImg.LoadBitmapW(IDB_BITMAP_FAM);
	hexImg.LoadBitmapW(IDB_BITMAP_HEX);
	roxImg.LoadBitmapW(IDB_BITMAP_ROX);
	cy5Img.LoadBitmapW(IDB_BITMAP_CY5);
	((CButton*)GetDlgItem(IDC_BUTTON_FAM))->SetBitmap((HBITMAP)offImg);
	((CButton*)GetDlgItem(IDC_BUTTON_HEX))->SetBitmap((HBITMAP)offImg);
	((CButton*)GetDlgItem(IDC_BUTTON_ROX))->SetBitmap((HBITMAP)offImg);
	((CButton*)GetDlgItem(IDC_BUTTON_CY5))->SetBitmap((HBITMAP)offImg);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CForm_Main::OnBnClickedButtonFam()
{
	if (filters[0] == false)
	{
		SET_BUTTON_IMAGE(IDC_BUTTON_FAM, famImg);
		filters[0] = true;
	}
	else
	{
		SET_BUTTON_IMAGE(IDC_BUTTON_FAM, offImg);
		filters[0] = false;
	}
}

void CForm_Main::OnBnClickedButtonHex()
{
	if (filters[1] == false)
	{
		SET_BUTTON_IMAGE(IDC_BUTTON_HEX, hexImg);
		filters[1] = true;
	}
	else
	{
		SET_BUTTON_IMAGE(IDC_BUTTON_HEX, offImg);
		filters[1] = false;
	}
}

void CForm_Main::OnBnClickedButtonRox()
{
	if (filters[2] == false)
	{
		SET_BUTTON_IMAGE(IDC_BUTTON_ROX, roxImg);
		filters[2] = true;
	}
	else
	{
		SET_BUTTON_IMAGE(IDC_BUTTON_ROX, offImg);
		filters[2] = false;
	}
}

void CForm_Main::OnBnClickedButtonCy5()
{
	if (filters[3] == false)
	{
		SET_BUTTON_IMAGE(IDC_BUTTON_CY5, cy5Img);
		filters[3] = true;
	}
	else
	{
		SET_BUTTON_IMAGE(IDC_BUTTON_CY5, offImg);
		filters[3] = false;
	}
}




void CForm_Main::OnBnClickedButtonPcrOpen()
{
	if (!isConnected)
		return;

	m_nLeftTotalSec = 0;

	CFileDialog fdlg(TRUE, NULL, NULL, NULL, L"*.txt |*.txt|");
	if (fdlg.DoModal() == IDOK)
	{
		FileManager::saveRecentPath(FileManager::PROTOCOL_PATH, fdlg.GetPathName());

		m_sProtocolName = getProtocolName(fdlg.GetPathName());

		readProtocol(fdlg.GetPathName());
	}
}


void CForm_Main::readProtocol(CString path)
{
	CFile file;
	if (path.IsEmpty() || !file.Open(path, CFile::modeRead))
	{
		AfxMessageBox(L"No Recent Protocol File! Please Read Protocol!");
		return;
	}

	int fileSize = (int)file.GetLength() + 1;
	char *inTemp = new char[fileSize * sizeof(char)];
	file.Read(inTemp, fileSize);

	CString inString = AfxCharToString(inTemp);

	m_sProtocolName = getProtocolName(path);

	if (m_sProtocolName.GetLength() > MAX_PROTOCOL_LENGTH)
	{
		AfxMessageBox(L"Protocol Name is Too Long !");
		return;
	}

	if (inTemp) delete[] inTemp;

	int markPos = inString.Find(L"%PCR%");
	if (markPos < 0)
	{
		AfxMessageBox(L"This is not PCR File !!");
		return;
	}
	markPos = inString.Find(L"%END");
	if (markPos < 0)
	{
		AfxMessageBox(L"This is not PCR File !!");
		return;
	}

	int line = 0;
	for (int i = 0; i<markPos; i++)
	{
		if (inString.GetAt(i) == '\n')
			line++;
	}

	m_totalActionNumber = 0;
	for (int i = 0; i<line; i++)
	{
		int linePos = inString.FindOneOf(L"\n\r");			//  inString : PCR 프로토콜 전체 텍스트
		CString oneLine = (CString)inString.Left(linePos);	// PCR 프로토콜 한 줄
		inString.Delete(0, linePos + 2);					// action 구조체에 텍스트 데이터가 들어가면 텍스트 데이터의 한 줄 삭제
		
		if (i > 0)	// 첫 라인 무시?
		{
			int spPos = oneLine.FindOneOf(L" \t");
			
			// Label Extraction
			if (oneLine.CompareNoCase(L"SHOT") != 0)
			{
				actions[m_totalActionNumber].Label = oneLine.Left(spPos);
				oneLine.Delete(0, spPos);
				oneLine.TrimLeft();
			}
			else
			{
				actions[m_totalActionNumber].Label = "SHOT";
			}
			
			// Temperature Extraction
			bool timeflag = false;
			if (oneLine.CompareNoCase(L"SHOT") != 0)
			{
				spPos = oneLine.FindOneOf(L" \t");
				CString tmpStr = oneLine.Left(spPos);
				oneLine.Delete(0, spPos);
				oneLine.TrimLeft();

				actions[m_totalActionNumber].Temp = (double)_wtof(tmpStr);

				wchar_t tempChar = NULL;

				for (int j = 0; j < oneLine.GetLength(); j++)
				{
					tempChar = oneLine.GetAt(j);
					if (tempChar == (wchar_t)'m')
						timeflag = true;
					else if (tempChar == (wchar_t)'M')
						timeflag = true;
					else
						timeflag = false;
				}
			}
			else
			{
				actions[m_totalActionNumber].Temp = .0;
			}

			// Duration Extraction
			if (oneLine.CompareNoCase(L"SHOT") != 0)
			{
				double time = (double)_wtof(oneLine);

				if (timeflag)
					actions[m_totalActionNumber].Time = time * 60;
				else
					actions[m_totalActionNumber].Time = time;

				timeflag = false;
			}
			else
			{
				actions[m_totalActionNumber].Time = .0;
			}

			if (actions[m_totalActionNumber].Label != "GOTO")
			{
				if (oneLine.CompareNoCase(L"SHOT") != 0)
					m_nLeftTotalSec += (int)(actions[m_totalActionNumber].Time);

				pcr_progressbar_max++;
			}

			int label = 0;
			CString temp;
			if (actions[m_totalActionNumber].Label == "GOTO")
			{
				pcr_progressbar_max++;
				while (true && actions[m_totalActionNumber].Temp != 0 && actions[m_totalActionNumber].Temp < 101)
				{
					pcr_progressbar_max++;
					temp.Format(L"%.0f", actions[m_totalActionNumber].Temp);
					if (actions[label++].Label == temp) break;
				}

				for (int j = 0; j<actions[m_totalActionNumber].Time; j++)
				{
					for (int k = label - 1; k<m_totalActionNumber; k++)
					{
						m_nLeftTotalSec += (int)(actions[k].Time);
						pcr_progressbar_max++;
					}
				}
			}
			m_totalActionNumber++;
		}
	}

	int labelNo = 1;

	for (int i = 0; i<m_totalActionNumber; i++)
	{


		if (actions[i].Label != "GOTO")
		{
			if (actions[i].Label != "SHOT")
			{
				if (_ttoi(actions[i].Label) != labelNo)
				{
					AfxMessageBox(L"Label numbering error");
					return;
				}
				else
					labelNo++;

				if ((actions[i].Temp > 100) || (actions[i].Temp < 0))
				{
					AfxMessageBox(L"Target Temperature error!!");
					return;
				}

				if ((actions[i].Time > 3600) || (actions[i].Time < 0))
				{
					AfxMessageBox(L"Target Duration error!!");
					return;
				}
			}
		}
		else
		{
			/*
			TCHAR szTemp[256];
			_stprintf(szTemp, _T("%d\n"), (double)_wtof(actions[i - 1].Label));
			OutputDebugString(szTemp);
			*/
			if (actions[i-1].Label == "SHOT")
			{
				if ((actions[i].Temp > (double)_wtof(actions[i - 2].Label)) ||
					(actions[i].Temp < 1))
				{
					AfxMessageBox(L"GOTO Label error !!");
					return;
				}

				if ((actions[i].Time > 100) || (actions[i].Time < 1))
				{
					AfxMessageBox(L"GOTO repeat count error !!");
					return;
				}
			}
			else if (actions[i-1].Label != "SHOT")
			{ 
				if ((actions[i].Temp >(double)_wtof(actions[i - 1].Label)) ||
					(actions[i].Temp < 1))
				{
					AfxMessageBox(L"GOTO Label error !!");
					return;
				}

				if ((actions[i].Time > 100) || (actions[i].Time < 1))
				{
					AfxMessageBox(L"GOTO repeat count error !!");
					return;
				}
			}
		}
	}

	m_nLeftTotalSecBackup = m_nLeftTotalSec;

	displayList();	// 리스트에 표시해준다.
}

void CForm_Main::displayList()
{
	int j = 0;

	m_cProtocolList.DeleteAllItems();

	for (int i = 0; i<m_totalActionNumber; i++)
	{
		m_cProtocolList.InsertItem(i, actions[i].Label);		// 라벨 값을 리스트에 표시한다.
		CString tempString;

		if (actions[i].Label == "GOTO")		// GOTO 일 경우
		{
			tempString.Format(L"%d", (int)actions[i].Temp);
			m_cProtocolList.SetItemText(i, 1, tempString);

			tempString.Format(L"%d", (int)actions[i].Time);
			m_cProtocolList.SetItemText(i, 2, tempString);
		}
	
		else if (actions[i].Label == "SHOT")		// SHOT 일 경우
		{
			// 무시
		}
	
		else	// 숫자 일 경우
		{
			tempString.Format(L"%d", (int)actions[i].Temp);
			m_cProtocolList.SetItemText(i, 1, tempString);			// 온도를 리스트에 표시한다.

			// 시간 받기(분단위)
			int durs = (int)actions[i].Time;
			// 소수점 값을 초로 환산
			int durm = durs / 60;
			durs = durs % 60;

			if (durs == 0)
			{
				if (durm == 0) tempString.Format(L"∞");
				else tempString.Format(L"%dm", durm);
			}
			else
			{
				if (durm == 0) tempString.Format(L"%ds", durs);
				else tempString.Format(L"%dm %ds", durm, durs);
			}

			m_cProtocolList.SetItemText(i, 2, tempString);		// 시간을 리스트에 표현한다.
		}

		CString values;
		values.Format(L"'%s', %d, %d", actions[i].Label, (int)actions[i].Temp, (int)actions[i].Time);
		pDlg->insertFieldValue(_T("PCR_PROTOCOL"), _T("pcr_protoc_no, pcr_protoc_temper, pcr_protoc_time"), values);
	}

	// 프로토콜 이름
	// InvalidateRect(&CRect(10, 5, 10 * m_sProtocolName.GetLength() + MAX_PROTOCOL_LENGTH, 34));		// Protocol Name 갱신

	SetDlgItemText(IDC_EDIT_PCR_PROTC_NAME, m_sProtocolName);

	// 전체 시간을 시, 분, 초 단위로 계산
	int second = m_nLeftTotalSec % 60;
	int minute = m_nLeftTotalSec / 60;
	int hour = minute / 60;
	minute = minute - hour * 60;

	// 전체 시간 표시
	CString leftTime;
	leftTime.Format(L"%02d:%02d:%02d", hour, minute, second);
	SetDlgItemText(IDC_EDIT_LEFT_PROTOCOL_TIME, leftTime);
}

CString CForm_Main::getProtocolName(CString path)
{
	int pos = 0;
	CString protocol = path;

	for (int i = 0; i<protocol.GetLength(); i++)
	{
		if (protocol.GetAt(i) == '\\')
			pos = i;
	}

	return protocol.Mid(pos + 1, protocol.GetLength());
}


void CForm_Main::enableWindows()
{
	GetDlgItem(IDC_BUTTON_PCR_OPEN)->EnableWindow(isConnected);
}


void CForm_Main::OnStnClickedStaticProgressbar4()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


void CForm_Main::OnBnClickedButton1()
{
	pDlg->OnBnClickedButtonStart();
}


BOOL CForm_Main::PreTranslateMessage(MSG* pMsg)
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

void CForm_Main::OnBnClickedBtnFlttest()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}
