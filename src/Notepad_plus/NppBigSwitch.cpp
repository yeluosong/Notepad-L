// NppBigSwitch.cpp
//
// In the original NotePad-L this file hosts the huge WM_COMMAND/WM_NOTIFY
// dispatcher. In M1 that logic lives inline in Notepad_plus_Window::WndProc
// for readability; this file is kept as a placeholder so the module layout
// matches the planned architecture and future milestones can split the
// dispatcher out here without touching CMake.
