//自定义的一个List类

#pragma once


// CMyList

class CMyList : public CListCtrl
{
	DECLARE_DYNAMIC(CMyList)

public:
	CMyList();
	virtual ~CMyList();
	//插入数据，可设置字体颜色
	int InsertItem(int nItem,LPCTSTR lpText,COLORREF fontcolor=RGB(0,0,0));

	//获得图标
	int SetProcessIcon(CString strPath);

	void SetItemColor(int nItem,COLORREF fontcolor);

	LV_ITEM		m_Item;

	CImageList  g_SmallIcon;

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg	void OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult);
};


