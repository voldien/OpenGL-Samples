#include <AL/al.h>
#include <AL/alc.h>
#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <OpenALAudioInterface.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <fmt/core.h>
#include <glm/glm.hpp>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 28, 1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

// TODO add support for framebuffer blit or quad texture.

namespace glsample {

	class VideoPlayback : public GLSampleWindow {
	  public:
		VideoPlayback() : GLSampleWindow() {
			this->setTitle(fmt::format("VideoPlayback: {}", this->videoPath).c_str());
		}
		virtual ~VideoPlayback() {
			if (this->frame) {
				av_frame_free(&this->frame);
			}
			avcodec_free_context(&this->pAudioCtx);
			avcodec_free_context(&this->pVideoCtx);

			avformat_close_input(&this->pformatCtx);
			avformat_free_context(this->pformatCtx);
		}

		typedef struct _vertex_t {
			float pos[3];
			float uv[2];
		} Vertex;

		static const size_t nrVideoFrames = 2;
		int nthVideoFrame = 0;
		int frameSize;

		/*  */
		struct AVFormatContext *pformatCtx = nullptr;
		struct AVCodecContext *pVideoCtx = nullptr;
		struct AVCodecContext *pAudioCtx = nullptr;

		/*  */
		int videoStream;
		int audioStream;
		size_t video_width;
		size_t video_height;

		size_t audio_sample_rate;
		size_t audio_bit_rate;
		size_t audio_channel;

		/*  */
		struct AVFrame *frame = nullptr;
		struct AVFrame *frameoutput = nullptr;
		struct SwsContext *sws_ctx = nullptr;

		unsigned int flag;
		double video_clock;
		double frame_timer;
		double frame_last_pts;
		double frame_last_delay;

		/*  */
		unsigned int videoFramebuffer;
		unsigned int vbo;
		unsigned vao;

		unsigned int mSource;

		/*  */
		unsigned int videoplayback_program;

		/*  */
		size_t videoStageBufferMemorySize = 0;
		std::array<unsigned int, nrVideoFrames> videoFrameTextures;
		unsigned int videoStagingTextureBuffer; // PBO buffers

		fragcore::AudioClip *clip;
		fragcore::AudioListener *listener;
		fragcore::AudioSource *audioSource;
		std::shared_ptr<fragcore::OpenALAudioInterface> audioInterface;

		std::string videoPath = "video.mp4";

		/*	*/
		const std::string vertexShaderPath = "Shaders/videoplayback/videoplayback.vert.spv";
		const std::string fragmentShaderPath = "Shaders/videoplayback/videoplayback.frag.spv";

