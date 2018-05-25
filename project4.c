/*******************************
 *
 *  Project Name: Wave Goodbye
 *  Description: Process audio files based on user input
 *  File names: wave.h wave.c project4.c
 *  Date: Friday, March 23, 2018 at 11:59pm
 *  Authors: Jonathan Freaney, Muntabir Choudhury
 *
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "wave.h"

/**
 * Simple struct for storing and passing the wave file header and the sound
 * data together.  The "numSamples" field makes it much easier to keep track
 * of how many samples there are, which is the lengths of "left" and "right".
 */
typedef struct _WaveData {
	WaveHeader *header;
	int numSamples;
	short *left;
	short *right;
} WaveData;

// Error messages for various errors

#define ERROR_COMMAND_LINE_USAGE  "Usage: wave [[-r][-s factor][-f][-o delay][-i delay][-v scale][-e delay scale] < input > output"
#define ERROR_INSUFFICIENT_MEMORY "Program out of memory"
#define ERROR_FILE_NOT_RIFF       "File is not a RIFF file"
#define ERROR_BAD_FORMAT_CHUNK    "Format chunk is corrupted"
#define ERROR_BAD_DATA_CHUNK      "Data chunk is corrupted"
#define ERROR_NOT_STEREO          "File is not stereo"
#define ERROR_INVALID_SAMPLE_RATE "File does not use 44,100Hz sample rate"
#define ERROR_INVALID_SAMPLE_SIZE "File does not have 16-bit samples"
#define ERROR_INVALID_FILE_SIZE   "File size does not match size in header"

#define ERROR_INVALID_SPEED       "A positive number must be supplied for the speed change"
#define ERROR_INVALID_TIME        "A positive number must be supplied for the fade in and fade out time"
#define ERROR_INVALID_VOLUME      "A positive number must be supplied for the volume scale"
#define ERROR_INVALID_ECHO        "A positive number must be supplied for the echo delay and scale parameters"

/**
 * Prints an error-message to stderr and exits the program.
 *
 * @param message The error-message to print.
 */
void failure(char *message) {
	fprintf(stderr, "Error: %s\n", message);
	exit(1);
} 
/**
 * Reads the wave file header into an allocated struct and returns a pointer to
 * it.  In addition, the files format is validated.
 *
 * @return A pointer to the wave file header.
 */
WaveHeader *readFileHeader() {
	// Read the wave file header
	WaveHeader *header = malloc(sizeof(WaveHeader));
	if (header == NULL)
		failure(ERROR_INSUFFICIENT_MEMORY);
	readHeader(header);
	
	// Perform validation tests on file
	if (strncmp(header->ID, "RIFF", 4) != 0)
		failure(ERROR_FILE_NOT_RIFF);

	if (strncmp(header->formatChunk.ID, "fmt ", 4) != 0
		|| header->formatChunk.size != 16
		|| header->formatChunk.compression != 1)
		failure(ERROR_BAD_FORMAT_CHUNK);

	if (strncmp(header->dataChunk.ID, "data", 4) != 0)
		failure(ERROR_BAD_DATA_CHUNK);

	if (header->formatChunk.channels != 2)
		failure(ERROR_NOT_STEREO);

	if (header->formatChunk.sampleRate != 44100)
		failure(ERROR_INVALID_SAMPLE_RATE);

	if (header->formatChunk.bitsPerSample != 16)
		failure(ERROR_INVALID_SAMPLE_SIZE);

	return header;
}

/**
 * Reads the next byte from the input stream.  Fails if the end of the stream
 * is reached.  This separates checking for the end of stream from reading the
 * next byte, which makes the program smoother to read and write.
 *
 * @return The next byte from the input stream.
 */
int readByte() {
	int ch = getchar();
	if (ch == EOF)
		failure(ERROR_INVALID_FILE_SIZE);

	return ch;
}

/**
 * Reads the sound data from the input stream.  They are stored in the given
 * WaveData struct's "left" and "right" fields.  The "numSamples" field is
 * the number of bytes the sound data consumes divided by 4.
 *
 * @param A WaveData struct to store the sound data in.
 */
void readSoundData(WaveData *data) {
	// Divide by 4 to account for the sample size and number of channels.
	data->numSamples = data->header->dataChunk.size / 4;
	data->left  = malloc(sizeof(short) * data->numSamples);
	data->right = malloc(sizeof(short) * data->numSamples);
	if (data->left == NULL || data->right == NULL)
		failure(ERROR_INSUFFICIENT_MEMORY);

	// Each sample uses two bytes (a short).
	// Samples alternate between the left and right channels.
	for (int i = 0; i < data->numSamples; i++) {
		data->left[i]  = readByte() | (readByte() << 8);
		data->right[i] = readByte() | (readByte() << 8);
	}
}

/**
 * Writes the wave data to another file.
 *
 * @param data The WaveData containing to file header and sound data.
 */
void writeToFile(WaveData *data) {
	writeHeader(data->header);

	for (int i = 0; i < data->numSamples; i++) {
		putchar((data->left[i]  & 0x00FF) >> 0);
		putchar((data->left[i]  & 0xFF00) >> 8);
		putchar((data->right[i] & 0x00FF) >> 0);
		putchar((data->right[i] & 0xFF00) >> 8);
	}
}

