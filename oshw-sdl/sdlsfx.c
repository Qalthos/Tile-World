/* sdlsfx.c: Creating the program's sound effects.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	"SDL.h"
#include	"sdlsfx.h"
#include	"../err.h"
#include	"../state.h"

/* Some generic default settings for the audio output.
 */
#define DEFAULT_SND_FMT		AUDIO_S16SYS
#define DEFAULT_SND_FREQ	22050
#define	DEFAULT_SND_CHAN	1

/* The data needed for each sound effect's wave.
 */
typedef	struct sfxinfo {
    Uint8	       *wave;		/* the actual wave data */
    Uint32		len;		/* size of the wave data */
    int			pos;		/* how much has been played already */
    int			playing;	/* is the wave currently playing? */
    char const	       *textsfx;	/* the onomatopoeia string */
} sfxinfo;

/* The data needed to talk to the sound output device.
 */
static SDL_AudioSpec	spec;

/* All of the sound effects.
 */
static sfxinfo		sounds[SND_COUNT];

/* TRUE if sound-playing has been enabled.
 */
static int		enabled = FALSE;

/* TRUE if the program is currently talking to a sound device.
 */
static int		hasaudio = FALSE;

/* The volume level.
 */
static int		volume = SDL_MIX_MAXVOLUME;

/* The sound buffer size scaling factor.
 */
static int		soundbufsize = 0;


/* Initialize the textual sound effects.
 */
static void initonomatopoeia(void)
{
    sounds[SND_CHIP_LOSES].textsfx      = "\"Bummer\"";
    sounds[SND_CHIP_WINS].textsfx       = "Tadaa!";
    sounds[SND_TIME_OUT].textsfx        = "Clang!";
    sounds[SND_TIME_LOW].textsfx        = "Ktick!";
    sounds[SND_DEREZZ].textsfx		= "Bzont!";
    sounds[SND_CANT_MOVE].textsfx       = "Mnphf!";
    sounds[SND_IC_COLLECTED].textsfx    = "Chack!";
    sounds[SND_ITEM_COLLECTED].textsfx  = "Slurp!";
    sounds[SND_BOOTS_STOLEN].textsfx    = "Flonk!";
    sounds[SND_TELEPORTING].textsfx     = "Bamff!";
    sounds[SND_DOOR_OPENED].textsfx     = "Spang!";
    sounds[SND_SOCKET_OPENED].textsfx   = "Clack!";
    sounds[SND_BUTTON_PUSHED].textsfx   = "Click!";
    sounds[SND_BOMB_EXPLODES].textsfx   = "Booom!";
    sounds[SND_WATER_SPLASH].textsfx    = "Plash!";
    sounds[SND_TILE_EMPTIED].textsfx    = "Whisk!";
    sounds[SND_WALL_CREATED].textsfx    = "Chunk!";
    sounds[SND_TRAP_ENTERED].textsfx    = "Shunk!";
    sounds[SND_SKATING_TURN].textsfx    = "Whing!";
    sounds[SND_SKATING_FORWARD].textsfx = "Whizz ...";
    sounds[SND_SLIDING].textsfx         = "Drrrr ...";
    sounds[SND_BLOCK_MOVING].textsfx    = "Scrrr ...";
    sounds[SND_SLIDEWALKING].textsfx    = "slurp slurp ...";
    sounds[SND_ICEWALKING].textsfx      = "snick snick ...";
    sounds[SND_WATERWALKING].textsfx    = "plip plip ...";
    sounds[SND_FIREWALKING].textsfx     = "crackle crackle ...";
}

/* Display the onomatopoeia for the currently playing sound effect.
 * Only the first sound is used, since we can't display multiple
 * strings.
 */
static void displaysoundeffects(unsigned long sfx, int display)
{
    unsigned long	flag;
    int			i;

    if (!display) {
	setdisplaymsg(NULL, 0, 0);
	return;
    }

    for (flag = 1, i = 0 ; flag ; flag <<= 1, ++i) {
	if (sfx & flag) {
	    setdisplaymsg(sounds[i].textsfx, 500, 10);
	    return;
	}
    }
}

