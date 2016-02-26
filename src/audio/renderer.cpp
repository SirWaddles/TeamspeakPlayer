#include "audio.h"
#include "renderer.h"

void AudioM::SetupFileRenderer(){
	avcodec_register_all();
	av_register_all();
	avformat_network_init();
}

AudioFile::AudioFile() {
	lPacketQueue = new AudioPacketQueue(this);
	rPacketQueue = new AudioPacketQueue(this);
}

AudioFileEncoded::AudioFileEncoded(std::string filepath){

	extDets = new FFMpegAudioFile();
	TrackOver = false;

	extDets->pFormatCtx = NULL;
	int error = avformat_open_input(&(extDets->pFormatCtx), filepath.c_str(), NULL, NULL);
	if (error < 0){
		char stuff[100];
		av_strerror(error, stuff, 100);
		printf("Could not open input: %s\n",stuff);
		return;
	}
	avformat_find_stream_info(extDets->pFormatCtx, NULL);

	extDets->codec = NULL;
	stream = av_find_best_stream(extDets->pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &(extDets->codec), 0);
	if (stream < 0){
		char stuff[100];
		av_strerror(stream, stuff, 100);
		printf("Could not open codec: %s\n", stuff);
		return;
	}
	extDets->context = extDets->pFormatCtx->streams[stream]->codec;
	error = avcodec_open2(extDets->context, extDets->codec, NULL);
	if (error < 0){
		char stuff[100];
		av_strerror(error, stuff, 100);
		printf("Could not open codec: %s\n", stuff);
		return;
	}

	duration = extDets->pFormatCtx->duration / double(AV_TIME_BASE);

	extDets->resContext = swr_alloc();
	av_opt_set_int(extDets->resContext, "in_channel_layout", extDets->context->channel_layout, 0);
	av_opt_set_int(extDets->resContext, "out_channel_layout", 3, 0);
	av_opt_set_int(extDets->resContext, "in_sample_rate", extDets->context->sample_rate, 0);
	av_opt_set_int(extDets->resContext, "out_sample_rate", TARGET_SAMPLE_RATE, 0);
	//av_opt_set_int(resContext, "out_sample_rate", 44100, 0);
	av_opt_set_sample_fmt(extDets->resContext, "in_sample_fmt", extDets->context->sample_fmt, 0);
	av_opt_set_sample_fmt(extDets->resContext, "out_sample_fmt", AV_SAMPLE_FMT_S16P, 0);
	int ret = swr_init(extDets->resContext);
	if (ret < 0){
		char stuff[100];
		av_strerror(ret, stuff, 100);
		printf("Could not open codec: %s\n", stuff);
		return;
	}

	frames = 0;
}

AudioFile::~AudioFile() {
	if (lPacketQueue && rPacketQueue) {
		//lPacketQueue->ClearQueue();
		//rPacketQueue->ClearQueue();
		delete lPacketQueue;
		delete rPacketQueue;
	}
}

AudioFileEncoded::~AudioFileEncoded(){
	avcodec_close(extDets->context);
	avformat_close_input(&(extDets->pFormatCtx));
	swr_free(&(extDets->resContext));
}

bool AudioFileEncoded::readFrame(){
	AVPacket pkt;
	int err = av_read_frame(extDets->pFormatCtx, &pkt);
	if (err == AVERROR_EOF){
		printf("Reached end of file.\n");
		return false;
	}

	if (pkt.stream_index != stream){
		av_free_packet(&pkt);
		//printf("Incorrect stream\n");
		return true;
	}
	int gotFrame = 0;
	AVFrame *nFrame = new AVFrame();
	//avcodec_get_frame_defaults(&nFrame);
	av_frame_unref(nFrame);
	extDets->frame = nFrame;
	int len = avcodec_decode_audio4(extDets->context, nFrame, &gotFrame, &pkt);

	if (extDets->frame->nb_samples <= 0){
		av_free_packet(&pkt);
		printf("No samples found.\n");
		return true;
	}

	int or_data_size = av_samples_get_buffer_size(NULL, extDets->context->channels, extDets->frame->nb_samples, extDets->context->sample_fmt, 1) / extDets->context->channels;
	int reqSamples = av_rescale_rnd(extDets->frame->nb_samples, TARGET_SAMPLE_RATE, extDets->context->sample_rate, AV_ROUND_DOWN);
	int data_size = av_samples_get_buffer_size(NULL, 2, reqSamples, AV_SAMPLE_FMT_S16P, 1) / 2;
	//if (or_data_size > data_size) data_size = or_data_size;
	AudioPacket lPacket(data_size);
	AudioPacket rPacket(data_size);

	unsigned char* barr[2];
	barr[0] = lPacket.getData();
	barr[1] = rPacket.getData();

	swr_convert(extDets->resContext, barr, reqSamples, (const uint8_t**)extDets->frame->extended_data, extDets->frame->nb_samples);
	
	lPacketQueue->AddPacket(lPacket);
	rPacketQueue->AddPacket(rPacket);
	//printf("Read Bytes: %i\n", len);
	av_free_packet(&pkt);

	delete nFrame;
	
	//avcodec_free_frame(&frame);
	return true;

}

void AudioFileEncoded::OutOfData() {

}

AudioFileData::AudioFileData(int num_samples, short* samples) {
	int data_size = num_samples * sizeof(short);
	AudioPacket lPacket(data_size);
	AudioPacket rPacket(data_size);
	memcpy(lPacket.getData(), samples, data_size);
	memcpy(rPacket.getData(), samples, data_size);

	duration = double(num_samples) / 44000.0;
	frames = 1;
	lPacketQueue->AddPacket(lPacket);
	rPacketQueue->AddPacket(rPacket);
	TrackOver = false;
}

AudioFileData::~AudioFileData() {

}

TrackDeleteEvent::TrackDeleteEvent(AudioFile* tD){
	toDelete = tD;
}

void TrackDeleteEvent::RunEvent(){
	printf("Deleting track\n");
	delete toDelete;
}