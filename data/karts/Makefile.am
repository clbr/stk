# data/karts

pkgdatadir= $(datadir)/games/$(PACKAGE)/data/karts

nobase_dist_pkgdata_DATA = $(shell find $(srcdir) -print0 -name "materials.xml") \
                           $(shell find $(srcdir) -print0 -name "*.png")         \
                           $(shell find $(srcdir) -print0 -name "*.jpg")         \
                           $(shell find $(srcdir) -print0 -name "*.b3d")         \
                           $(shell find $(srcdir) -print0 -name "*.ogg")         \
                           $(shell find $(srcdir) -print0 -name "*.challenge")   \
                           $(shell find $(srcdir) -print0 -iname "License.txt")  \
                           $(shell find $(srcdir) -print0 -name "kart.xml")
