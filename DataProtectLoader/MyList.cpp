// MyList.cpp : 实现文件
//

#include "stdafx.h"
#include "DataProtectLoader.h"
#include "MyList.h"


// CMyList

IMPLEMENT_DYNAMIC(CMyList, CListCtrl)

CMyList::CMyList()
{
}

CMyList::~CMyList()
{
}

CMap<DWORD_PTR , DWORD_PTR& , COLORREF , COLORREF&> MapItemColor;
int CMyList::InsertItem(int nItem,LPCTSTR lpText,COLORREF fontcolor)
{
	/*
	memset((char*)&m_Item,0,sizeof(LV_ITEM));

	m_Item.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_STATE;
	m_Item.iItem = nItem;
	m_Item.iSubItem = 0;
	m_Item.stateMask = 0;
	m_Item.iImage = nImage;
	//strcpy((char*)(m_Item.pszText),(char*)(lpText));
	const int IDX = CListCtrl::InsertItem(&m_Item);
	*/
	const int IDX = CListCtrl::InsertItem(nItem, lpText);

	
/*
	//改变颜色
	DWORD_PTR iItem=(DWORD)nItem;
	MapItemColor.SetAt(iItem, fontcolor);
	this->RedrawItems(iItem,iItem);
	this->SetFocus();    //设置焦点
	UpdateWindow();
	*/
	return IDX;
}

void 
CMyList::SetItemColor(int nItem,COLORREF fontcolor)
{
	//改变颜色
	DWORD_PTR iItem=(DWORD)nItem;
	MapItemColor.SetAt(iItem, fontcolor);
	this->RedrawItems(iItem,iItem);
	this->SetFocus();    //设置焦点
	UpdateWindow();
}




BEGIN_MESSAGE_MAP(CMyList, CListCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdraw)
END_MESSAGE_MAP()



// CMyList 消息处理程序
void CMyList::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	
	*pResult = CDRF_DODEFAULT;
	NMLVCUSTOMDRAW * lplvdr=(NMLVCUSTOMDRAW*)pNMHDR;
	NMCUSTOMDRAW &nmcd = lplvdr->nmcd;

	switch(lplvdr->nmcd.dwDrawStage)//判断状态
	{
	case CDDS_PREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;
		}
	case CDDS_ITEMPREPAINT://如果为画ITEM之前就要进行颜色的改变
		{
			COLORREF ItemColor;
			if(MapItemColor.Lookup(nmcd.dwItemSpec, ItemColor))
				// 根据在 SetItemColor(DWORD iItem, COLORREF color) 设置的
				// ITEM号和COLORREF 在摸板中查找，然后进行颜色赋值。
			{
				lplvdr->clrText = RGB(0,0,0);//ItemColor;
				lplvdr->clrTextBk = ItemColor;
				*pResult = CDRF_DODEFAULT;
			}
		}
		break;
	}


}


int CMyList::SetProcessIcon(CString strPath)
{
	if (!PathFileExists(strPath))
		return 0;

	SHFILEINFO shInfo;
	memset(&shInfo, 0, sizeof(shInfo));
	SHGetFileInfo(strPath, FILE_ATTRIBUTE_NORMAL, &shInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES|SHGFI_SMALLICON);

	CMyList::g_SmallIcon.Add(shInfo.hIcon);
	return	(shInfo.iIcon);
}

