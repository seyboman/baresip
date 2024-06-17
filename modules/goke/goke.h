/**
 * @file goke.h  GOKE sound driver -- internal interface
 *
 * Copyright (C) 2024 Florian Seybold
 */

#include <gk_api_audio.h>
#include <audio_acodec.h>


extern char goke_dev[64];

/*int alsa_reset(snd_pcm_t *pcm, uint32_t srate, uint32_t ch,
	       uint32_t num_frames, snd_pcm_format_t pcmfmt);*/
AUDIO_BIT_WIDTH_E aufmt_to_audiobitwidth(enum aufmt fmt);
ACODEC_FS_E srate_to_acodec_fs(uint32_t srate);
//snd_pcm_format_t aufmt_to_alsaformat(enum aufmt fmt);
int goke_src_alloc(struct ausrc_st **stp, const struct ausrc *as,
		   struct ausrc_prm *prm, const char *device,
		   ausrc_read_h *rh, ausrc_error_h *errh, void *arg);
int goke_play_alloc(struct auplay_st **stp, const struct auplay *ap,
		    struct auplay_prm *prm, const char *device,
		    auplay_write_h *wh, void *arg);
