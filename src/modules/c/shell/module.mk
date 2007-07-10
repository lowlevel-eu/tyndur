shell_SRC += $(call SOURCE_FILES,$(MODULE_PATH)/cmds))
shell_OBJS := $(call OBJECT_FILES,$(shell_SRC))