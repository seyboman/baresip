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


char goke_dev[64] = "default";

//static struct ausrc *ausrc;
static struct auplay *auplay;



/*/int alsa_reset(snd_pcm_t *pcm, uint32_t srate, uint32_t ch,
	       uint32_t num_frames,
	       snd_pcm_format_t pcmfmt)
{
	snd_pcm_hw_params_t *hw_params = NULL;
	snd_pcm_uframes_t period = num_frames, bufsize = num_frames * 4;
	int err;

	debug("alsa: reset: srate=%u, ch=%u, num_frames=%u, pcmfmt=%s\n",
	      srate, ch, num_frames, snd_pcm_format_name(pcmfmt));

	err = snd_pcm_hw_params_malloc(&hw_params);
	if (err < 0) {
		warning("alsa: cannot allocate hw params (%s)\n",
			snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_any(pcm, hw_params);
	if (err < 0) {
		warning("alsa: cannot initialize hw params (%s)\n",
			snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_access(pcm, hw_params,
					   SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		warning("alsa: cannot set access type (%s)\n",
			snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_format(pcm, hw_params, pcmfmt);
	if (err < 0) {
		warning("alsa: cannot set sample format %d (%s)\n",
			pcmfmt, snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_rate(pcm, hw_params, srate, 0);
	if (err < 0) {
		warning("alsa: cannot set sample rate to %u Hz (%s)\n",
			srate, snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_channels(pcm, hw_params, ch);
	if (err < 0) {
		warning("alsa: cannot set channel count to %d (%s)\n",
			ch, snd_strerror(err));
		goto out;
	}

	err = snd_pcm_hw_params_set_period_size_near(pcm, hw_params,
						     &period, 0);
	if (err < 0) {
		warning("alsa: cannot set period size to %d (%s)\n",
			period, snd_strerror(err));
	}

	err = snd_pcm_hw_params_set_buffer_size_near(pcm, hw_params, &bufsize);
	if (err < 0) {
		warning("alsa: cannot set buffer size to %d (%s)\n",
			bufsize, snd_strerror(err));
	}

	err = snd_pcm_hw_params(pcm, hw_params);
	if (err < 0) {
		warning("alsa: cannot set parameters (%s)\n",
			snd_strerror(err));
		goto out;
	}

	err = snd_pcm_prepare(pcm);
	if (err < 0) {
		warning("alsa: cannot prepare audio interface for use (%s)\n",
			snd_strerror(err));
		goto out;
	}

	err = 0;

 out:
	snd_pcm_hw_params_free(hw_params);

	if (err) {
		warning("alsa: init failed: err=%d\n", err);
	}

	return err;
}*/

AUDIO_BIT_WIDTH_E aufmt_to_audiobitwidth(enum aufmt fmt)
{
	switch (fmt) {

	case AUFMT_S16LE:    return AUDIO_BIT_WIDTH_16;
	case AUFMT_FLOAT:    return AUDIO_BIT_WIDTH_BUTT; // Assuming float corresponds to an unsupported bit width
	case AUFMT_S24_3LE:  return AUDIO_BIT_WIDTH_24;
	default:             return AUDIO_BIT_WIDTH_BUTT; // Assuming this is the default case for unsupported formats
	}
}
/*snd_pcm_format_t aufmt_to_alsaformat(enum aufmt fmt)
{
	switch (fmt) {

	case AUFMT_S16LE:    return SND_PCM_FORMAT_S16;
	case AUFMT_FLOAT:    return SND_PCM_FORMAT_FLOAT;
	case AUFMT_S24_3LE:  return SND_PCM_FORMAT_S24_3LE;
	default:             return SND_PCM_FORMAT_UNKNOWN;
	}
}*/

ACODEC_FS_E srate_to_acodec_fs(uint32_t srate)
{
   ACODEC_FS_E i2s_fs_sel;

   switch (srate) {
      case 8000:  i2s_fs_sel = ACODEC_FS_8000;  break;
      case 16000: i2s_fs_sel = ACODEC_FS_16000; break;
      case 32000: i2s_fs_sel = ACODEC_FS_32000; break;
      case 11025: i2s_fs_sel = ACODEC_FS_11025; break;
      case 22050: i2s_fs_sel = ACODEC_FS_22050; break;
      case 44100: i2s_fs_sel = ACODEC_FS_44100; break;
      case 12000: i2s_fs_sel = ACODEC_FS_12000; break;
      case 24000: i2s_fs_sel = ACODEC_FS_24000; break;
      case 48000: i2s_fs_sel = ACODEC_FS_48000; break;
      case 64000: i2s_fs_sel = ACODEC_FS_64000; break;
      case 96000: i2s_fs_sel = ACODEC_FS_96000; break;
      default: i2s_fs_sel = ACODEC_FS_BUTT;
   }

   return i2s_fs_sel;
}

static int goke_init(void)
{
	int err = 0;

	debug("goke: init\n");
	/*err  = ausrc_register(&ausrc, baresip_ausrcl(),
			      "alsa", alsa_src_alloc);*/
	err |= auplay_register(&auplay, baresip_auplayl(),
			       "goke", goke_play_alloc);

	return err;
}


static int goke_close(void)
{
	debug("goke: close\n");
	//ausrc  = mem_deref(ausrc);
	auplay = mem_deref(auplay);

	/* releases all resources of the global configuration tree,
	   and sets snd_config to NULL. */
	//snd_config_update_free_global();

	return 0;
}


const struct mod_export DECL_EXPORTS(goke) = {
	"goke",
	"sound",
	goke_init,
	goke_close
};