/**
 * Note: Not required by this program. For debugging purposes.
 *
 * Prints the file header in a readable form.
 *
 * @param header The wave file header.
 */
void printWaveHeader(WaveHeader *header) {
	fprintf(stderr, "ID:              %.*s\n", 4, header->ID);
	fprintf(stderr, "Size:            %d\n", header->size);
	fprintf(stderr, "Format:          %.*s\n", 4, header->format);
	fprintf(stderr, "Format ID:       %.*s\n", 4, header->formatChunk.ID);
	fprintf(stderr, "Format Size:     %d\n", header->formatChunk.size);
	fprintf(stderr, "Compression:     %d\n", header->formatChunk.compression);
	fprintf(stderr, "Channels:        %d\n", header->formatChunk.channels);
	fprintf(stderr, "Sample Rate:     %d\n", header->formatChunk.sampleRate);
	fprintf(stderr, "Byte Rate:       %d\n", header->formatChunk.byteRate);
	fprintf(stderr, "Block Align:     %d\n", header->formatChunk.blockAlign);
	fprintf(stderr, "Bits Per Sample: %d\n", header->formatChunk.bitsPerSample);
	fprintf(stderr, "Data ID:         %.*s\n", 4, header->dataChunk.ID);
	fprintf(stderr, "Data Size:       %d\n", header->dataChunk.size);
	fprintf(stderr, "\n");
}

/**
 * Reads an argument and parses it as a flag.  On success, the 6 flags '-r',
 * '-s', '-f', '-o', '-i', '-v', and '-e' return the integer codes 0 - 5,
 * respectively, referring to individual actions to be taken.  Fails if none
 * of these are matched.
 *
 * @param arg The argument to parse.
 * @return An integer code representing the matched flag.
 */
int parseArgument(char *arg) {
	if (*arg++ == '-') {
		int action = -1;

		switch (*arg++) {
		case 'r': action = 0; break;
		case 's': action = 1; break;
		case 'f': action = 2; break;
		case 'o': action = 3; break;
		case 'i': action = 4; break;
		case 'v': action = 5; break;
		case 'e': action = 6; break;
		}

		// If matched and arg had a length of 2.
		if (action != -1 && *arg++ == '\0')
			return action;
	}

	// If '-[rsfoive]' is not matched.
	failure(ERROR_COMMAND_LINE_USAGE);
}

/**
 * Parses a command line argument as a double and returns the result.
 * The argument is first checked to ensure it matches the discription
 * of a positive integer, which here is '\d+.|.\d+|\d+.\d+'.
 * A negative double is returned upon failure.
 *
 * @param arg The argument to parse.
 * @return The parsed double.
 */
double parseDouble(char *arg) {
	// Check that arg is in the correct form.
	int hasDot = 0;
	for (int i = 0; arg[i]; i++) {
		if (arg[i] == '.') {
			if (hasDot)
				return -1;
			else
				hasDot = 1;

		} else if ('0' > arg[i] || arg[i] > '9') {
			return -1;
		}
	}

	// Parse the argument.
	return atof(arg);
}

/**
 * The '-r' action.  Reverses the order of the sound samples so they play in
 * reverse.
 *
 * @param data The WaveData struct containing the samples to reverse.
 */
void actionReverse(WaveData *data) {
	short temp;

	// Go half-way - too far would undo the reversal.
	for (int i = 0; i < data->numSamples / 2; i++) {
		temp = data->left[i];
		data->left[i] = data->left[data->numSamples - i - 1];
		data->left[data->numSamples - i - 1] = temp;

		temp = data->right[i];
		data->right[i] = data->right[data->numSamples - i - 1];
		data->right[data->numSamples - i - 1] = temp;
	}
}

/**
 * The '-s' action.  Slows down or speeds up the sound data.
 *
 * @param data The WaveData struct containing the samples to speed up or down.
 * @param factor How much to scale the speed.
 */
void actionChangeSpeed(WaveData *data, double factor) {
	if (factor <= 0)
		failure(ERROR_INVALID_SPEED);

	// Create new channels.
	int length = (int) (data->numSamples / factor);
	short *left = malloc(sizeof(short) * length);
	short *right = malloc(sizeof(short) * length);
	if (left == NULL || right == NULL)
		failure(ERROR_INSUFFICIENT_MEMORY);

	// Copy over the old sound data.
	for (int i = 0; i < length; i++) {
		int j = (int) (i * factor);

		left[i]  = data->left[j];
		right[i] = data->right[j];
	}

	// Update the data records.
	free(data->left);
	free(data->right);

	data->numSamples = length;
	data->left = left;
	data->right = right;

	data->header->size = sizeof(WaveHeader) + 4 * length;
	data->header->dataChunk.size = 4 * length;
}

/**
 * The '-f' action.  Swaps the left and the right channels.
 *
 * @param data The WaveData struct containing the channels to swap.
 */
