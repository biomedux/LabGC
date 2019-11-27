// CDeviceSetup.cpp: 구현 파일
//

#include "stdafx.h"
#include "DeviceSetup.h"
#include "afxdialogex.h"
#include "resource.h"

// CDeviceSetup 대화 상자

IMPLEMENT_DYNAMIC(CDeviceSetup, CDialogEx)

CDeviceSetup::CDeviceSetup(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_DEVICE_SETUP, pParent)
{

}

CDeviceSetup::~CDeviceSetup()
{
}

void CDeviceSetup::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDeviceSetup, CDialogEx)
END_MESSAGE_MAP()


// CDeviceSetup 메시지 처리기
