# RetroPad Notepad

A lightweight retro-style text editor written in pure C using the Win32 API, inspired by Dave Plummer’s *Dave’s Garage* projects.  
Developed collaboratively by Codex LLM and adjusted, debugged, and finalized by Dmitry Ignatiev using ChatGPT Model 5.1 and Gemini 3 Pro.

**Note:**  
The goal of this project was to test how effectively an LLM could complete a full C/Win32 GUI application inside VS Code.  
It became clear that while the model could generate functional code, human insight and debugging were essential to achieve a complete and stable result.

---

## Features
- Simple text editing with Open and Save dialogs  
- Classic Windows 95-style interface  
- Custom application icon  
- Optional word wrap and status bar  
- Built using MinGW and a minimal Makefile

---

## Build Instructions

```bash
make
./retropad.exe
