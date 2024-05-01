#
# module.mk
#
# Copyright (C) 2010 2024 by Florian Seybold
#

MOD		:= goke
$(MOD)_SRCS	+= goke.c goke_play.c
$(MOD)_LFLAGS  += -lgk_api

include mk/mod.mk
