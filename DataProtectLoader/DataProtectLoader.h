
// DataProtectLoader.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CDataProtectLoaderApp:
// �йش����ʵ�֣������ DataProtectLoader.cpp
//

class CDataProtectLoaderApp : public CWinApp
{
public:
	CDataProtectLoaderApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CDataProtectLoaderApp theApp;