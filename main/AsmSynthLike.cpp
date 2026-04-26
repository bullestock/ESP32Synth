#include "ESP32Synth.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"

ESP32Synth synth;

const int CH_BASS_SAW = 0;
const int CH_BASS_SUB = 1;
const int CH_CHORDS   = 2;
const int CH_LEAD     = 7;
const int CH_ARP      = 8;
const int CH_COUNTER  = 9;

#define TAPE_LEN 20000
static int32_t* abyssTape = nullptr;
static int writeHead = 0;

// Estados dos filtros
static int32_t lpState = 0; // Low-Pass (Escurece o som)

extern "C" void IRAM_ATTR theAbyssDSP(int32_t* mixBuffer, int numSamples) {
    if (!abyssTape) return;

    for (int i = 0; i < numSamples; i++) {
        int32_t dry = mixBuffer[i];

        int tap1 = writeHead - 4327;  if (tap1 < 0) tap1 += TAPE_LEN;
        int tap2 = writeHead - 11003; if (tap2 < 0) tap2 += TAPE_LEN;
        int tap3 = writeHead - 19013; if (tap3 < 0) tap3 += TAPE_LEN;

        int32_t wet = (abyssTape[tap1] >> 2) + (abyssTape[tap2] >> 2) + (abyssTape[tap3] >> 2);

        lpState = ((wet * 50) + (lpState * 206)) >> 8;

        int32_t feedback = (dry >> 1) + ((lpState * 200) >> 8);

        if (feedback > 32767) feedback = 32767;
        else if (feedback < -32768) feedback = -32768;

        abyssTape[writeHead] = feedback;
        writeHead++;
        if (writeHead >= TAPE_LEN) writeHead = 0;

        mixBuffer[i] = (dry) + (lpState << 1);
    }
}

