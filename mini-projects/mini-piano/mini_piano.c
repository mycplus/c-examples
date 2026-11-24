/*
 * mini_piano.c - type a tune as note names and get a WAV file you can play.
 *
 * Each note's frequency comes from its name, the sound wave is generated
 * sample by sample, and the samples are written to a standard WAV file
 * that any media player opens. Type ode for the opening of Beethoven's
 * Ode to Joy.
 *
 * Build with GCC or Clang (-lm links the maths library for sin() and pow();
 * it must come after the source file):
 *     gcc -std=c11 -Wall -Wextra -o mini_piano mini_piano.c -lm
 * Build with Visual Studio (Developer Command Prompt):
 *     cl /W4 mini_piano.c
 * Run:
 *     ./mini_piano      then open the WAV file in any media player
 */
#define _CRT_SECURE_NO_WARNINGS     /* fopen on MSVC; must come before the includes */
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE    64
#define SAMPLE_RATE  22050          /* samples per second (CD quality is 44100) */
#define AMPLITUDE    0.3            /* 30% of full volume */
#define FADE_SAMPLES 220            /* 10 ms fade in and out per note */
#define MAX_NOTES    64
#define TUNE_SIZE    256            /* longest tune line, including '\0' */
#define PATH_SIZE    61             /* longest file name: 60 characters + '\0' */
/* M_PI is not standard C: it is POSIX, and MSVC hides it unless
 * _USE_MATH_DEFINES is defined. So the program defines its own. */
#define PI           3.14159265358979323846

/* One note of the tune, as parsed. */
typedef struct {
    char name[4];   /* as typed, uppercased: "C4", "F#5", "R" */
    int  midi;      /* MIDI note number, or -1 for a rest */
} Note;

/* Semitones above C for the letters A to G, indexed by letter - 'A'. */
static const int semitones[7] = { 9, 11, 0, 2, 4, 5, 7 };

/* The opening of Beethoven's Ode to Joy (public domain). */
static const char ode_to_joy[] = "E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 E4 D4 D4";

static int    read_line(char *buf, size_t size);
static int    read_int(const char *prompt, int lo, int hi, int *value);
static int    read_text(const char *prompt, char *out, size_t out_size);
static int    parse_note(const char *token, Note *out);
static int    parse_tune(char *line, Note notes[], int *count);
static double note_frequency(int midi);
static void   write_le16(FILE *f, unsigned value);
static void   write_le32(FILE *f, unsigned long value);
static void   write_wav_header(FILE *f, unsigned long data_bytes);
static void   write_note(FILE *f, const Note *note, long samples);
static long   write_wav(const char *path, const Note notes[], int count, int ms);
static void   print_table(const Note notes[], int count);

int main(int argc, char *argv[])
{
    char line[TUNE_SIZE], path[PATH_SIZE];
    Note notes[MAX_NOTES];
    int count = 0, ms = 0;

    (void)argv;
    if (argc != 1) {
        fprintf(stderr, "Usage: mini_piano\n");
        return 1;
    }

    printf("Mini Piano\n==========\n");
    printf("Type a tune as notes separated by spaces, for example: C4 D4 E4 F4 G4\n");
    printf("Notes are A to G, # makes a sharp, the octave is 0 to 8 (4 if left out), R is a rest.\n");
    printf("Type ode to load the opening of Beethoven's Ode to Joy.\n");

    int ok = 1;
    do {                                        /* ask until the tune is valid */
        printf("Tune: ");
        fflush(stdout);
        ok = read_line(line, sizeof line);
    } while (ok && !parse_tune(line, notes, &count));
    ok = ok && read_int("Note length in milliseconds (100-2000): ", 100, 2000, &ms);
    while (ok) {                                /* ask until the name ends in .wav */
        ok = read_text("Save as: ", path, PATH_SIZE);
        if (!ok)
            break;
        size_t len = strlen(path);
        if (len >= 4 && strcmp(path + len - 4, ".wav") == 0)
            break;
        printf("Please use a name ending in .wav.\n");
    }
    if (!ok) {
        printf("\n\nGoodbye!\n");              /* input ended */
        return 0;
    }

    print_table(notes, count);
    long size = write_wav(path, notes, count, ms);
    if (size < 0) {
        printf("Could not create %s.\n", path);
        return 1;
    }
    printf("\nWrote %d notes (%.2f seconds) to %s: %ld bytes.\n",
           count, count * ms / 1000.0, path, size);
    return 0;
}

