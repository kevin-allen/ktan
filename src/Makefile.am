bin_PROGRAMS = kacqtan
kacqtan_SOURCES = main.cpp rhd2000evalboard.cpp rhd2000datablock.cpp rhd2000registers.cpp okFrontPanelDLL.cpp mainWindow.cpp
kacqtan_CPPFLAGS = @GTKMM_CFLAGS@  -DDATADIR='"$(datadir)"' 
kacqtan_LDADD = @GTKMM_LIBS@
kacqtan_LDFLAGS= -export-dynamic