static void setupSketch(){
  synth.begin(19, 27, 25, I2S_32BIT); // matches Arduino sketch pins
  synth.setMasterVolume(200);
  abyssTape = (int32_t*)heap_caps_calloc(TAPE_LEN, sizeof(int32_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  synth.setCustomDSP(theAbyssDSP);

  synth.setWave(CH_BASS_SAW, WAVE_SAW);
  synth.setWave(CH_BASS_SUB, WAVE_SINE);

  for(int i = 0; i < 5; i++){
    synth.setWave(CH_CHORDS + i, WAVE_PULSE);
    synth.setPulseWidth(CH_CHORDS + i, 128);
    synth.setEnv(CH_CHORDS + i, 50, 100, 150, 600);
  }

  synth.setWave(CH_ARP, WAVE_TRIANGLE);
  synth.setEnv(CH_ARP, 10, 50, 200, 100);
  synth.setWave(CH_LEAD, WAVE_PULSE);
  synth.setPulseWidth(CH_LEAD, 64);
  synth.setEnv(CH_LEAD, 20, 100, 200, 400);
  synth.setVibrato(CH_LEAD, 500, 15);

  synth.setWave(CH_COUNTER, WAVE_SINE);
  synth.setEnv(CH_COUNTER, 400, 0, 255, 800);
}

static void chord(uint32_t bs, uint32_t n1, uint32_t n2, uint32_t n3, uint32_t n4, uint32_t n5) {
  synth.noteOn(CH_BASS_SAW, bs, 100);
  synth.noteOn(CH_BASS_SUB, bs, 40);
  synth.noteOn(CH_CHORDS + 0, n1, 25);
  synth.noteOn(CH_CHORDS + 1, n2, 25);
  synth.noteOn(CH_CHORDS + 2, n3, 25);
  synth.noteOn(CH_CHORDS + 3, n4, 25);
  synth.noteOn(CH_CHORDS + 4, n5, 25);
}

static void chordOff() {
  synth.noteOff(CH_BASS_SAW);
  synth.noteOff(CH_BASS_SUB);
  for(int i = 0; i < 5; i++) { synth.noteOff(CH_CHORDS + i); }
}

static void part1_Intro() {
  chord(cs2, cs4, e4, gs4, cs5, e5);
  synth.setArpeggio(CH_ARP, 120, cs4, e4, gs4, cs5, gs4, e4);
  synth.noteOn(CH_ARP, cs4, 50);
  vTaskDelay(pdMS_TO_TICKS(4000)); chordOff(); synth.noteOff(CH_ARP); vTaskDelay(pdMS_TO_TICKS(500));

  chord(ds2, ds4, fs4, a4, ds5, fs5);
  synth.setArpeggio(CH_ARP, 120, ds4, fs4, a4, ds5, a4, fs4);
  synth.noteOn(CH_ARP, ds4, 50);
  vTaskDelay(pdMS_TO_TICKS(4000)); chordOff(); synth.noteOff(CH_ARP); vTaskDelay(pdMS_TO_TICKS(500));

  chord(e2, cs4, e4, gs4, cs5, e5);
  synth.setArpeggio(CH_ARP, 120, e4, gs4, b4, e5, b4, gs4);
  synth.noteOn(CH_ARP, e4, 50);
  vTaskDelay(pdMS_TO_TICKS(4000)); chordOff(); synth.noteOff(CH_ARP); vTaskDelay(pdMS_TO_TICKS(500));
  synth.noteOn(CH_BASS_SAW, e2, 100);
  synth.noteOn(CH_BASS_SUB, e2, 40);
  synth.slideFreqTo(CH_BASS_SAW, gs2, 1000);
  synth.slideFreqTo(CH_BASS_SUB, gs2, 1000);
  vTaskDelay(pdMS_TO_TICKS(1000));

  chord(gs2, c4, ds4, fs4, gs4, c5);
  synth.setArpeggio(CH_ARP, 60, gs4, c5, ds5, fs5);
  synth.noteOn(CH_ARP, gs4, 80);
  synth.noteOn(CH_COUNTER, gs3, 0);
  synth.slideVolTo(CH_COUNTER, 100, 2000);
  synth.slideFreqTo(CH_COUNTER, ds5, 2000);
  vTaskDelay(pdMS_TO_TICKS(3000));

  chordOff();
  synth.detachArpeggio(CH_ARP);
  synth.noteOff(CH_ARP);
  synth.noteOff(CH_COUNTER);
  vTaskDelay(pdMS_TO_TICKS(400));
}

static void part2_Improv_Csm() {
  chord(cs2, cs4, e4, gs4, cs5, e5);
  synth.setArpeggio(CH_ARP, 100, cs4, e4, gs4, cs5, gs4, e4);
  synth.noteOn(CH_ARP, cs4, 40);

  synth.noteOn(CH_COUNTER, e4, 70);
  synth.noteOn(CH_LEAD, gs4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, e4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, cs5, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, b4, 150); vTaskDelay(pdMS_TO_TICKS(1000));

  chordOff(); synth.noteOff(CH_COUNTER); synth.noteOff(CH_LEAD); vTaskDelay(pdMS_TO_TICKS(100));

  chord(ds2, ds4, fs4, a4, ds5, fs5);
  synth.noteOn(CH_COUNTER, a4, 70);

  synth.noteOn(CH_LEAD, a4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, fs4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, ds5, 300); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, cs5, 150); vTaskDelay(pdMS_TO_TICKS(1000));

  chordOff(); synth.noteOff(CH_COUNTER); synth.noteOff(CH_LEAD); vTaskDelay(pdMS_TO_TICKS(100));

  chord(e2, cs4, e4, gs4, cs5, e5);
  synth.noteOn(CH_COUNTER, b4, 70);

  synth.noteOn(CH_LEAD, gs4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, b4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, e5, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, ds5, 150); vTaskDelay(pdMS_TO_TICKS(1000));

  chordOff(); synth.noteOff(CH_COUNTER); synth.noteOff(CH_LEAD); vTaskDelay(pdMS_TO_TICKS(100));

  chord(gs2, c4, ds4, fs4, gs4, c5);
  synth.noteOn(CH_COUNTER, c5, 80);

  synth.noteOn(CH_LEAD, c5, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, gs4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, fs5, 400); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, e5, 200); vTaskDelay(pdMS_TO_TICKS(1000));

  chordOff(); synth.noteOff(CH_COUNTER); synth.noteOff(CH_LEAD);
  synth.detachArpeggio(CH_ARP); synth.noteOff(CH_ARP);
  vTaskDelay(pdMS_TO_TICKS(300));
}

