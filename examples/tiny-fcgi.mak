# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=tinyfcgi - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to tinyfcgi - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "tinyfcgi - Win32 Release" && "$(CFG)" !=\
 "tinyfcgi - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "tiny-fcgi.mak" CFG="tinyfcgi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tinyfcgi - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "tinyfcgi - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "tinyfcgi - Win32 Debug"
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tinyfcgi - Win32 Release"

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

ALL : "$(OUTDIR)\tiny-fcgi.exe"

CLEAN : 
	-@erase "$(INTDIR)\tiny-fcgi.obj"
	-@erase "$(OUTDIR)\tiny-fcgi.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D\
 "_CONSOLE" /Fp"$(INTDIR)/tiny-fcgi.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/tiny-fcgi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  ..\libfcgi\Release\libfcgi.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib  ..\libfcgi\Release\libfcgi.lib /nologo /subsystem:console\
 /incremental:no /pdb:"$(OUTDIR)/tiny-fcgi.pdb" /machine:I386\
 /out:"$(OUTDIR)/tiny-fcgi.exe" 
LINK32_OBJS= \
	"$(INTDIR)\tiny-fcgi.obj"

"$(OUTDIR)\tiny-fcgi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "tinyfcgi - Win32 Debug"

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

ALL : "$(OUTDIR)\tiny-fcgi.exe"

CLEAN : 
	-@erase "$(INTDIR)\tiny-fcgi.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\tiny-fcgi.exe"
	-@erase "$(OUTDIR)\tiny-fcgi.ilk"
	-@erase "$(OUTDIR)\tiny-fcgi.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MD /W3 /Gm /GX /Zi /Od /I "..\include" /D "WIN32" /D "_DEBUG"\
 /D "_CONSOLE" /Fp"$(INTDIR)/tiny-fcgi.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/"\
 /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/tiny-fcgi.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  ..\libfcgi\Debug\libfcgi.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib  ..\libfcgi\Debug\libfcgi.lib /nologo /subsystem:console\
 /incremental:yes /pdb:"$(OUTDIR)/tiny-fcgi.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/tiny-fcgi.exe" 
LINK32_OBJS= \
	"$(INTDIR)\tiny-fcgi.obj"

"$(OUTDIR)\tiny-fcgi.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

# Name "tinyfcgi - Win32 Release"
# Name "tinyfcgi - Win32 Debug"

!IF  "$(CFG)" == "tinyfcgi - Win32 Release"

!ELSEIF  "$(CFG)" == "tinyfcgi - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=".\tiny-fcgi.c"

!IF  "$(CFG)" == "tinyfcgi - Win32 Release"

DEP_CPP_TINY_=\
	"..\include\fcgi_stdio.h"\
	"..\include\fcgiapp.h"\
	".\*"\
	{$(INCLUDE)}"\sys\types.h"\
	
NODEP_CPP_TINY_=\
	"..\include\fcgi_config.h"\
	

"$(INTDIR)\tiny-fcgi.obj" : $(SOURCE) $(DEP_CPP_TINY_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "tinyfcgi - Win32 Debug"

DEP_CPP_TINY_=\
	"..\include\fcgi_config.h"\
	"..\include\fcgi_stdio.h"\
	"..\include\fcgiapp.h"\
	{$(INCLUDE)}"\sys\types.h"\
	

"$(INTDIR)\tiny-fcgi.obj" : $(SOURCE) $(DEP_CPP_TINY_) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
