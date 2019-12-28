#include "WInkTab.h"

HWND windowHandle = nullptr;
HCTX wintabContext = nullptr;
tagLOGCONTEXTA wintabPref;
POINTER_PEN_INFO penInfoHistory[PEN_HISTORY_SIZE];
PacketQueueItem packetQueue[PACKET_QUEUE_SIZE];
UINT packetQueueIndex = 0;
int screenWidth = 0;
int screenHeight = 0;

void ProcessPenInput(HWND hWnd, POINTER_PEN_INFO& penInfo)
{
	float screenRatioX = (float)wintabPref.lcOutExtX / (float)screenWidth;
	float screenRatioY = (float)wintabPref.lcOutExtY / (float)screenHeight;

	packetQueue[packetQueueIndex & PACKET_QUEUE_MASK].hasData = true;
	PacketData* packetData = &packetQueue[packetQueueIndex & PACKET_QUEUE_MASK].data;
	packetData->pkX = (int)((float)penInfo.pointerInfo.ptPixelLocation.x * screenRatioX) + wintabPref.lcOutOrgX;
	packetData->pkY = wintabPref.lcOutExtY - (int)((float)penInfo.pointerInfo.ptPixelLocation.y * screenRatioY);
	packetData->pkNormalPressure = (penInfo.penMask & PEN_MASK_PRESSURE) ? (penInfo.pressure << 6) : 0;
	packetData->pkCursor = 1; // No eraser support yet

	if (penInfo.pointerInfo.ButtonChangeType && POINTER_CHANGE_FIRSTBUTTON_DOWN)
	{
		packetData->pkButtons = (TBN_DOWN << 16) | 0;
	}
	else if (penInfo.pointerInfo.ButtonChangeType && POINTER_CHANGE_FIRSTBUTTON_UP)
	{
		packetData->pkButtons = (TBN_UP << 16) | 0;
	}
	else if (penInfo.pointerInfo.ButtonChangeType && POINTER_CHANGE_SECONDBUTTON_DOWN)
	{
		packetData->pkButtons = (TBN_DOWN << 16) | 1;
	}
	else if (penInfo.pointerInfo.ButtonChangeType && POINTER_CHANGE_SECONDBUTTON_UP)
	{
		packetData->pkButtons = (TBN_UP << 16) | 1;
	}
	else
	{
		packetData->pkButtons = 0;
	}

	// Send message and increment the queue index
	SendMessage(hWnd, _WT_PACKET(wintabPref.lcMsgBase), packetQueueIndex++, (LPARAM)CONTEXT_HANDLE);
}