/* Reads one line into buf and removes the newline. If the line is too long,
 * the rest is thrown away so it cannot become the next answer, and buf is
 * emptied so the caller rejects it instead of using half a number.
 * Returns 1 on success, 0 at end of input. */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else if (len == size - 1) {               /* buffer full: is there more? */
        int c, extra = 0;
        while ((c = getchar()) != '\n' && c != EOF)
            extra = 1;
        if (extra)
            buf[0] = '\0';
    }
    return 1;
}

/* Prints prompt and reads a whole number from lo to hi into *value.
 * Anything else (letters, an empty line, 12abc, out of range) prints a
 * message and asks again. Returns 1 on success, 0 at end of input. */
static int read_int(const char *prompt, int lo, int hi, int *value)
{
    char buf[LINE_SIZE];
    char *end;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        errno = 0;
        long n = strtol(buf, &end, 10);
        while (*end == ' ' || *end == '\t')
            end++;                              /* allow trailing spaces */
        if (end != buf && *end == '\0' && errno == 0 && n >= lo && n <= hi) {
            *value = (int)n;
            return 1;
        }
        printf("Please enter a whole number from %d to %d.\n", lo, hi);
    }
}

/* Prints prompt and reads a line of text, without spaces or tabs at either
 * end, into out. It must be 1 to out_size - 1 characters; anything else
 * (an empty line, a name that is too long) asks again.
 * Returns 1 on success, 0 at end of input. */
static int read_text(const char *prompt, char *out, size_t out_size)
{
    char buf[LINE_SIZE];

    for (;;) {
        char *p = buf, *end;

        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;
        while (*p == ' ' || *p == '\t')
            p++;
        end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t'))
            end--;
        size_t len = (size_t)(end - p);
        if (len >= 1 && len <= out_size - 1) {
            memcpy(out, p, len);
            out[len] = '\0';
            return 1;
        }
        printf("Please enter 1 to %d characters.\n", (int)(out_size - 1));
    }
}

/* Reads one note such as C4, f#5, E or R (a rest) into *out.
 * A note is a letter A to G, an optional #, then an optional octave 0 to 8
 * (4 if left out). Case does not matter. Flats, C9 or anything extra are
 * refused. The name is stored uppercased with the octave: "c" becomes "C4".
 * Returns 1 if the token is a valid note, 0 otherwise. */
static int parse_note(const char *token, Note *out)
{
    int letter = toupper((unsigned char)token[0]);
    int octave = 4, sharp = 0, p = 1;

    if (letter == 'R' && token[1] == '\0') {
        strcpy(out->name, "R");
        out->midi = -1;
        return 1;
    }
    if (letter < 'A' || letter > 'G')
        return 0;
    if (token[p] == '#') {
        sharp = 1;
        p++;
    }
    if (isdigit((unsigned char)token[p])) {
        octave = token[p] - '0';
        if (octave > 8)
            return 0;
        p++;
    }
    if (token[p] != '\0')
        return 0;                               /* "C#4#", "Cb4", "C10"... */

    int k = 0;                                  /* at most 3 characters + '\0' */
    out->name[k++] = (char)letter;
    if (sharp)
        out->name[k++] = '#';
    out->name[k++] = (char)('0' + octave);
    out->name[k] = '\0';
    /* MIDI numbers count semitones from C-1: C4 is 60 and A4 is 69. */
    out->midi = 12 * (octave + 1) + semitones[letter - 'A'] + sharp;
    return 1;
}

/* Splits line into notes and fills notes[0..*count-1]. The word ode on its
 * own loads Ode to Joy instead. strtok() cuts the line into words by writing
 * '\0' over the spaces, so it changes the string it is given.
 * Returns 1 if the tune is valid; otherwise prints why and returns 0. */
static int parse_tune(char *line, Note notes[], int *count)
{
    char ode[sizeof ode_to_joy];
    char *token = strtok(line, " \t");
    int n = 0;

    if (token != NULL && strlen(token) == 3 && tolower((unsigned char)token[0]) == 'o' &&
        tolower((unsigned char)token[1]) == 'd' && tolower((unsigned char)token[2]) == 'e' &&
        strtok(NULL, " \t") == NULL) {
        memcpy(ode, ode_to_joy, sizeof ode);    /* strtok needs a copy it may change */
        token = strtok(ode, " \t");
    }
    while (token != NULL) {
        if (n == MAX_NOTES) {
            printf("Please use at most %d notes.\n", MAX_NOTES);
            return 0;
        }
        if (!parse_note(token, &notes[n])) {
            printf("Sorry, \"%s\" is not a note.\n", token);
            return 0;
        }
        n++;
        token = strtok(NULL, " \t");
    }
    if (n == 0) {
        printf("Please type at least one note.\n");
        return 0;
    }
    *count = n;
    return 1;
}

