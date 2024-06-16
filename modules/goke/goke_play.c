/**
 * @file goke_play.c  GOKE sound driver - player
 *
 * Copyright (C) 2024 Florian Seybold
 */
#define _DEFAULT_SOURCE 1
#define _POSIX_SOURCE 1
#include <pthread.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include <gk_api_audio.h>
#include "goke.h"


struct auplay_st {
	pthread_t thread;
	volatile bool run;
	//snd_pcm_t *write;
    AUDIO_DEV AoDevId;  // Audio output device ID
    AO_CHN AoChn;       // Audio output channel ID
	void *sampv;
	size_t sampc;
	auplay_write_h *wh;
	void *arg;
	struct auplay_prm prm;
	char *device;
};


static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	/* Wait for termination of other thread */
	if (st->run) {
		debug("goke: stopping playback thread (%s)\n", st->device);
		st->run = false;
		(void)pthread_join(st->thread, NULL);
	}

	if (st->AoDevId != -1 && st->AoChn != -1) {
		GK_API_AO_DisableChn(st->AoDevId, st->AoChn);
		GK_API_AO_Disable(st->AoDevId);
   }

	mem_deref(st->sampv);
	mem_deref(st->device);
}

static void *write_thread(void *arg)
{
	struct auplay_st *st = arg;
	struct auframe af;
	GK_S32 ret;
	int num_frames;

	debug("goke: init write thread\n");
	num_frames = st->prm.srate * st->prm.ptime / 1000;

	auframe_init(&af, st->prm.fmt, st->sampv, st->sampc, st->prm.srate,
		     st->prm.ch);

	while (st->run) {
		const int samples = num_frames;
		void *sampv;

		st->wh(&af, st->arg);

		sampv = st->sampv;

		ret = GK_API_AO_SendFrame(st->AoDevId, st->AoChn, sampv, samples);

		/*if (-EPIPE == n) {
			snd_pcm_prepare(st->write);

			n = snd_pcm_writei(st->write, sampv, samples);
			if (n < 0) {
				warning("alsa: write error: %s\n",
					snd_strerror((int) n));
			}
		}
		else*/
		if (ret != GK_SUCCESS) {
			if (st->run)
				warning("goke: write error: %d\n", ret);
		}
		/*else if (n != samples) {
			warning("alsa: write: wrote %d of %d samples\n",
				(int) n, samples);
		}*/
	}

	ret = GK_API_AO_ClearChnBuf(st->AoDevId, st->AoChn);
	if (ret != GK_SUCCESS) {
		if (st->run)
			warning("goke: clear channel buffer error: %d\n", ret);
	}

	return NULL;
}


int goke_play_alloc(struct auplay_st **stp, const struct auplay *ap,
		    struct auplay_prm *prm, const char *device,
		    auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	AUDIO_BIT_WIDTH_E bidwidth;
	//snd_pcm_format_t pcmfmt;
	int num_frames;
	GK_S32 err;

	debug("goke: allocating device\n");

	if (!stp || !ap || !prm || !wh)
		return EINVAL;

	if (!str_isset(device))
		device = goke_dev;
	AUDIO_DEV AoDevId = 0;
	AO_CHN AoChn = 0;

	st = mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	err = str_dup(&st->device, device);
	if (err)
		goto out;

	st->prm = *prm;
	st->wh  = wh;
	st->arg = arg;

	st->sampc = prm->srate * prm->ch * prm->ptime / 1000;
	num_frames = st->prm.srate * st->prm.ptime / 1000;

	st->sampv = mem_alloc(aufmt_sample_size(prm->fmt) * st->sampc, NULL);
	if (!st->sampv) {
		err = ENOMEM;
		goto out;
	}

	bidwidth = aufmt_to_audiobitwidth(prm->fmt);
	if (bidwidth == AUDIO_BIT_WIDTH_BUTT) {
		warning("goke: unknown sample format '%s'\n",
			aufmt_name(prm->fmt));
		err = EINVAL;
		goto out;
	}

	//err = alsa_reset(st->write, st->prm.srate, st->prm.ch, num_frames,
	//		 pcmfmt);
	debug("goke: resetting device %d\n", AoDevId);
   err = goke_reset(AoDevId, st->prm.srate, st->prm.ch, num_frames, bidwidth);
	if (err) {
		warning("goke: could not reset player '%s' (%d)\n",
			st->device, err);
		goto out;
	}

	debug("goke: enabling device %d\n", AoDevId);
	//err = snd_pcm_open(&st->write, st->device, SND_PCM_STREAM_PLAYBACK, 0);
	err = GK_API_AO_Enable(AoDevId);
	if (err < 0) {
		warning("goke: could not enable device '%d' (%d)\n",
			AoDevId, err);
		goto out;
	}

	debug("goke: enabling channel %d on device %d\n", AoChn, AoDevId);
	err = GK_API_AO_EnableChn(AoDevId, AoChn);
	if (err < 0) {
		warning("goke: could not enable channel '%d' (%d)\n",
			AoChn, err);
		goto out;
	}

	debug("goke: starting device thread\n");
	st->run = true;
	err = pthread_create(&st->thread, NULL, write_thread, st);
	if (err) {
		st->run = false;
		goto out;
	}

	//debug("goke: playback started (%s)\n", st->device);

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}
