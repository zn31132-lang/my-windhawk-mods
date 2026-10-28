// SPDX-License-Identifier: MIT
// Self-contained reproduction harness for the accompanying local test report.
#include <windows.h>
#include <dwmapi.h>
#include <cstdio>
#include <thread>
#include <atomic>
#include <cstring>
std::atomic<bool> done{false};
void logs(){
 HANDLE ready=CreateEventW(0,FALSE,FALSE,L"DBWIN_BUFFER_READY");
 HANDLE data=CreateEventW(0,FALSE,FALSE,L"DBWIN_DATA_READY");
 HANDLE map=CreateFileMappingW(INVALID_HANDLE_VALUE,0,PAGE_READWRITE,0,4096,L"DBWIN_BUFFER");
 char* p=(char*)MapViewOfFile(map,FILE_MAP_READ,0,0,4096);
 FILE* f=fopen("probe.log","w");
 if(!p||!f)return;
 while(!done){SetEvent(ready);if(WaitForSingleObject(data,100)==WAIT_OBJECT_0){
 char s[4093]={};memcpy(s,p+4,4091);
 if(strstr(s,"windows-animations")){fprintf(f,"pid=%lu %s\n",*(DWORD*)p,s);fflush(f);}
 }} fclose(f);UnmapViewOfFile(p);CloseHandle(map);CloseHandle(data);CloseHandle(ready);
}
LRESULT CALLBACK proc(HWND w,UINT m,WPARAM a,LPARAM b){
 static int step=0;
 if(m==WM_TIMER){++step;if(step<=20)ShowWindow(w,step%2?SW_MINIMIZE:SW_RESTORE);else DestroyWindow(w);return 0;}
 if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT r;GetClientRect(w,&r);FillRect(dc,&r,(HBRUSH)(COLOR_WINDOW+1));DrawTextW(dc,L"Windows Animations performance test — temporary test window",-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);EndPaint(w,&ps);return 0;}
 if(m==WM_DESTROY){PostQuitMessage(0);return 0;}
 return DefWindowProcW(w,m,a,b);
}
int main(int argc, char** argv){
 SetProcessDPIAware();
 DEVMODEW dm{};dm.dmSize=sizeof(dm);EnumDisplaySettingsW(0,ENUM_CURRENT_SETTINGS,&dm);
 DWM_TIMING_INFO ti{};ti.cbSize=sizeof(ti);DwmGetCompositionTimingInfo(0,&ti);
 printf("monitor=%lux%lu refresh=%lu DWM=%u/%u\n",dm.dmPelsWidth,dm.dmPelsHeight,dm.dmDisplayFrequency,ti.rateRefresh.uiNumerator,ti.rateRefresh.uiDenominator);fflush(stdout);
 std::thread t(logs);Sleep(300);
 WNDCLASSW wc{};wc.lpfnWndProc=proc;wc.hInstance=GetModuleHandleW(0);wc.lpszClassName=L"WindhawkPerfProbe";wc.hCursor=LoadCursorW(0,IDC_ARROW);RegisterClassW(&wc);
 const bool large=argc>1 && strcmp(argv[1],"large")==0;
 HWND w=CreateWindowW(wc.lpszClassName,L"Windows Animations performance test",WS_OVERLAPPEDWINDOW,large?20:120,large?20:120,large?3600:1200,large?1900:760,0,0,wc.hInstance,0);
 ShowWindow(w,SW_SHOW);SetTimer(w,1,900,0);
 MSG msg;while(GetMessageW(&msg,0,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
 Sleep(1000);done=true;t.join();return 0;
}
