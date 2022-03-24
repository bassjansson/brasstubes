// Test playing a succession of MIDI files from the SD card.
// Example program to demonstrate the use of the MIDFile library
// Just for fun light up a LED in time to the music.
//
// Hardware required:
//  SD card interface - change SD_SELECT for SPI comms
//  3 LEDs (optional) - to display current status and beat.
//  Change pin definitions for specific hardware setup - defined below.

#include <MD_MIDIFile.h>
#include <SPI.h>
#include <SdFat.h>

#define USE_MIDI 1 // set to 1 to enable MIDI output, otherwise debug output

#if USE_MIDI // set up for direct MIDI serial output

#define DEBUG(s, x)
#define DEBUGX(s, x)
#define DEBUGS(s)
#define SERIAL_RATE 115200

#else // don't use MIDI to allow printing debug statements

#define DEBUG(s, x)                                                                                                                                            \
    do {                                                                                                                                                       \
        Serial.print(F(s));                                                                                                                                    \
        Serial.print(x);                                                                                                                                       \
    } while (false)
#define DEBUGX(s, x)                                                                                                                                           \
    do {                                                                                                                                                       \
        Serial.print(F(s));                                                                                                                                    \
        Serial.print(F("0x"));                                                                                                                                 \
        Serial.print(x, HEX);                                                                                                                                  \
    } while (false)
#define DEBUGS(s)                                                                                                                                              \
    do {                                                                                                                                                       \
        Serial.print(F(s));                                                                                                                                    \
    } while (false)
#define SERIAL_RATE 115200

#endif // USE_MIDI

// SD SPI pins
#define FSPI_CLK 12
#define FSPI_MISO 13
#define FSPI_MOSI 11
#define FSPI_CS0 10
#define FSPI_SCK SD_SCK_MHZ(20)

// LED definitions for status and user indicators
const uint8_t READY_LED = 7;     // when finished
const uint8_t SMF_ERROR_LED = 6; // SMF error
const uint8_t SD_ERROR_LED = 5;  // SD error
const uint8_t BEAT_LED = 4;      // toggles to the 'beat'

const uint16_t WAIT_DELAY = 2000; // ms

// The files in the tune list should be located on the SD card
// or an error will occur opening the file and the next in the
// list will be opened (skips errors).
const char *tuneList[] = {"MIDITEST.MID"};

SPIClass FSPI_SPI(FSPI);
SdFat SD;
MD_MIDIFile SMF;

// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
void midiCallback(midi_event *pev) {
#if USE_MIDI
    // if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0)) {
    //     Serial.write(pev->data[0] | pev->channel);
    //     Serial.write(&pev->data[1], pev->size - 1);
    // } else
    //     Serial.write(pev->data, pev->size);
    Serial.print(pev->data[0]);
    Serial.print(",");
    Serial.println(pev->data[1]);
#endif
    DEBUG("\n", millis());
    DEBUG("\tM T", pev->track);
    DEBUG(":  Ch ", pev->channel + 1);
    DEBUGS(" Data");
    for (uint8_t i = 0; i < pev->size; i++)
        DEBUGX(" ", pev->data[i]);
}

// Called by the MIDIFile library when a system Exclusive (sysex) file event needs
// to be processed through the midi communications interface. Most sysex events cannot
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
void sysexCallback(sysex_event *pev) {
    DEBUG("\nS T", pev->track);
    DEBUGS(": Data");
    for (uint8_t i = 0; i < pev->size; i++)
        DEBUGX(" ", pev->data[i]);
}

// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
void midiSilence(void) {
    midi_event ev;

    // All sound off
    // When All Sound Off is received all oscillators will turn off, and their volume
    // envelopes are set to zero as soon as possible.
    ev.size = 0;
    ev.data[ev.size++] = 0xb0;
    ev.data[ev.size++] = 120;
    ev.data[ev.size++] = 0;

    for (ev.channel = 0; ev.channel < 16; ev.channel++)
        midiCallback(&ev);
}

