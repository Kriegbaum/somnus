#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

ifdef CONFIG_AUDIO_BOARD_CUSTOM
COMPONENT_ADD_INCLUDEDIRS += ./pmod_codec
COMPONENT_SRCDIRS += ./pmod_codec

COMPONENT_ADD_INCLUDEDIRS += ./hat_kit
COMPONENT_SRCDIRS += ./hat_kit
endif
