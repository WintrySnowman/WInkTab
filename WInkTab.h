#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#define NOWTFUNCTIONS
#include "wintab.h"

typedef struct
{
	UINT pkCursor;
	DWORD pkButtons;
	DWORD pkX;
	DWORD pkY;
	UINT pkNormalPressure;
} PacketData;

typedef struct
{
	bool hasData;
	PacketData data;
} PacketQueueItem;

const UINT PACKET_QUEUE_SIZE = 256;
const UINT PACKET_QUEUE_MASK = PACKET_QUEUE_SIZE - 1;
const UINT PEN_HISTORY_SIZE = 50;
const LONG CMIN = 0;
const LONG CMAX = 65536;
const HCTX CONTEXT_HANDLE = (HCTX)1; // Bogus handle
