!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_INSTALLMODE_INSTDIR AWeather
!include "MultiUser.nsh"
!include "MUI2.nsh"

Function .onInit
	!insertmacro MULTIUSER_INIT
FunctionEnd

Function un.onInit
	!insertmacro MULTIUSER_UNINIT
FunctionEnd

name "AWeather"
!ifndef VERSION
	!define VERSION LATEST
!endif
!ifdef USE_GTK
	outFile "aweather-${VERSION}-gtk.exe"
!else
	outFile "aweather-${VERSION}.exe"
!endif
installDir AWeather
Icon "data/aweather.ico"

!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_LICENSE "COPYING"
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

section "AWeather (required)" SecAWeather
	SectionIn RO

	setOutPath $INSTDIR
	file /r build/*
	!ifdef USE_GTK
		file /r gtk/*
	!endif
	
	StrCmp $MultiUser.InstallMode "AllUsers" 0 +4
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AWeather" "DisplayName" "AWeather"
		WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AWeather" "UninstallString" "$INSTDIR\uninstaller.exe"
	Goto +3
		WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\AWeather" "DisplayName" "AWeather"
		WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\AWeather" "UninstallString" "$INSTDIR\uninstaller.exe"

	FileOpen $0 $INSTDIR\instmode.dat w
	FileWrite $0 $MultiUser.InstallMode
	FileClose $0
	
	writeUninstaller $INSTDIR\uninstaller.exe
sectionEnd
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${SecAWeather} "AWeather core files."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Desktop Icons"
	setOutPath $INSTDIR\bin
	CreateShortCut "$DESKTOP\AWeather.lnk" "$INSTDIR\bin\aweather.exe" "" "$INSTDIR\bin\aweather.exe" 0
SectionEnd

Section "Start Menu Shortcuts"
	setOutPath $INSTDIR\bin
	CreateDirectory "$SMPROGRAMS\AWeather"
	CreateShortCut  "$SMPROGRAMS\AWeather\Uninstall AWeather.lnk" "$INSTDIR\uninstaller.exe"  "" "$INSTDIR\uninstaller.exe"  0
	CreateShortCut  "$SMPROGRAMS\AWeather\AWeather.lnk"           "$INSTDIR\bin\aweather.exe" "" "$INSTDIR\bin\aweather.exe" 0
	CreateShortCut  "$SMPROGRAMS\AWeather\AWeather (debug).lnk" "cmd.exe" "/K aweather-dbg.exe -d 7" "$INSTDIR\bin\aweather-dbg.exe" 0
SectionEnd

section "Uninstall"
	FileOpen $0 $INSTDIR\instmode.dat r
	FileRead $0 $1
	FileClose $0

	StrCmp $1 "AllUsers" 0 +4
		SetShellVarContext all
		DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AWeather"
	Goto +3
		SetShellVarContext current
		DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\AWeather"

	delete $DESKTOP\AWeather.exe.lnk

	rmdir /r $SMPROGRAMS\AWeather
	rmdir /r $INSTDIR
sectionEnd
