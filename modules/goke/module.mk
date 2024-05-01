#
# module.mk
#
# Copyright (C) 2010 2024 by Florian Seybold
#

MOD		:= goke
$(MOD)_SRCS	+= goke.c goke_play.c alias.c
$(MOD)_LFLAGS  += -lgk_api -lupvqe -lsecurec -lvoice_engine -ldnvqe #-Wl,goke.ld

include mk/mod.mk
