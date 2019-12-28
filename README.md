# WInkTab

Adds Windows Ink / Tablet PC support to ZBrush 2020, via Wacom's WinTab API. It has been tested on a Surface Pro 7.

My motivation for this project has been to get ZBrush working over remote desktop (currently untested) with pen pressure support.

## Requirements

- Windows 8 or higher, 64-bit
- ZBrush 2020 (likely works with earlier versions, but untested - same with ZBrushCore)

## Installation

Copy `wintab32.dll` into the ZBrush directory (alongside `ZBrush.exe`). Despite its name, it is a 64-bit DLL.

## Limitations

- The Windows API calls used are only able to respect up to 1024 levels of pressure sensitivity, according to the documentation.
- No pen eraser support yet (but I don't think this was ever supported in ZBrush anyway).
- Whilst the approach here may be used to allow other wintab-supported software to work with Windows Ink, the DLL's data handling is rather rigid, so only accepts a specific configuration which ZBrush provides. If the need arises, it may be tweaked to become more flexible.
