/**
 * @file goke_play.c  GOKE sound driver - player
 *
 * Copyright (C) 2024 Florian Seybold
 */
#define _DEFAULT_SOURCE 1
#define _POSIX_SOURCE 1
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
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

int goke_reset(AUDIO_DEV AoDevId, uint32_t srate, GK_U32 chnCnt,
	       uint32_t num_frames,
	       AUDIO_BIT_WIDTH_E bitwidth)
{
   struct conf *conf = conf_cur();
   uint32_t conf_value;

	AIO_ATTR_S pstAttr;

	GK_S32 err;

	debug("goke: resetting device\n");

	pstAttr.enSamplerate = srate; // Assuming this is a valid value from AUDIO_SAMPLE_RATE_E
	pstAttr.enBitwidth = bitwidth; // Assuming this is a valid value from AUDIO_BIT_WIDTH_E
	pstAttr.enWorkmode = AIO_MODE_I2S_MASTER; // Assuming this is a valid value from AIO_MODE_E
	pstAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;	 // Assuming this is a valid value from AUDIO_SOUND_MODE_E
	pstAttr.u32EXFlag = 0; // Assuming this is a	 flag and 0 is a valid value
	pstAttr.u32FrmNum = num_frames; // Assuming 	this is the number of frames and 256 is a valid value
	pstAttr.u32PtNumPerFrm = 1024; // 	Assuming this is the number of points per frame and 1024 is a valid value
	pstAttr.u32ChnCnt = chnCnt; // Assuming this	 is the number of channels and 2 is a valid value
	pstAttr.u32ClkSel = 0; // Assuming this is a	 clock selector and 0 is a valid value
	pstAttr.enI2sType = AIO_I2STYPE_INNERCODEC; 	// Assuming this is a valid value from AIO_I2STYPE_E

   if (0 == conf_get_u32(conf, "goke_ao_samplerate", &conf_value)) { pstAttr.enSamplerate=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_bitwidth", &conf_value)) { pstAttr.enBitwidth=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_workmode", &conf_value)) { pstAttr.enWorkmode=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_soundmode", &conf_value)) { pstAttr.enSoundmode=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_exflag", &conf_value)) { pstAttr.u32EXFlag=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_frmNum", &conf_value)) { pstAttr.u32FrmNum=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_ptNumPerFrm", &conf_value)) { pstAttr.u32PtNumPerFrm=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_chnCnt", &conf_value)) { pstAttr.u32ChnCnt=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_clkSel", &conf_value)) { pstAttr.u32ClkSel=conf_value; }
   if (0 == conf_get_u32(conf, "goke_ao_i2sType", &conf_value)) { pstAttr.enI2sType=conf_value; }

	debug("goke: set attributes\n");            	
                                               	
	err = GK_API_AO_SetPubAttr(AoDevId, &pstAttr);
	if (err < 0) {
		warning("goke: cannot set public attributes (%d), enSamplerate: %d, enBitwidth: %d, enWorkmode: %d, enSoundmode: %d, u32EXFlag: %d, u32FrmNum: %d, u32PtNumPerFrm: %d, u32CznCnt: %d, u32ClkSel: %d, enI2sType: %d\n",
			err,
			pstAttr.enSamplerate,
			pstAttr.enBitwidth,
			pstAttr.enWorkmode,
			pstAttr.enSoundmode,
			pstAttr.u32EXFlag,
			pstAttr.u32FrmNum,
			pstAttr.u32PtNumPerFrm,
			pstAttr.u32ChnCnt,
			pstAttr.u32ClkSel,
			pstAttr.enI2sType
      );
		goto out;
	}

	err = 0;

out:
	if (err) {
		warning("goke: init failed: err=%d\n", err);
	}

	return err;
}

int goke_init_codec(uint32_t srate, int volume) {
   int result, fd;
   ACODEC_FS_E i2s_fs_sel;

   fd = open("/dev/acodec", O_RDWR);
   if (fd < 0) {
		warning("failed to open /dev/acodec\n");
      goto out;
   }

   result = ioctl(fd, ACODEC_SOFT_RESET_CTRL);
   if (result != GK_SUCCESS) {
      warning("ACODEC_SOFT_RESET_CTRL error: %s", result);
      goto out;
   }

   i2s_fs_sel = srate_to_acodec_fs(srate);
   result = ioctl(fd, ACODEC_SET_I2S1_FS, &i2s_fs_sel);
   if (result != GK_SUCCESS) {
      warning("ACODEC_SET_I2S1_FS: %s", result);
      goto out;
   }

   result = ioctl(fd, ACODEC_SET_OUTPUT_VOL, &volume);
   if (result != GK_SUCCESS) {
      warning("ACODEC_SET_OUTPUT_VOL: %s", result);
      goto out;
   }

out:
   close(fd);

   return result;
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

	debug("goke: init codec\n");
   err = goke_init_codec(st->prm.srate, 0x70);
	if (err) {
		warning("goke: could not init codec. (%d)\n", err);
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
