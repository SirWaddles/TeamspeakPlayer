#include "../events/events.h"
#include "../events/factory.h"
#include "audio.h"
#include <flite.h>

static EventsF::EventCreatorType<TextToSpeechEvent> tts("tts");

extern "C" {
	cst_voice *register_cmu_us_rms(const char* voxdir);
}

TextToSpeechEvent::TextToSpeechEvent() {

}

void TextToSpeechEvent::RunSingleEvent() {
	cst_voice* v;
	v = register_cmu_us_rms(NULL);
	cst_wave* wave = flite_text_to_wave(textToSay.c_str(), v);
	cst_wave_resample(wave, TARGET_SAMPLE_RATE);

	AudioFileData* nFile = new AudioFileData(wave->num_samples, wave->samples);
	nFile->looping = false;
	AudioM::getAudioManager()->AddFile(nFile);
}

void TextToSpeechEvent::SetupArgs(std::deque<std::string>& args) {
	for (auto it = args.begin(); it != args.end(); it++) {
		textToSay += *it;
		textToSay += " ";
	}
}

std::string TextToSpeechEvent::GetEventMessage() {
	return textToSay;
}
