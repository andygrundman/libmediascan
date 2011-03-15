/*
 * audio_out_al.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#ifdef LIBAO_AL

#include <stdio.h>
#include <stdlib.h>
#include <dmedia/audio.h>
#include <inttypes.h>

#include "dca.h"
#include "audio_out.h"
#include "audio_out_internal.h"

typedef struct al_instance_s {
    ao_instance_t ao;
    ALport port;
    int sample_rate;
    int set_params;
    int flags;
} al_instance_t;

static int al_setup (ao_instance_t * _instance, int sample_rate, int * flags,
		     level_t * level, sample_t * bias)
{
    al_instance_t * instance = (al_instance_t *) _instance;

    if ((instance->set_params == 0) && (instance->sample_rate != sample_rate))
	return 1;
    instance->sample_rate = sample_rate;

    *flags = instance->flags;
    *level = CONVERT_LEVEL;
    *bias = CONVERT_BIAS;

    return 0;
}

static int al_play (ao_instance_t * _instance, int flags, sample_t * _samples)
{
    al_instance_t * instance = (al_instance_t *) _instance;
    int16_t int16_samples[256*6];
    int chans = -1;

#ifdef LIBDCA_DOUBLE
    convert_t samples[256 * 6];
    int i;

    for (i = 0; i < 256 * 6; i++)
	samples[i] = _samples[i];
#else
    convert_t * samples = _samples;
#endif

    chans = channels_multi (flags);
    flags &= DCA_CHANNEL_MASK | DCA_LFE;

    if (instance->set_params) {
	ALconfig config;
	ALpv params[2];

	config = alNewConfig ();
	if (!config) {
	    fprintf (stderr, "alNewConfig failed\n");
	    return 1;
	}
	if (alSetChannels (config, chans)) {
	    fprintf (stderr, "alSetChannels failed\n");
	    return 1;
	}
	if (alSetConfig (instance->port, config)) {
	    fprintf (stderr, "alSetConfig failed\n");
	    return 1;
	}
	alFreeConfig (config);

	params[0].param = AL_MASTER_CLOCK;
	params[0].value.i = AL_CRYSTAL_MCLK_TYPE;
	params[1].param = AL_RATE;
	params[1].value.ll = alIntToFixed (instance->sample_rate);
	if (alSetParams (alGetResource (instance->port), params, 2) < 0) {
	    fprintf (stderr, "alSetParams failed\n");
	    return 1;
	}

	instance->flags = flags;
	instance->set_params = 0;
    } else if ((flags == DCA_DOLBY) && (instance->flags == DCA_STEREO)) {
	fprintf (stderr, "Switching from stereo to dolby surround\n");
	instance->flags = DCA_DOLBY;
    } else if ((flags == DCA_STEREO) && (instance->flags == DCA_DOLBY)) {
	fprintf (stderr, "Switching from dolby surround to stereo\n");
	instance->flags = DCA_STEREO;
    } else if (flags != instance->flags)
	return 1;

    convert2s16_multi (samples, int16_samples, flags);
    alWriteFrames (instance->port, int16_samples, 256);

    return 0;
}

static void al_close (ao_instance_t * _instance)
{
    al_instance_t * instance = (al_instance_t *) _instance;

    alClosePort (instance->port);
}

static ao_instance_t * al_open (int flags)
{
    al_instance_t * instance;
    int format;

    instance = malloc (sizeof (al_instance_t));
    if (instance == NULL)
	return NULL;

    instance->ao.setup = al_setup;
    instance->ao.play = al_play;
    instance->ao.close = al_close;

    instance->sample_rate = 0;
    instance->set_params = 1;
    instance->flags = flags;

    instance->port = alOpenPort ("a52dec", "w", 0);
    if (instance->port < 0) {
	fprintf (stderr, "alOpenPort failed\n");
	free (instance);
	return NULL;
    }

    return (ao_instance_t *) instance;
}

ao_instance_t * ao_al_open (void)
{
    return al_open (DCA_STEREO);
}

ao_instance_t * ao_aldolby_open (void)
{
    return al_open (DCA_DOLBY);
}

ao_instance_t * ao_al4_open (void)
{
    return al_open (DCA_2F2R);
}

ao_instance_t * ao_al6_open (void)
{
    return al_open (DCA_3F2R | DCA_LFE);
}

#endif