static void part3_Modulation_Fsm() {
  chord(fs2, cs4, fs4, a4, cs5, fs5);
  synth.setArpeggio(CH_ARP, 90, fs4, a4, cs5, fs5, cs5, a4);
  synth.noteOn(CH_ARP, fs4, 50);

  synth.noteOn(CH_COUNTER, cs5, 70);

  synth.noteOn(CH_LEAD, a4, 160); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, fs4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, e5, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, ds5, 150); vTaskDelay(pdMS_TO_TICKS(1000));

  chordOff(); synth.noteOff(CH_COUNTER); synth.noteOff(CH_LEAD); vTaskDelay(pdMS_TO_TICKS(100));
  chord(gs2, d4, f4, gs4, d5, f5);
  synth.noteOn(CH_COUNTER, d5, 70);

  synth.noteOn(CH_LEAD, b4, 160); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, gs4, 150); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, f5, 300); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, d5, 150); vTaskDelay(pdMS_TO_TICKS(1000));

  chordOff(); synth.noteOff(CH_COUNTER); synth.noteOff(CH_LEAD);
  synth.detachArpeggio(CH_ARP); synth.noteOff(CH_ARP);
  vTaskDelay(pdMS_TO_TICKS(1000));
}

static void part4_Finalization() {

  chord(a2, e4, a4, cs5, e5, a5);
  synth.setArpeggio(CH_ARP, 100, a4, cs5, e5, a5, e5, cs5);
  synth.noteOn(CH_ARP, a4, 50);

  synth.noteOn(CH_COUNTER, a5, 60);

  synth.noteOn(CH_LEAD, cs5, 160); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, e5, 2000); vTaskDelay(pdMS_TO_TICKS(2000));

  chordOff(); synth.noteOff(CH_COUNTER); synth.noteOff(CH_LEAD); vTaskDelay(pdMS_TO_TICKS(100));
  chord(b2, ds4, fs4, b4, ds5, fs5);
  synth.setArpeggio(CH_ARP, 90, b4, ds5, fs5, b5, fs5, ds5);
  synth.noteOn(CH_ARP, b4, 55);

  synth.noteOn(CH_COUNTER, b5, 65);

  synth.noteOn(CH_LEAD, ds5, 160); vTaskDelay(pdMS_TO_TICKS(1000));
  synth.slideFreqTo(CH_LEAD, fs5, 2000); vTaskDelay(pdMS_TO_TICKS(2000));

  chordOff(); synth.noteOff(CH_COUNTER); synth.noteOff(CH_LEAD); vTaskDelay(pdMS_TO_TICKS(100));

  chord(cs2, gs4, cs5, fs5, gs5, cs6);
  synth.setArpeggio(CH_ARP, 60, cs5, fs5, gs5, cs6);
  synth.noteOn(CH_ARP, cs5, 70);
  synth.noteOn(CH_COUNTER, gs5, 70);
  synth.noteOn(CH_LEAD, fs5, 180);
  vTaskDelay(pdMS_TO_TICKS(3000));
  chord(cs2, gs4, cs5, f5, gs5, cs6);
  synth.setArpeggio(CH_ARP, 150, cs4, f4, gs4, cs5);
  synth.slideFreqTo(CH_LEAD, f5, 1000);
  vTaskDelay(pdMS_TO_TICKS(2000));

  for(int i = 0; i < 10; i++) {
    synth.slideVolTo(i, 0, 4000);
  }

  vTaskDelay(pdMS_TO_TICKS(4500));

  chordOff();
  synth.noteOff(CH_LEAD);
  synth.noteOff(CH_COUNTER);
  synth.detachArpeggio(CH_ARP);
  synth.noteOff(CH_ARP);
  vTaskDelay(pdMS_TO_TICKS(2000));
}

static void main_task(void* pv){
  (void) pv;
  setupSketch();
  while(true){
    part1_Intro();
    part2_Improv_Csm();
    part3_Modulation_Fsm();
    part4_Finalization();

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

extern "C" void startAsmSynth(void){
  xTaskCreate(&main_task, "asm_synth", 16 * 1024, NULL, 5, NULL);
}
