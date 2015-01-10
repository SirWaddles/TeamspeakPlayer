#include "audio.h"
#include "renderer.h"
#include <thread>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

void AudioRenderer(){

}

void AudioM::SetupFileRenderer(){
	avcodec_register_all();
	
}

void AudioM::PlayFile(std::string filepath){
	AudioFile* nFile = new AudioFile(filepath);
	nFile->readFrame();
}

AudioFile::AudioFile(std::string filepath){
	//AVFormatContext* pFormatCtx = NULL;
	//avformat_open_input(&pFormatCtx, filepath.c_str(), NULL, NULL);

	codec = avcodec_find_decoder(AV_CODEC_ID_MP3);
	if (!codec){
		printf("Can't find codec.\n");
		return;
	}
	context = avcodec_alloc_context3(codec);
	avcodec_open2(context, codec, NULL);
	frame = av_frame_alloc();
	parser = av_parser_init(AV_CODEC_ID_MP3);

	fp = fopen(filepath.c_str(), "rb");
	if (!fp){
		printf("Can't Open\n");
	}
	printf("stuff\n");
	readBuffer();
	printf("test\n");
}

int AudioFile::readBuffer(){
	printf("bufread\n");
	int bytes_read = (int)fread(inbuf, 1, MP3_BUF_SIZE, fp);
	printf("bufread2\n");

	std::copy(inbuf, inbuf + bytes_read, std::back_inserter(buffer));

	return bytes_read;
}

bool AudioFile::update(bool& needsBytes){
	printf("Update1\n");
	needsBytes = false;
	if (buffer.size() == 0){
		needsBytes = true;
		return false;
	}
	printf("Update2\n");

	uint8_t* data = NULL;
	int size = 0;
	int len = av_parser_parse2(parser, context, &data, &size, &buffer[0], buffer.size(), 0, 0, AV_NOPTS_VALUE);
	printf("Update3\n");

	if (size == 0 && len >= 0){
		needsBytes = true;
		return false;
	}
	printf("Update4\n");

	if (len){
		decodeFrame(&buffer[0], size);
		buffer.erase(buffer.begin(), buffer.begin() + len);
		return true;
	}
	printf("Update5\n");

	return false;
}

void AudioFile::decodeFrame(uint8_t* data, int size){
	AVPacket pkt;
	int got_frame = 0;
	int len = 0;

	av_init_packet(&pkt);

	pkt.data = data;
	pkt.size = size;

	len = avcodec_decode_audio4(context, frame, &got_frame, &pkt);
	if (got_frame == 0){
		return;
	}

	++currentFrame;


}

void AudioFile::readFrame(){
	bool needs_more = false;
	printf("Reading Frame\n");
	while (!update(needs_more)){
		if (needs_more){
			readBuffer();
		}
	}
	printf("Frame Read\n");

}