# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=libfcgi - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to libfcgi - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libfcgi - Win32 Release" && "$(CFG)" !=\
 "libfcgi - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "libfcgi.mak" CFG="libfcgi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libfcgi - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libfcgi - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "libfcgi - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "libfcgi - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\libfcgi.dll"

CLEAN : 
	-@erase "$(INTDIR)\fcgi_stdio.obj"
	-@erase "$(INTDIR)\fcgiapp.obj"
	-@erase "$(INTDIR)\os_win32.obj"
	-@erase "$(OUTDIR)\libfcgi.dll"
	-@erase "$(OUTDIR)\libfcgi.exp"
	-@erase "$(OUTDIR)\libfcgi.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/libfcgi.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/libfcgi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)/libfcgi.pdb" /machine:I386 /out:"$(OUTDIR)/libfcgi.dll"\
 /implib:"$(OUTDIR)/libfcgi.lib" 
LINK32_OBJS= \
	"$(INTDIR)\fcgi_stdio.obj" \
	"$(INTDIR)\fcgiapp.obj" \
	"$(INTDIR)\os_win32.obj"

"$(OUTDIR)\libfcgi.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libfcgi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\libfcgi.dll"

CLEAN : 
	-@erase "$(INTDIR)\fcgi_stdio.obj"
	-@erase "$(INTDIR)\fcgiapp.obj"
	-@erase "$(INTDIR)\os_win32.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\libfcgi.dll"
	-@erase "$(OUTDIR)\libfcgi.exp"
	-@erase "$(OUTDIR)\libfcgi.ilk"
	-@erase "$(OUTDIR)\libfcgi.lib"
	-@erase "$(OUTDIR)\libfcgi.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MD /W3 /Gm /GX /Zi /Od /I "..\include" /D "WIN32" /D "_DEBUG"\
 /D "_WINDOWS" /Fp"$(INTDIR)/libfcgi.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c\
 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/libfcgi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib wsock32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)/libfcgi.pdb" /debug /machine:I386 /out:"$(OUTDIR)/libfcgi.dll"\
 /implib:"$(OUTDIR)/libfcgi.lib" 
LINK32_OBJS= \
	"$(INTDIR)\fcgi_stdio.obj" \
	"$(INTDIR)\fcgiapp.obj" \
	"$(INTDIR)\os_win32.obj"

"$(OUTDIR)\libfcgi.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "libfcgi - Win32 Release"
# Name "libfcgi - Win32 Debug"

!IF  "$(CFG)" == "libfcgi - Win32 Release"

!ELSEIF  "$(CFG)" == "libfcgi - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\fcgi_stdio.c
DEP_CPP_FCGI_=\
	"..\include\fcgi_config.h"\
	"..\include\fcgi_stdio.h"\
	"..\include\fcgiapp.h"\
	"..\include\fcgios.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\fcgi_stdio.obj" : $(SOURCE) $(DEP_CPP_FCGI_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\fcgiapp.c
DEP_CPP_FCGIA=\
	"..\include\fastcgi.h"\
	"..\include\fcgi_config.h"\
	"..\include\fcgiapp.h"\
	"..\include\fcgiappmisc.h"\
	"..\include\fcgimisc.h"\
	"..\include\fcgios.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\fcgiapp.obj" : $(SOURCE) $(DEP_CPP_FCGIA) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\os_win32.c
DEP_CPP_OS_WI=\
	"..\include\fcgios.h"\
	{$(INCLUDE)}"\sys\timeb.h"\
	

"$(INTDIR)\os_win32.obj" : $(SOURCE) $(DEP_CPP_OS_WI) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
