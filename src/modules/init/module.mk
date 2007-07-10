# Die Variablennamen sind immer Name des Moduls + _SRC bzw. _OBJS.
# Die .c-Dateien im Unterverzeichnis zlib hinzufügen.
init_SRC += $(call SOURCE_FILES,$(MODULE_PATH)/zlib))
# Die Liste der Objektdateien aktualisieren.
init_OBJS := $(call OBJECT_FILES,$(init_SRC))