void actionFlipChannels(WaveData *data) {
	short *temp = data->left;
	data->left = data->right;
	data->right = temp;
}

/**
 * The '-o' action.  Fades out the sound near the end of the data.
 *
 * @param data The WaveData stuct containing the samples to fade.
 * @param duration How long the fade out should last.
 */
void actionFadeOut(WaveData *data, double duration) {
	if (duration < 0)
		failure(ERROR_INVALID_TIME);

	int n = (int) (data->header->formatChunk.sampleRate * duration);
	short *left = data->left + (data->numSamples - n);
	short *right = data->right + (data->numSamples - n);

	// Fade out only last n samples of each channel.
	for (int i = 0; i < n; i++) {
		double factor = 1.0 - i / (double) n;
		
		left [i] *= factor * factor;
		right[i] *= factor * factor;
	}
}

/**
 * The '-i' action.  Fades in the sound near the start of the data.
 *
 * @param data The WaveData struct containing the samples to fade.
 * @param duration How long the fade in should last.
 */
void actionFadeIn(WaveData *data, double duration) {
	if (duration < 0)
		failure(ERROR_INVALID_TIME);

	int n = (int) (data->header->formatChunk.sampleRate * duration);
	short *left = data->left;
	short *right = data->right;

	// Fade in only first n samples of each channel.
	for (int i = 0; i < n; i++) {
		double factor = i / (double) n;

		left[i]  *= factor * factor;
		right[i] *= factor * factor;
	}
}

/**
 * Scales a short value by some double quantity.
 * Ensures the result will be on [SHRT_MIN, SHRT_MAX].
 *
 * @param sample The data to scale.
 * @param scale How much to scale the sample.
 * @return The scaled short.
 */
short scaleSample(short sample, double scale) {
	int temp = (int) (sample * scale);
	if (temp < SHRT_MIN)
		temp = SHRT_MIN;
	if (temp > SHRT_MAX)
		temp = SHRT_MAX;
	return (short) temp;
}

/**
 * The '-v' action.  Scales the volume of the data.
 *
 * @param data The WaveData struct containing the samples whose volume to scale.
 * @param scale How much to scale the volume.
 */
void actionVolume(WaveData *data, double scale) {
	if (scale < 0)
		failure(ERROR_INVALID_VOLUME);

	// Scale data in left and right channels.
	for (int i = 0; i < data->numSamples; i++) {
		data->left[i]  = scaleSample(data->left[i], scale);
		data->right[i] = scaleSample(data->right[i], scale);
	}
}

/**
 * The '-e' action.  Adds an echo to the sound data.
 *
 * @param data The WaveData struct containing the data to add echos to.
 * @param delay How far in to the data to add the delay.
 * @param scale How much to scale the volume of the echo.
 */
void actionEcho(WaveData *data, double delay, double scale) {
	if (delay < 0 || scale < 0)
		failure(ERROR_INVALID_ECHO);

	int n = (int) (data->header->formatChunk.sampleRate * delay);
	short *left  = malloc(sizeof(short) * (data->numSamples + n));
	short *right = malloc(sizeof(short) * (data->numSamples + n));
	if (left == NULL || right == NULL)
		failure(ERROR_INSUFFICIENT_MEMORY);

	for (int i = 0; i < data->numSamples + n; i++) {
		left[i]  = (i < data->numSamples ? data->left[i] : 0);
		right[i] = (i < data->numSamples ? data->right[i] : 0);

		if (i >= n) {
			left[i]  += scaleSample(data->left[i - n], scale);
			right[i] += scaleSample(data->right[i - n], scale);
		}
	}

	// Update data records
	free(data->left);
	free(data->right);

	data->numSamples += n;
	data->left = left;
	data->right = right;

	data->header->size += 4 * n;
	data->header->dataChunk.size += 4 * n;
}

// The main function.  Program begins here.
int main(int argc, char **argv) {
	// Create WaveData struct and load in file.
	WaveData data;
	data.header = readFileHeader();
	readSoundData(&data);

	// Print out file header for convenience.
	fprintf(stderr, "\nInput Wave Header Information\n\n");
	printWaveHeader(data.header);

	for (int i = 1; i < argc; i++) {
		int action = parseArgument(argv[i]);
		
		// Perform the action specified by the given argument.
		switch (action) {
		case 0: actionReverse(&data); break;
		case 1: actionChangeSpeed(&data, parseDouble(argv[++i])); break;
		case 2: actionFlipChannels(&data); break;
		case 3: actionFadeOut(&data, parseDouble(argv[++i])); break;
		case 4: actionFadeIn(&data, parseDouble(argv[++i])); break;
		case 5: actionVolume(&data, parseDouble(argv[++i])); break;
		case 6: actionEcho(&data, parseDouble(argv[++i]), parseDouble(argv[++i])); break;
		}
	}

	// Print out file header to see comparison.
	fprintf(stderr, "\nOutput Wave Header Information\n\n");
	printWaveHeader(data.header);

	// Write data to file and free allocated memory.
	writeToFile(&data);
	free(data.header);
	free(data.left);
	free(data.right);

	return 0;
}

