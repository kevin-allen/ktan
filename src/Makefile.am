bin_PROGRAMS = ktan
ktan_SOURCES = main.cpp rhd2000evalboard.cpp rhd2000datablock.cpp rhd2000registers.cpp okFrontPanelDLL.cpp mainWindow.cpp acquisition.cpp
ktan_CPPFLAGS = @GTKMM_CFLAGS@  -DDATADIR='"$(datadir)"' 
ktan_LDADD = @GTKMM_LIBS@
ktan_LDFLAGS= -export-dynamic