LRESULT WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_POINTERENTER:
	case WM_POINTERLEAVE:
	case WM_POINTERDOWN:
	case WM_POINTERUP:
	case WM_POINTERUPDATE:
		UINT32 pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INPUT_TYPE pointerType;
		POINTER_PEN_INFO penInfo;
		if (GetPointerType(pointerId, &pointerType) && (pointerType == PT_PEN) && GetPointerPenInfo(pointerId, &penInfo))
		{
			UINT availablePenHistory = PEN_HISTORY_SIZE;
			if (GetPointerPenInfoHistory(pointerId, &availablePenHistory, &penInfoHistory[0]))
			{
				availablePenHistory = min(availablePenHistory, PEN_HISTORY_SIZE);
				for (int i = availablePenHistory - 1; i >= 0; i--)
				{
					ProcessPenInput(hWnd, penInfoHistory[i]);
				}
			}
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

UINT API WTInfoA(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
	if (nIndex != 0)
	{
		// Only support fetching all the data
		return 0;
	}

	if ((wCategory == 0) || (lpOutput == nullptr))
	{
		return sizeof(tagLOGCONTEXTA);
	}
	else if (wCategory == WTI_DEFCONTEXT)
	{
		tagLOGCONTEXTA info;
		memset(&info, 0, sizeof(info));
		strcpy_s(info.lcName, "WInkTab Wintab Emulation");
		info.lcOptions = CXO_MESSAGES;
		info.lcStatus = 0;
		info.lcLocks = 0;
		info.lcMsgBase = WT_DEFBASE;
		info.lcDevice = 0;
		info.lcPktRate = 100; // Packet rate, actually ignored
		info.lcPktData = PK_CURSOR | PK_BUTTONS | PK_X | PK_Y | PK_NORMAL_PRESSURE;
		info.lcMoveMask = PK_BUTTONS; // Buttons relative, everything else absolute
		info.lcBtnDnMask = -1;
		info.lcBtnUpMask = -1;
		info.lcInOrgX = CMIN;  info.lcInOrgY = CMIN;  info.lcInOrgZ = CMIN;
		info.lcInExtX = CMAX;  info.lcInExtY = CMAX;  info.lcInExtZ = CMIN;
		info.lcOutOrgX = CMIN; info.lcOutOrgY = CMIN; info.lcOutOrgZ = CMIN;
		info.lcOutExtX = CMAX; info.lcOutExtY = CMAX; info.lcOutExtZ = CMIN;
		info.lcSensX = CMAX;   info.lcSensY = CMAX;   info.lcSensZ = CMAX;
		info.lcSysMode = false;
		info.lcSysOrgX = 0;
		info.lcSysOrgY = 0;
		info.lcSysExtX = GetSystemMetrics(SM_CXSCREEN);
		info.lcSysExtY = GetSystemMetrics(SM_CYSCREEN);

		memcpy(lpOutput, &info, sizeof(info));
		return sizeof(info);
	}
	else
	{
		// Fetching anything except the default context will fail
		return 0;
	}
}

UINT API WTInfoW(UINT wCategory, UINT nIndex, LPVOID lpOutput)
{
	return 0;
}

HCTX API WTOpenA(HWND hWnd, LPLOGCONTEXTA lpLogCtx, BOOL fEnable)
{
	if ((wintabContext == nullptr) && fEnable)
	{
		if (lpLogCtx->lcPktData != (PK_CURSOR | PK_BUTTONS | PK_X | PK_Y | PK_NORMAL_PRESSURE))
		{
			// Unsupported data format
			return nullptr;
		}

		if (lpLogCtx->lcMoveMask != (PK_CURSOR | PK_BUTTONS | PK_X | PK_Y | PK_NORMAL_PRESSURE))
		{
			// Unsupported mask
			return nullptr;
		}

		if (lpLogCtx->lcPktMode != PK_BUTTONS)
		{
			// Only buttons can be relative
			return nullptr;
		}

		if (lpLogCtx->lcOptions != (CXO_SYSTEM | CXO_MESSAGES))
		{
			// Unsupported options
			return nullptr;
		}
		
		memcpy(&wintabPref, lpLogCtx, sizeof(wintabPref));
		wintabContext = CONTEXT_HANDLE;
		windowHandle = hWnd;
		screenWidth = GetSystemMetrics(SM_CXSCREEN);
		screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// Clear the packet queue
		packetQueueIndex = 0;
		for (int i = 0; i < PACKET_QUEUE_SIZE; i++)
		{
			packetQueue[i].hasData = false;
		}

		// Start listening for pointer events
		SetWindowSubclass(hWnd, &WindowProc, 0, 0);

		return CONTEXT_HANDLE;
	}
	else
	{
		return nullptr;
	}
}

HCTX API WTOpenW(HWND hWnd, LPLOGCONTEXTW lpLogCtx, BOOL fEnable)
{
	return 0;
}

BOOL API WTClose(HCTX hCtx)
{
	if ((wintabContext != nullptr) && (hCtx == CONTEXT_HANDLE))
	{
		wintabContext = nullptr;
		RemoveWindowSubclass(windowHandle, &WindowProc, 0);
		return true;
	}
	else
	{
		return false;
	}
}

int API WTPacketsGet(HCTX, int, LPVOID)
{
	return 0;
}

BOOL API WTPacket(HCTX hCtx, UINT wSerial, LPVOID lpPkt)
{
	if (packetQueue[wSerial & PACKET_QUEUE_MASK].hasData)
	{
		memcpy(lpPkt, &packetQueue[wSerial & PACKET_QUEUE_MASK].data, sizeof(PacketData));
		packetQueue[wSerial & PACKET_QUEUE_MASK].hasData = false;
		return true;
	}
	else
	{
		return false;
	}
}

BOOL API WTEnable(HCTX, BOOL)
{
	return false;
}

BOOL API WTOverlap(HCTX, BOOL)
{
	return false;
}

BOOL API WTConfig(HCTX, HWND)
{
	return false;
}

BOOL API WTGetA(HCTX, LPLOGCONTEXTA)
{
	return false;
}

BOOL API WTGetW(HCTX, LPLOGCONTEXTW)
{
	return false;
}

BOOL API WTSetA(HCTX, LPLOGCONTEXTA)
{
	return false;
}

BOOL API WTSetW(HCTX, LPLOGCONTEXTW)
{
	return false;
}

BOOL API WTExtGet(HCTX, UINT, LPVOID)
{
	return false;
}

BOOL API WTExtSet(HCTX, UINT, LPVOID)
{
	return false;
}

BOOL API WTSave(HCTX, LPVOID)
{
	return false;
}

HCTX API WTRestore(HWND, LPVOID, BOOL)
{
	return 0;
}

int API WTPacketsPeek(HCTX, int, LPVOID)
{
	return 0;
}

int API WTDataGet(HCTX, UINT, UINT, int, LPVOID, LPINT)
{
	return 0;
}

int API WTDataPeek(HCTX, UINT, UINT, int, LPVOID, LPINT)
{
	return 0;
}

DWORD API WTQueuePackets(HCTX)
{
	return 0;
}

int API WTQueueSizeGet(HCTX)
{
	return 0;
}

BOOL API WTQueueSizeSet(HCTX, int)
{
	return false;
}

HMGR API WTMgrOpen(HWND, UINT)
{
	return 0;
}

BOOL API WTMgrClose(HMGR)
{
	return false;
}

BOOL API WTMgrContextEnum(HMGR, WTENUMPROC, LPARAM)
{
	return false;
}

HWND API WTMgrContextOwner(HMGR, HCTX)
{
	return 0;
}

HCTX API WTMgrDefContext(HMGR, BOOL)
{
	return 0;
}

HCTX API WTMgrDefContextEx(HMGR, UINT, BOOL)
{
	return 0;
}

UINT API WTMgrDeviceConfig(HMGR, UINT, HWND)
{
	return 0;
}

BOOL API WTMgrExt(HMGR, UINT, LPVOID)
{
	return false;
}

BOOL API WTMgrCsrEnable(HMGR, UINT, BOOL)
{
	return false;
}

BOOL API WTMgrCsrButtonMap(HMGR, UINT, LPBYTE, LPBYTE)
{
	return false;
}

BOOL API WTMgrCsrPressureBtnMarks(HMGR, UINT, DWORD, DWORD)
{
	return false;
}

BOOL API WTMgrCsrPressureResponse(HMGR, UINT, UINT FAR*, UINT FAR*)
{
	return false;
}

BOOL API WTMgrCsrExt(HMGR, UINT, UINT, LPVOID)
{
	return false;
}

BOOL API WTQueuePacketsEx(HCTX, UINT FAR*, UINT FAR*)
{
	return false;
}

BOOL API WTMgrConfigReplaceExA(HMGR, BOOL, LPSTR, LPSTR)
{
	return false;
}

BOOL API WTMgrConfigReplaceExW(HMGR, BOOL, LPWSTR, LPSTR)
{
	return false;
}

HWTHOOK API WTMgrPacketHookExA(HMGR, int, LPSTR, LPSTR)
{
	return 0;
}

HWTHOOK API WTMgrPacketHookExW(HMGR, int, LPWSTR, LPSTR)
{
	return 0;
}

BOOL API WTMgrPacketUnhook(HWTHOOK)
{
	return false;
}

LRESULT API WTMgrPacketHookNext(HWTHOOK, int, WPARAM, LPARAM)
{
	return 0;
}

BOOL API WTMgrCsrPressureBtnMarksEx(HMGR, UINT, UINT FAR*, UINT FAR*)
{
	return false;
}
