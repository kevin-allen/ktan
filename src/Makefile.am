bin_PROGRAMS = kacq
kacq_SOURCES = main.c callback.c comedi_code.c oscilloscope.c recording.c main.h ../config.h
kacq_CPPFLAGS = @GTK_CFLAGS@ -DDATADIR='"$(datadir)"'
kacq_LDADD = @GTK_LIBS@  -lcomedi 
kacq_LDFLAGS= -export-dynamic

xmldir = $(datadir)
dist_xml_DATA = kacq_sed.glade
kacq_sed.glade_SOURCE = kacq.glade
kacq_sed.glade: kacq.glade
	cat kacq.glade | $(SED) 's;/usr/local/share/;$(datadir)/;g' > kacq_sed.glade

EXTRA_DIST = kacq.glade 