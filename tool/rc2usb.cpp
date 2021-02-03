//
// rc2usb.cpp
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#include "stdafx.h"
#include "MainDialog.h"

int Run()
{
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    CMainDialog dialog;
    dialog.DoModal(nullptr);
    return 0;
}

// _Use_decl_annotations_
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    return Run();
}

int main(int argc, char* argv[])
{
    return Run();
}