void setup(void) {
    // Set up LED pins
    pinMode(READY_LED, OUTPUT);
    pinMode(SD_ERROR_LED, OUTPUT);
    pinMode(SMF_ERROR_LED, OUTPUT);
    pinMode(BEAT_LED, OUTPUT);

    // reset LEDs
    digitalWrite(READY_LED, LOW);
    digitalWrite(SD_ERROR_LED, LOW);
    digitalWrite(SMF_ERROR_LED, LOW);
    digitalWrite(BEAT_LED, LOW);

    Serial.begin(SERIAL_RATE);
    FSPI_SPI.begin(FSPI_CLK, FSPI_MISO, FSPI_MOSI, FSPI_CS0);

    DEBUGS("\n[MidiFile Play List]");

    // Initialize SD
    if (!SD.begin(SdSpiConfig(FSPI_CS0, DEDICATED_SPI, FSPI_SCK, &FSPI_SPI))) {
        DEBUGS("\nSD init fail!");
        digitalWrite(SD_ERROR_LED, HIGH);
        while (true)
            ;
    }

    // Initialize MIDIFile
    SMF.begin(&SD);
    SMF.setMidiHandler(midiCallback);
    SMF.setSysexHandler(sysexCallback);

    digitalWrite(READY_LED, HIGH);
}

// flash a LED to the beat
void tickMetronome(void) {
    static uint32_t lastBeatTime = 0;
    static boolean inBeat = false;
    uint16_t beatTime;

    beatTime = 60000 / SMF.getTempo(); // msec/beat = ((60sec/min)*(1000 ms/sec))/(beats/min)
    if (!inBeat) {
        if ((millis() - lastBeatTime) >= beatTime) {
            lastBeatTime = millis();
            digitalWrite(BEAT_LED, HIGH);
            inBeat = true;
        }
    } else {
        if ((millis() - lastBeatTime) >= 100) // keep the flash on for 100ms only
        {
            digitalWrite(BEAT_LED, LOW);
            inBeat = false;
        }
    }
}

void loop(void) {
    static enum { S_IDLE, S_PLAYING, S_END, S_WAIT_BETWEEN } state = S_IDLE;
    static uint16_t currTune = ARRAY_SIZE(tuneList);
    static uint32_t timeStart;

    switch (state) {
    case S_IDLE: // now idle, set up the next tune
    {
        int err;

        DEBUGS("\nS_IDLE");

        digitalWrite(READY_LED, LOW);
        digitalWrite(SMF_ERROR_LED, LOW);

        currTune++;
        if (currTune >= ARRAY_SIZE(tuneList))
            currTune = 0;

        // use the next file name and play it
        DEBUG("\nFile: ", tuneList[currTune]);
        err = SMF.load(tuneList[currTune]);
        if (err != MD_MIDIFile::E_OK) {
            DEBUG(" - SMF load Error ", err);
            digitalWrite(SMF_ERROR_LED, HIGH);
            timeStart = millis();
            state = S_WAIT_BETWEEN;
            DEBUGS("\nWAIT_BETWEEN");
        } else {
            DEBUGS("\nS_PLAYING");
            state = S_PLAYING;
        }
    } break;

    case S_PLAYING: // play the file
        DEBUGS("\nS_PLAYING");
        if (!SMF.isEOF()) {
            if (SMF.getNextEvent())
                tickMetronome();
        } else
            state = S_END;
        break;

    case S_END: // done with this one
        DEBUGS("\nS_END");
        SMF.close();
        midiSilence();
        timeStart = millis();
        state = S_WAIT_BETWEEN;
        DEBUGS("\nWAIT_BETWEEN");
        break;

    case S_WAIT_BETWEEN: // signal finished with a dignified pause
        digitalWrite(READY_LED, HIGH);
        if (millis() - timeStart >= WAIT_DELAY)
            state = S_IDLE;
        break;

    default:
        state = S_IDLE;
        break;
    }
}