		const std::vector<Vertex> vertices = {{-1.0f, -1.0f, 0.0f, 0.0f, 0.0f},
											  {-1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
											  {1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
											  {1.0f, 1.0f, 0.0f, 1.0f, 0.0f}};

		// TODO add support to toggle between quad and blit.

		virtual void Release() override {

			/*	*/
			glDeleteProgram(this->videoplayback_program);
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);

			/*	*/
			glDeleteBuffers(1, &videoStagingTextureBuffer);
			glDeleteTextures(this->videoFrameTextures.size(), this->videoFrameTextures.data());
		}

		void loadVideo(const char *path) {
			int result;

			/*	*/
			this->pformatCtx = avformat_alloc_context();
			if (!pformatCtx) {
				throw cxxexcept::RuntimeException("Failed to allocate memory for the 'AVFormatContext'");
			}
			// Determine the input-format:
			this->pformatCtx->iformat = av_find_input_format(path);

			/*	*/
			result = avformat_open_input(&this->pformatCtx, path, nullptr, nullptr);
			if (result != 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to open input : {}", buf);
			}

			/*	*/
			if ((result = avformat_find_stream_info(this->pformatCtx, nullptr)) < 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to retrieve info from stream info : {}", buf);
			}

			struct AVStream *video_st = nullptr;
			struct AVStream *audio_st = nullptr;

			/*	Get video codecs.	*/
			for (unsigned int x = 0; x < this->pformatCtx->nb_streams; x++) {
				AVStream *stream = this->pformatCtx->streams[x];

				/*  */
				if (stream->codecpar) {

					switch (stream->codecpar->codec_type) {
					case AVMEDIA_TYPE_AUDIO:
						this->audioStream = x;
						audio_st = stream;
						break;
					case AVMEDIA_TYPE_SUBTITLE:
						break;
					case AVMEDIA_TYPE_VIDEO:
						this->videoStream = x;
						video_st = stream;
						break;
					default:
						break;
					}
				}
			}

			/*  Get selected codec parameters. */
			if (!video_st) {
				throw cxxexcept::RuntimeException("Failed to find a video stream in {}.", path);
			}

			/*	*/
			if (audio_st) {
				AVCodecParameters *pAudioCodecParam = audio_st->codecpar;

				/*  Create audio clip.  */
				AVCodec *audioCodec = avcodec_find_decoder(pAudioCodecParam->codec_id);
				this->pAudioCtx = avcodec_alloc_context3(audioCodec);
				if (!this->pAudioCtx)
					throw cxxexcept::RuntimeException("Failed to create audio decode context");

				result = avcodec_parameters_to_context(this->pAudioCtx, pAudioCodecParam);
				if (result < 0) {
					char buf[AV_ERROR_MAX_STRING_SIZE];
					av_strerror(result, buf, sizeof(buf));
					throw cxxexcept::RuntimeException("Failed to set codec parameters : {}", buf);
				}

				result = avcodec_open2(this->pAudioCtx, audioCodec, nullptr);
				if (result < 0) {
					char buf[AV_ERROR_MAX_STRING_SIZE];
					av_strerror(result, buf, sizeof(buf));
					throw cxxexcept::RuntimeException("Failed to retrieve info from stream info : {}", buf);
				}

				this->audio_bit_rate = pAudioCodecParam->bit_rate;
				this->audio_sample_rate = pAudioCodecParam->sample_rate;
				this->audio_channel = pAudioCodecParam->channels;
			}

			AVCodecParameters *pVideoCodecParam = video_st->codecpar;

			/*	*/
			AVCodec *pVideoCodec = avcodec_find_decoder(pVideoCodecParam->codec_id);
			if (pVideoCodec == nullptr) {
				throw cxxexcept::RuntimeException("failed to find decoder");
			}
			/*	*/
			this->pVideoCtx = avcodec_alloc_context3(pVideoCodec);
			if (this->pVideoCtx == nullptr) {
				throw cxxexcept::RuntimeException("Failed to allocate video decoder context");
			}
			/*	*/
			result = avcodec_parameters_to_context(this->pVideoCtx, pVideoCodecParam);
			if (result < 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to set codec parameters : {}", buf);
			}
			/*	*/
			if ((result = avcodec_open2(this->pVideoCtx, pVideoCodec, nullptr)) != 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to retrieve info from stream info : {}", buf);
			}

			video_width = this->pVideoCtx->width;
			video_height = this->pVideoCtx->height;

			this->frame = av_frame_alloc();
			this->frameoutput = av_frame_alloc();

			if (this->frame == nullptr || this->frameoutput == nullptr) {
				throw cxxexcept::RuntimeException("Failed to allocate frame");
			}

			/*	*/
			size_t m_bufferSize =
				av_image_get_buffer_size(AV_PIX_FMT_RGBA, this->pVideoCtx->width, this->pVideoCtx->height, 4);
			av_image_alloc(this->frameoutput->data, this->frameoutput->linesize, this->pVideoCtx->width,
						   this->pVideoCtx->height, AV_PIX_FMT_RGBA, 4);

			/*	*/
			this->sws_ctx = sws_getContext(this->pVideoCtx->width, this->pVideoCtx->height, this->pVideoCtx->pix_fmt,
										   this->pVideoCtx->width, this->pVideoCtx->height, AV_PIX_FMT_RGBA,
										   SWS_BICUBIC, nullptr, nullptr, nullptr);

			/*	*/
			this->frame_timer = av_gettime() / 1000000.0;
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			this->audioInterface = std::make_shared<fragcore::OpenALAudioInterface>(nullptr);

			fragcore::AudioListenerDesc list_desc = {.position = fragcore::Vector3(0, 0, 0),
													 .rotation = fragcore::Quaternion::Identity()};
			list_desc.position = fragcore::Vector3::Zero();
			listener = audioInterface->createAudioListener(&list_desc);
			listener->setVolume(1.0f);
			fragcore::AudioSourceDesc source_desc = {};
			source_desc.position = fragcore::Vector3::Zero();
			audioSource = audioInterface->createAudioSource(&source_desc);

			mSource = this->audioSource->getNativePtr();
			// fragcore::AudioClipDesc clip_desc = {};
			// clip_desc.decoder = nullptr;
			// clip_desc.samples = audio_sample_rate;
			// clip_desc.sampleRate = audio_bit_rate;
			// clip_desc.format = fragcore::AudioFormat::eStero;
			// clip_desc.datamode = fragcore::AudioDataMode::Streaming;
			//
			// this->clip = audioInterface->createAudioClip(&clip_desc);
			// this->audioSource->setClip(this->clip);

			loadVideo(this->videoPath.c_str());

			/*	Load shader	*/
			const std::vector<uint32_t> vertex_source =
				IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
			const std::vector<uint32_t> fragment_source =
				IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

			/*	*/
			fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
			compilerOptions.target = fragcore::ShaderLanguage::GLSL;
			compilerOptions.glslVersion = this->getShaderVersion();

			/*  */
			this->videoplayback_program =
				ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_source, &fragment_source);

			/*	*/
			glUseProgram(this->videoplayback_program);
			glUniform1iARB(glGetUniformLocation(this->videoplayback_program, "diffuse"), 0);
			glUseProgram(0);

			/*	Create array buffer, for rendering static geometry.	*/
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);