/* Returns the frequency in Hz of a MIDI note number. A4 (69) is 440 Hz,
 * and each semitone multiplies the frequency by the twelfth root of 2. */
static double note_frequency(int midi)
{
    return 440.0 * pow(2.0, (midi - 69) / 12.0);
}

/* Writes value as 2 bytes, lowest byte first (little-endian). A WAV file is
 * little-endian on every computer, so the bytes are written one at a time
 * instead of fwrite(&value, ...), which would use this computer's order. */
static void write_le16(FILE *f, unsigned value)
{
    fputc((int)(value & 0xFF), f);
    fputc((int)((value >> 8) & 0xFF), f);
}

/* Writes value as 4 bytes, lowest byte first (little-endian). */
static void write_le32(FILE *f, unsigned long value)
{
    for (int k = 0; k < 4; k++)
        fputc((int)((value >> (8 * k)) & 0xFF), f);
}

/* Writes the 44-byte header of a 16-bit mono PCM WAV file, field by field
 * (fwrite of a struct could include padding bytes):
 *    0 "RIFF"   4 36 + data size   8 "WAVE"
 *   12 "fmt "  16 16 (format block size)  20 1 (PCM)  22 1 (mono)
 *   24 sample rate   28 bytes per second   32 2 (bytes per sample)
 *   34 16 (bits per sample)   36 "data"   40 data size in bytes */
static void write_wav_header(FILE *f, unsigned long data_bytes)
{
    fputs("RIFF", f);
    write_le32(f, 36 + data_bytes);
    fputs("WAVE", f);
    fputs("fmt ", f);
    write_le32(f, 16);
    write_le16(f, 1);
    write_le16(f, 1);
    write_le32(f, SAMPLE_RATE);
    write_le32(f, SAMPLE_RATE * 2);
    write_le16(f, 2);
    write_le16(f, 16);
    fputs("data", f);
    write_le32(f, data_bytes);
}

/* Writes samples sound samples for one note: a sine wave at the note's
 * frequency, or silence for a rest. Each note fades in and out over
 * FADE_SAMPLES: a wave that starts or stops suddenly makes a click. */
static void write_note(FILE *f, const Note *note, long samples)
{
    double freq = (note->midi < 0) ? 0.0 : note_frequency(note->midi);

    for (long i = 0; i < samples; i++) {
        double env = 1.0;
        if (i < FADE_SAMPLES)
            env = (double)i / FADE_SAMPLES;
        if (samples - 1 - i < FADE_SAMPLES && (double)(samples - 1 - i) / FADE_SAMPLES < env)
            env = (double)(samples - 1 - i) / FADE_SAMPLES;
        double value = (note->midi < 0) ? 0.0
                     : AMPLITUDE * env * sin(2.0 * PI * freq * (double)i / SAMPLE_RATE);
        int sample = (int)(value * 32767.0);    /* fits in a signed 16-bit range */
        write_le16(f, (unsigned)sample & 0xFFFF);   /* two's complement bytes */
    }
}

/* Writes the whole tune to a WAV file at path, every note ms long.
 * "wb" matters: in text mode Windows would turn every byte 10 into 13, 10
 * and corrupt the sound. Returns the file size measured with ftell(), or
 * -1 if the file could not be created or written. */
static long write_wav(const char *path, const Note notes[], int count, int ms)
{
    long samples = (long)SAMPLE_RATE * ms / 1000;   /* integer: 5512 at 250 ms */
    unsigned long data_bytes = (unsigned long)samples * (unsigned long)count * 2;
    FILE *f = fopen(path, "wb");

    if (f == NULL)
        return -1;
    write_wav_header(f, data_bytes);
    for (int k = 0; k < count; k++)
        write_note(f, &notes[k], samples);
    long size = ftell(f);                       /* measured, not calculated */
    if (ferror(f))                              /* did any fputc fail? */
        size = -1;
    if (fclose(f) != 0)
        size = -1;
    return size;
}

/* Prints each note with its frequency, or (rest), in an 18-wide table. */
static void print_table(const Note notes[], int count)
{
    printf("\n%-4s %13s\n", "Note", "Frequency");
    for (int k = 0; k < count; k++) {
        if (notes[k].midi < 0)
            printf("%-4s %13s\n", notes[k].name, "(rest)");
        else
            printf("%-4s %10.2f Hz\n", notes[k].name, note_frequency(notes[k].midi));
    }
}
