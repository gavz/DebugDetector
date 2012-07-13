#include "DDMain.h"

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)
{
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAINFRAME),NULL,reinterpret_cast<DLGPROC>(MainDLGProc));
	return false;
}

LRESULT CALLBACK MainDLGProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	hwDlgMainFrame = hWndDlg;
	switch(Msg)
	{
	case WM_INITDIALOG:
		{
			LVCOLUMN LvCol;
			HWND hwPluginList = GetDlgItem(hwDlgMainFrame,IDC_PLUGINS);
			SendMessage(hwPluginList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

			memset(&LvCol,0,sizeof(LvCol));                  
			LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;                                     
			LvCol.pszText = L"Name";                         
			LvCol.cx = 0x100;                               
			SendMessage(hwPluginList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);
			LvCol.pszText = L"Version";
			LvCol.cx = 0x30;
			SendMessage(hwPluginList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);
			LvCol.pszText = L"Debugged"; 
			LvCol.cx = 0x40;
			SendMessage(hwPluginList,LVM_INSERTCOLUMN,2,(LPARAM)&LvCol);
			LvCol.pszText = L"ErrorMessage"; 
			LvCol.cx = 0x80;
			SendMessage(hwPluginList,LVM_INSERTCOLUMN,3,(LPARAM)&LvCol);

			if(!LoadPlugins())
			{
				MessageBox(hwDlgMainFrame,L"No Plugins found!",L"Debug Detector",MB_OK);
				EndDialog(hwDlgMainFrame,0);
			}
			else
			{
				ExecutePlugins();
				TCHAR* sTemp = (TCHAR*)malloc(255);
				swprintf(sTemp,L"DebugCheck: loaded %d Plugins! - %d of %d detections - ratio: %0.2f %%",
					vPluginList.size(),
					iDetectNum,
					vPluginList.size(),
					((iDetectNum* 1.0 / vPluginList.size() *  1.0)  * 100));

				SetWindowTextW(GetDlgItem(hwDlgMainFrame,IDC_STATE),sTemp);			
				free(sTemp);
			}
			return true;
		}
	case WM_CLOSE:
		{
			EndDialog(hwDlgMainFrame,0);
			return true;
		}
	}
	return false;
}

bool LoadPlugins()
{
	WIN32_FIND_DATA FindDataw32;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	TCHAR* szCurDir = (TCHAR*)malloc(MAX_PATH);
	GetCurrentDirectory(MAX_PATH,szCurDir);
	wcscat(szCurDir,L"\\*");

	hFind = FindFirstFile(szCurDir,&FindDataw32);

	if (INVALID_HANDLE_VALUE == hFind) 
		return false;

	do
	{
		if (!(FindDataw32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			if(wcsstr(FindDataw32.cFileName,L".dll") != NULL)
			{
				HMODULE hPlugin = LoadLibrary(FindDataw32.cFileName);

				if(hPlugin != NULL)
				{
					srcPlugin newPlugin;
					newPlugin.dwVersion = (DWORD)GetProcAddress(hPlugin,"PluginVersion");
					newPlugin.dwName = (DWORD)GetProcAddress(hPlugin,"PluginName");
					newPlugin.dwDebugCheck = (DWORD)GetProcAddress(hPlugin,"PluginDebugCheck");
					newPlugin.dwErrorMessage = (DWORD)GetProcAddress(hPlugin,"PluginErrorMessage");
					newPlugin.hPlugin = hPlugin;

					if(newPlugin.dwDebugCheck != NULL && newPlugin.dwName != NULL && newPlugin.dwVersion != NULL && newPlugin.dwErrorMessage != NULL)
						vPluginList.push_back(newPlugin);
					else
						FreeLibrary(hPlugin);
				}
			}
		}
	}
	while (FindNextFile(hFind,&FindDataw32) != 0);

	free(szCurDir);
	if(vPluginList.size() > 0)
		return true;
	else 
		return false;
}

bool ExecutePlugins()
{
	for(int i = 0; i < vPluginList.size(); i++)
	{
		PluginName newPluginName = (PluginName)vPluginList[i].dwName;
		PluginVersion newPluginVersion = (PluginVersion)vPluginList[i].dwVersion;
		PluginDebugCheck newPluginDebugCheck = (PluginDebugCheck)vPluginList[i].dwDebugCheck;
		PluginErrorMessage newPluginErrorMessage = (PluginErrorMessage)vPluginList[i].dwErrorMessage;

		LVITEM LvItem;
		TCHAR* sTemp = (TCHAR*)malloc(255);
		HWND hwPluginList = GetDlgItem(hwDlgMainFrame,IDC_PLUGINS);
		int itemIndex = SendMessage(hwPluginList,LVM_GETITEMCOUNT,0,0);

		memset(&LvItem,0,sizeof(LvItem));
		wsprintf(sTemp,L"%s",newPluginName());
		LvItem.mask = LVIF_TEXT;
		LvItem.cchTextMax = 255;
		LvItem.iItem = itemIndex;
		LvItem.iSubItem = 0;
		LvItem.pszText = sTemp;
		SendMessage(hwPluginList,LVM_INSERTITEM,0,(LPARAM)&LvItem);

		wsprintf(sTemp,L"%s",newPluginVersion());
		LvItem.iSubItem = 1;
		SendMessage(hwPluginList,LVM_SETITEM,0,(LPARAM)&LvItem);

		memset(sTemp,0,255);
		switch(newPluginDebugCheck())
		{
		case 0:
			wsprintf(sTemp,L"%s",L"FALSE");
			break;
		case 1:
			wsprintf(sTemp,L"%s",L"TRUE");	
			iDetectNum++;
			break;
		case -1:
			wsprintf(sTemp,L"%s",newPluginErrorMessage());
			LvItem.iSubItem = 3;
			SendMessage(hwPluginList,LVM_SETITEM,0,(LPARAM)&LvItem);
			break;
		}
				
		LvItem.iSubItem = 2;
		SendMessage(hwPluginList,LVM_SETITEM,0,(LPARAM)&LvItem);

		free(sTemp);
	}
	return true;
}