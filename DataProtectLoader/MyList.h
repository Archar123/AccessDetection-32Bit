//�Զ����һ��List��

#pragma once


// CMyList

class CMyList : public CListCtrl
{
	DECLARE_DYNAMIC(CMyList)

public:
	CMyList();
	virtual ~CMyList();
	//�������ݣ�������������ɫ
	int InsertItem(int nItem,LPCTSTR lpText,COLORREF fontcolor=RGB(0,0,0));

	//���ͼ��
	int SetProcessIcon(CString strPath);

	void SetItemColor(int nItem,COLORREF fontcolor);

	LV_ITEM		m_Item;

	CImageList  g_SmallIcon;

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg	void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
};


