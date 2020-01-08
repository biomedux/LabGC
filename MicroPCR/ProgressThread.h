#pragma once


class CProgressThread : public CWinThread
{
	DECLARE_DYNCREATE(CProgressThread)

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void setTitle(CString title);
	void closeProgress();
};