/* The callback function that is called by the sound driver to supply
 * the latest sound effects. All the sound effects are checked, and
 * the ones that are being played get another chunk of their sound
 * data mixed into the output buffer. When the end of a sound effect's
 * wave data is reached, the one-shot sounds are changed to be marked
 * as not playing, and the continuous sounds are looped.
 */
static void sfxcallback(void *data, Uint8 *wave, int len)
{
    int	i, n;

    (void)data;
    memset(wave, spec.silence, len);
    for (i = 0 ; i < SND_COUNT ; ++i) {
	if (!sounds[i].wave)
	    continue;
	if (!sounds[i].playing)
	    if (!sounds[i].pos || i >= SND_ONESHOT_COUNT)
		continue;
	n = sounds[i].len - sounds[i].pos;
	if (n > len) {
	    SDL_MixAudio(wave, sounds[i].wave + sounds[i].pos, len, volume);
	    sounds[i].pos += len;
	} else {
	    SDL_MixAudio(wave, sounds[i].wave + sounds[i].pos, n, volume);
	    sounds[i].pos = 0;
	    if (i < SND_ONESHOT_COUNT) {
		sounds[i].playing = FALSE;
	    } else if (sounds[i].playing) {
		while (len - n >= (int)sounds[i].len) {
		    SDL_MixAudio(wave + n, sounds[i].wave, sounds[i].len,
				 volume);
		    n += sounds[i].len;
		}
		sounds[i].pos = len - n;
		SDL_MixAudio(wave + n, sounds[i].wave, sounds[i].pos, volume);
	    }
	}
    }
}

/*
 * The exported functions.
 */

/* Activate or deactivate the sound system. When activating for the
 * first time, the connection to the sound device is established. When
 * deactivating, the connection is closed.
 */
int setaudiosystem(int active)
{
    SDL_AudioSpec	des;
    int			n;

    if (!enabled)
	return !active;

    if (!active) {
	if (hasaudio) {
	    SDL_PauseAudio(TRUE);
	    SDL_CloseAudio();
	    hasaudio = FALSE;
	}
	return TRUE;
    }

    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
	    warn("Cannot initialize audio output: %s", SDL_GetError());
	    return FALSE;
	}
    }

    if (hasaudio)
	return TRUE;

    des.freq = DEFAULT_SND_FREQ;
    des.format = DEFAULT_SND_FMT;
    des.channels = DEFAULT_SND_CHAN;
    des.callback = sfxcallback;
    des.userdata = NULL;
    for (n = 1 ; n <= des.freq / TICKS_PER_SECOND ; n <<= 1) ;
    des.samples = (n << soundbufsize) >> 2;
    if (SDL_OpenAudio(&des, &spec) < 0) {
	warn("can't access audio output: %s", SDL_GetError());
	return FALSE;
    }
    hasaudio = TRUE;
    SDL_PauseAudio(FALSE);

    return TRUE;
}

/* Load a single wave file into memory. The wave data is converted to
 * the format expected by the sound device.
 */
int loadsfxfromfile(int index, char const *filename)
{
    SDL_AudioSpec	specin;
    SDL_AudioCVT	convert;
    Uint8	       *wavein;
    Uint8	       *wavecvt;
    Uint32		lengthin;

    if (!filename) {
	freesfx(index);
	return TRUE;
    }

    if (!enabled)
	return FALSE;
    if (!hasaudio)
	if (!setaudiosystem(TRUE))
	    return FALSE;

    if (!SDL_LoadWAV(filename, &specin, &wavein, &lengthin)) {
	warn("can't load %s: %s", filename, SDL_GetError());
	return FALSE;
    }

    if (SDL_BuildAudioCVT(&convert,
			  specin.format, specin.channels, specin.freq,
			  spec.format, spec.channels, spec.freq) < 0) {
	warn("can't create converter for %s: %s", filename, SDL_GetError());
	return FALSE;
    }
    if (!(wavecvt = malloc(lengthin * convert.len_mult)))
	memerrexit();
    memcpy(wavecvt, wavein, lengthin);
    SDL_FreeWAV(wavein);
    convert.buf = wavecvt;
    convert.len = lengthin;
    if (SDL_ConvertAudio(&convert) < 0) {
	warn("can't convert %s: %s", filename, SDL_GetError());
	return FALSE;
    }

    freesfx(index);
    SDL_LockAudio();
    sounds[index].wave = convert.buf;
    sounds[index].len = convert.len * convert.len_ratio;
    sounds[index].pos = 0;
    sounds[index].playing = FALSE;
    SDL_UnlockAudio();

    return TRUE;
}

