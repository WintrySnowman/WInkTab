# WInkTab

Adds Windows Ink / Tablet PC support to ZBrush 2020, via Wacom's WinTab API. It has been tested on a Surface Pro 7.

Adds pressure support via:
- Regular usage (without official Wacom driver installed)
- Remote Desktop (requires a recent version of Windows 10)
- EasyCanvas (iPad Pro, tested by third party)

## Requirements

- Windows 8 or higher, 64-bit
- Visual C++ Redistributable for Visual Studio 2015
- ZBrush 2020 (likely works with earlier versions, but untested - same with ZBrushCore)

## Installation

Copy `wintab32.dll` into the ZBrush directory (alongside `ZBrush.exe`). Despite its name, it is a 64-bit DLL.

## Limitations

- The Windows API calls used are only able to respect up to 1024 levels of pressure sensitivity, according to the documentation.
- No pen eraser support yet (but I don't think this was ever supported in ZBrush anyway).
- Whilst the approach here may be used to allow other wintab-supported software to work with Windows Ink, the DLL's data handling is rather rigid, so only accepts a specific configuration which ZBrush provides. If the need arises, it may be tweaked to become more flexible.
