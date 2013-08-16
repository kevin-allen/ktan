bin_PROGRAMS = kacqtan
kacqtan_SOURCES = main.cpp rhd2000evalboard.cpp rhd2000datablock.cpp rhd2000registers.cpp okFrontPanelDLL.cpp
kacqtan_CPPFLAGS = @GTK_CFLAGS@ -DDATADIR='"$(datadir)"' -ldl
kacqtan_LDADD = @GTK_LIBS@ 
kacqtan_LDFLAGS= -export-dynamic
