
#include "stdafx.h"
#include "ProgressThread.h"
#include "CProgressDialog.h"

IMPLEMENT_DYNCREATE(CProgressThread, CWinThread)

BOOL CProgressThread::InitInstance()
{
	m_pMainWnd = new CProgressDialog;
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CProgressThread::ExitInstance()
{
	::OutputDebugString(L"test\n");
	delete m_pMainWnd;

	return CWinThread::ExitInstance();
}

void CProgressThread::setTitle(CString title) {
	m_pMainWnd->SetWindowTextW(title);
}

void CProgressThread::closeProgress() {
	m_pMainWnd->ShowWindow(SW_HIDE);
	m_pMainWnd->UpdateWindow();
}