/* Select the sounds effects to be played. sfx is a bitmask of sound
 * effect indexes. Any continuous sounds that are not included in sfx
 * are stopped. One-shot sounds that are included in sfx are
 * restarted.
 */
void playsoundeffects(unsigned long sfx)
{
    unsigned long	flag;
    int			i;

    if (!hasaudio || !volume) {
	displaysoundeffects(sfx, TRUE);
	return;
    }

    SDL_LockAudio();
    for (i = 0, flag = 1 ; i < SND_COUNT ; ++i, flag <<= 1) {
	if (sfx & flag) {
	    sounds[i].playing = TRUE;
	    if (sounds[i].pos && i < SND_ONESHOT_COUNT)
		sounds[i].pos = 0;
	} else {
	    if (i >= SND_ONESHOT_COUNT)
		sounds[i].playing = FALSE;
	}
    }
    SDL_UnlockAudio();
}

/* If action is negative, stop playing all sounds immediately.
 * Otherwise, just temporarily pause or unpause sound-playing.
 */
void setsoundeffects(int action)
{
    int	i;

    if (!hasaudio || !volume)
	return;

    if (action < 0) {
	SDL_LockAudio();
	for (i = 0 ; i < SND_COUNT ; ++i) {
	    sounds[i].playing = FALSE;
	    sounds[i].pos = 0;
	}
	SDL_UnlockAudio();
    } else {
	SDL_PauseAudio(!action);
    }
}

/* Release all memory for the given sound effect.
 */
void freesfx(int index)
{
    if (sounds[index].wave) {
	SDL_LockAudio();
	free(sounds[index].wave);
	sounds[index].wave = NULL;
	sounds[index].pos = 0;
	sounds[index].playing = FALSE;
	SDL_UnlockAudio();
    }
}

/* Set the current volume level to v. If display is true, the
 * new volume level is displayed to the user.
 */
int setvolume(int v, int display)
{
    char	buf[16];

    if (!hasaudio)
	return FALSE;
    if (v < 0)
	v = 0;
    else if (v > 10)
	v = 10;
    if (!volume && v)
	displaysoundeffects(0, FALSE);
    volume = (SDL_MIX_MAXVOLUME * v + 9) / 10;
    if (display) {
	sprintf(buf, "Volume: %d", v);
	setdisplaymsg(buf, 1000, 1000);
    }
    return TRUE;
}

/* Change the current volume level by delta. If display is true, the
 * new volume level is displayed to the user.
 */
int changevolume(int delta, int display)
{
    return setvolume(((10 * volume) / SDL_MIX_MAXVOLUME) + delta, display);
}

/* Shut down the sound system.
 */
static void shutdown(void)
{
    setaudiosystem(FALSE);
    if (SDL_WasInit(SDL_INIT_AUDIO))
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
    hasaudio = FALSE;
}

/* Initialize the module. If silence is TRUE, then the program will
 * leave sound output disabled.
 */
int _sdlsfxinitialize(int silence, int _soundbufsize)
{
    atexit(shutdown);
    enabled = !silence;
    soundbufsize = _soundbufsize;
    initonomatopoeia();
    if (enabled)
	setaudiosystem(TRUE);
    return TRUE;
}