			/*	*/
			glGenBuffers(1, &this->vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			/*	*/
			glEnableVertexAttribArrayARB(0);
			glVertexAttribPointerARB(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

			/*	*/
			glEnableVertexAttribArrayARB(1);
			glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void *>(12));

			glBindVertexArray(0);

			/*	Allocate buffers.	*/
			videoStageBufferMemorySize = video_width * video_height * 4;
			glGenBuffers(1,
						 &videoStagingTextureBuffer); // TODO ffix t oa single buffer.
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, videoStagingTextureBuffer);
			glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, videoStageBufferMemorySize * nrVideoFrames, nullptr,
						 GL_DYNAMIC_COPY);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

			/*	*/
			glGenTextures(this->videoFrameTextures.size(), this->videoFrameTextures.data());
			for (size_t i = 0; i < this->videoFrameTextures.size(); i++) {
				glBindTexture(GL_TEXTURE_2D, this->videoFrameTextures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, video_width, video_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
							 nullptr);
				/*	wrap and filter	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			}
			glBindTexture(GL_TEXTURE_2D, 0);

			glGenFramebuffers(1, &this->videoFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, this->videoFramebuffer);

			for (size_t i = 0; i < this->videoFrameTextures.size(); i++) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
									   this->videoFrameTextures[i], 0);
			}

			// const GLenum drawAttach = GL_COLOR_ATTACHMENT0;
			// glDrawBuffers(1, &drawAttach);

			/*  Validate if created properly.*/
			int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/*	*/
			alSourcePlay(this->audioSource->getNativePtr());
			// this->audioSource->play();
		}

		virtual void onResize(int width, int height) override {}

