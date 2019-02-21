bin_PROGRAMS =  ktan ktanStartStop

ktanStartStop_SOURCE = ktanStartStop.c

ktan_SOURCES = main.cpp rhd2000evalboard.cpp rhd2000datablock.cpp rhd2000registers.cpp okFrontPanelDLL.cpp mainWindow.cpp acquisition.cpp recording.cpp shared_memory.cpp shared_memory_data.cpp timeKeeper.cpp dataBuffer.cpp channelGroup.cpp oscilloscope.cpp
ktan_CPPFLAGS = @GTKMM_CFLAGS@  -DDATADIR='"$(datadir)"' 
ktan_LDADD = @GTKMM_LIBS@


xmldir = $(datadir)
dist_xml_DATA = ktan_sed.glade
ktan_sed.glade_SOURCE = ktan.glade
ktan_sed.glade: ktan.glade
	cat ktan.glade | $(SED) 's;/usr/local/share/;$(datadir)/;g' > ktan_sed.glade
EXTRA_DIST = ktan.glade 


# Install my_script in $(bindir) and distribute it.
dist_bin_SCRIPTS = ktanCopyUserConfig.sh

