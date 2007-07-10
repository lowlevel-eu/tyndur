freetype_src = src/freetype/base/ftbase.c src/freetype/base/ftsystem.c src/freetype/base/ftglyph.c src/freetype/base/ftbitmap.c src/freetype/base/ftinit.c src/freetype/autofit/autofit.c src/freetype/smooth/smooth.c src/freetype/psaux/psaux.c src/freetype/psnames/psnames.c src/freetype/pshinter/pshinter.c src/freetype/cid/type1cid.c src/freetype/truetype/truetype.c src/freetype/raster/raster.c src/freetype/sfnt/sfnt.c src/freetype/gzip/ftgzip.c src/freetype/lzw/ftlzw.c
#Ausgelassene Dateien siehe Makefile.all

gui_SRC += $(call SOURCE_FILES,$(MODULE_PATH)/src) $(addprefix $(MODULE_PATH)/,$(freetype_src));
gui_OBJS := $(call OBJECT_FILES,$(gui_SRC))