		virtual void draw() override {

			update();

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			bool blit = false;
			if (blit) {
				/*	*/
				glDisable(GL_CULL_FACE);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);

				glUseProgram(this->videoplayback_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->videoFrameTextures[nthVideoFrame]);

				/*	Draw triangle*/
				glBindVertexArray(this->vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, this->vertices.size());
				glBindVertexArray(0);
			} else {
				/*	Blit mandelbrot framebuffer to default framebuffer.	*/
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
				glBindFramebuffer(GL_READ_FRAMEBUFFER, this->videoFramebuffer);
				glReadBuffer(GL_COLOR_ATTACHMENT0 + nthVideoFrame);

				glBlitFramebuffer(0, 0, this->video_width, this->video_height, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
								  GL_LINEAR);
			}
		}

		virtual void update() {

			/*  */
			AVPacket *packet = av_packet_alloc();
			if (!packet) {
				throw cxxexcept::RuntimeException("failed to allocated memory for AVPacket");
			}

			int res, result;
			// FIXME: add support for sync.

			// res = av_seek_frame(this->pformatCtx, this->videoStream, (int64_t)(getTimer().getElapsed() * 1000000.0f),
			// 					AVSEEK_FLAG_ANY);

			res = av_read_frame(this->pformatCtx, packet);
			if (res == 0) {
				if (packet->stream_index == this->videoStream) {
					result = avcodec_send_packet(this->pVideoCtx, packet);
					if (result < 0) {
						char buf[AV_ERROR_MAX_STRING_SIZE];
						av_strerror(result, buf, sizeof(buf));
						throw cxxexcept::RuntimeException("Failed to send packet for decoding picture frame : {}", buf);
					}

					while (result >= 0) {
						result = avcodec_receive_frame(this->pVideoCtx, this->frame);
						if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
							break;
						if (result < 0) {
							char buf[AV_ERROR_MAX_STRING_SIZE];
							av_strerror(result, buf, sizeof(buf));
							throw cxxexcept::RuntimeException(" : {}", buf);
						}

						if (this->frame->format == AV_PIX_FMT_YUV420P) {

							this->frame->data[0] =
								this->frame->data[0] + this->frame->linesize[0] * (this->pVideoCtx->height - 1);
							this->frame->data[1] =
								this->frame->data[1] + this->frame->linesize[0] * this->pVideoCtx->height / 4 - 1;
							this->frame->data[2] =
								this->frame->data[2] + this->frame->linesize[0] * this->pVideoCtx->height / 4 - 1;

							this->frame->linesize[0] *= -1;
							this->frame->linesize[1] *= -1;
							this->frame->linesize[2] *= -1;
							sws_scale(this->sws_ctx, this->frame->data, this->frame->linesize, 0, this->frame->height,
									  this->frameoutput->data, this->frameoutput->linesize);

							glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, this->videoStagingTextureBuffer);
							void *uniformPointer =
								glMapBufferRange(GL_PIXEL_UNPACK_BUFFER_ARB, nthVideoFrame * videoStageBufferMemorySize,
												 videoStageBufferMemorySize,
												 GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT); // TODO fix access bit

							/*	Upload the image to staging.	*/
							memcpy(uniformPointer, this->frameoutput->data[0], videoStageBufferMemorySize);
							glFlushMappedBufferRange(GL_PIXEL_UNPACK_BUFFER_ARB, 0, videoStageBufferMemorySize);
							glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);

							glBindTexture(GL_TEXTURE_2D, this->videoFrameTextures[this->nthVideoFrame]);
							glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_width, video_height, GL_RGBA,
											GL_UNSIGNED_BYTE,
											reinterpret_cast<const void *>(videoStageBufferMemorySize * nthVideoFrame));
							glBindTexture(GL_TEXTURE_2D, 0);
							glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

							nthVideoFrame = (nthVideoFrame + 1) % nrVideoFrames;
						}
					}
				} else if (packet->stream_index == this->audioStream) {
					result = avcodec_send_packet(this->pAudioCtx, packet);
					if (result < 0) {
						char buf[AV_ERROR_MAX_STRING_SIZE];
						av_strerror(result, buf, sizeof(buf));
						throw cxxexcept::RuntimeException("Failed to send packet for decoding audio frame : {}", buf);
					}

					while (result >= 0) {
						result = avcodec_receive_frame(this->pAudioCtx, this->frame);
						if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
							break;
						}
						if (result < 0) {
							char buf[AV_ERROR_MAX_STRING_SIZE];
							av_strerror(result, buf, sizeof(buf));
							throw cxxexcept::RuntimeException(" : {}", buf);
						}
						/*	*/
						int data_size = av_get_bytes_per_sample(pAudioCtx->sample_fmt);

						/*	*/
						this->frame->linesize[0];
						av_get_channel_layout_nb_channels(this->frame->channel_layout);
						this->frame->format != AV_SAMPLE_FMT_S16P;
						this->frame->channel_layout;

						// TODO queue info
						// this->clip->
						printf("%d %d\n", this->frame->linesize[0], this->frame->nb_samples);

						/*	Assign new audio data.	*/
						for (int i = 0; i < frame->nb_samples; i++) {
							for (int ch = 0; ch < pAudioCtx->channels; ch++) {

								ALint processed, queued;
								alGetSourcei((ALuint)this->audioSource->getNativePtr(), AL_BUFFERS_PROCESSED,
											 &processed);
								// clip->setData(this->frame->data[0], data_size, 0);

								// alBufferData(bufid, mFormat, samples.get(), buffer_len, mCodecCtx->sample_rate);
								// alSourceQueueBuffers(mSource, 1, &bufid);
								continue;
							}
						}
						/* Check that the source is playing. */
						int state;
						alGetSourcei(mSource, AL_SOURCE_STATE, &state);
						if (state == AL_STOPPED) {
							alSourceRewind(mSource);
							alSourcei(mSource, AL_BUFFER, 0);
							// if (alcGetInteger64vSOFT) {
							//	/* Also update the device start time with the current
							//	 * device clock, so the decoder knows we're running behind.
							//	 */
							//	int64_t devtime{};
							//	alcGetInteger64vSOFT(alcGetContextsDevice(alcGetCurrentContext()),
							//						 ALC_DEVICE_CLOCK_SOFT, 1, &devtime);
							//	mDeviceStartTime = nanoseconds{devtime} - mCurrentPts;
							//}
						}
						// this->audioSource->play();
					}
				}
			}
			av_packet_unref(packet);
			av_packet_free(&packet);
		}
	};

	class VideoPlaybackGLSample : public GLSample<VideoPlayback> {
	  public:
		VideoPlaybackGLSample(int argc, const char **argv) : GLSample<VideoPlayback>(argc, argv) {}
		virtual void commandline(cxxopts::Options &options) override {
			options.add_options("VideoPlayback-Sample")("v,video", "Video Path",
														cxxopts::value<std::string>()->default_value("video.mp4"));
		}
	};
} // namespace glsample

// TODO add custom options

int main(int argc, const char **argv) {
	try {
		GLSample<glsample::VideoPlayback> sample(argc, argv);

		sample.run();

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}