/**
 * @file goke.c  GOKE sound driver
 *
 * Copyright (C) 2024 Florian Seybold
 */
#define _DEFAULT_SOURCE 1
#define _POSIX_SOURCE 1

#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "goke.h"



/**
 * @defgroup alsa alsa
 *
 * GOKE audio driver module
 *
 *
 * References:
 *
 *    http://www.goke.com/en/
 */


//char goke_dev[64] = "default";

//static struct ausrc *ausrc;
static struct auplay *auplay;


/*/int alsa_reset(snd_pcm_t *pcm, uint32_t srate, uint32_t ch,
	       uint32_t num_frames,
	       snd_pcm_format_t pcmfmt)
{

}*/


/*snd_pcm_format_t aufmt_to_alsaformat(enum aufmt fmt)
{
	switch (fmt) {

	case AUFMT_S16LE:    return SND_PCM_FORMAT_S16;
	case AUFMT_FLOAT:    return SND_PCM_FORMAT_FLOAT;
	case AUFMT_S24_3LE:  return SND_PCM_FORMAT_S24_3LE;
	default:             return SND_PCM_FORMAT_UNKNOWN;
	}
}*/


static int goke_init(void)
{
	int err = 0;

	/*err  = ausrc_register(&ausrc, baresip_ausrcl(),
			      "alsa", alsa_src_alloc);*/
	err |= auplay_register(&auplay, baresip_auplayl(),
			       "goke", goke_play_alloc);

	return err;
}


static int goke_close(void)
{
	//ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);

	/* releases all resources of the global configuration tree,
	   and sets snd_config to NULL. */
	snd_config_update_free_global();

	return 0;
}


const struct mod_export DECL_EXPORTS(alsa) = {
	"goke",
	"sound",
	goke_init,
	goke_close
};
