#include "Common.h"
#include "GLSampleSession.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <GL/glew.h>
#include <GLSample.h>
#include <GLSampleWindow.h>
#include <OpenALAudioInterface.h>
#include <ShaderLoader.h>
#include <Util/CameraController.h>
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <glm/glm.hpp>
#include <iostream>
#include <thread>

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
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 28, 1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

namespace glsample {

	/**
	 * @brief
	 *
	 */
	class VideoPlayback : public GLSampleWindow {
	  public:
		VideoPlayback() : GLSampleWindow() {
			this->videoplaybackSettingComponent = std::make_shared<VideoPlaybackSettingComponent>();
			this->addUIComponent(this->videoplaybackSettingComponent);
		}

		static const size_t nrVideoFrames = 2;
		int nthVideoFrame = 0;
		int frameSize = 0;

		/*  */
		struct AVFormatContext *pformatCtx = nullptr;
		struct AVCodecContext *pVideoCtx = nullptr;
		struct AVCodecContext *pAudioCtx = nullptr;

		/*  */
		int videoStream{};
		int audioStream{};
		size_t video_width{};
		size_t video_height{};

		size_t audio_sample_rate{};
		size_t audio_bit_rate{};
		size_t audio_channels{};

		/*  */
		struct AVFrame *frame = nullptr;
		struct AVFrame *frameoutput = nullptr;
		struct AVStream *video_st = nullptr;
		struct AVStream *audio_st = nullptr;
		struct SwsContext *sws_ctx = nullptr;
		struct SwrContext *swrContext = nullptr;
		uint8_t **destBuffer = nullptr;

		int destBufferLinesize{};
		unsigned int flag{};
		double video_clock{};
		double frame_timer{};
		double frame_last_pts{};
		double frame_last_delay{};

		/*  */
		unsigned int videoFramebuffer{};
		unsigned int vbo{};
		unsigned vao{};
		MeshObject plane;

		/*	Audio.	*/ // TODO fix
		fragcore::AudioClip *clip{};
		fragcore::AudioListener *listener{};
		fragcore::AudioSource *audioSource{};
		std::shared_ptr<fragcore::OpenALAudioInterface> audioInterface;

		unsigned int mSource{};
		std::vector<unsigned int> mAudioBuffers;
		unsigned int bufferIndex = 0;

		/*  */
		unsigned int videoplayback_program{};

		/*  */
		size_t videoStageBufferMemorySize = 0;
		std::array<unsigned int, nrVideoFrames> videoFrameTextures{};
		std::array<void *, nrVideoFrames> videoMapBuffer{};
		unsigned int videoStagingTextureBuffer{}; // PBO buffers

		class VideoPlaybackSettingComponent : public nekomimi::UIComponent {

		  public:
			VideoPlaybackSettingComponent() { this->setName("VideoPlayback Settings"); }
			void draw() override {
				ImGui::DragFloat("Speed", &this->speed);
				ImGui::DragFloat("Volume", &this->volume);
				ImGui::Checkbox("Use Blit", &this->useBlit);
			}

			bool showWireFrame = false;
			float speed = 1.0f;
			float volume = 1.0f;
			bool useBlit = true;
		};
		std::shared_ptr<VideoPlaybackSettingComponent> videoplaybackSettingComponent;

		/*	*/
		const std::string vertexShaderPath = "Shaders/postprocessingeffects/postprocessing.vert.spv";
		const std::string fragmentShaderPath = "Shaders/postprocessingeffects/overlay.frag.spv";

		std::string error_message(const int result) {
			char buf[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(result, buf, sizeof(buf));
			return buf;
		}

		void Release() override {
			/*	Release Video Data.	*/
			if (this->frame) {
				av_frame_free(&this->frame);
			}
			avcodec_free_context(&this->pAudioCtx);
			avcodec_free_context(&this->pVideoCtx);

			avformat_close_input(&this->pformatCtx);
			avformat_free_context(this->pformatCtx);
			/*	*/
			glDeleteProgram(this->videoplayback_program);
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);

			/*	*/
			glDeleteFramebuffers(1, &this->videoFramebuffer);

			/*	*/
			glDeleteBuffers(1, &videoStagingTextureBuffer);
			glDeleteTextures(this->videoFrameTextures.size(), this->videoFrameTextures.data());
		}

		void loadVideo(const char *path) {
			int result = 0;

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

			/*	Audio Configuration.	*/
			{
				/*	*/
				if (audio_st) {
					AVCodecParameters *pAudioCodecParam = audio_st->codecpar;

					/*  Create audio clip.  */
					const AVCodec *audioCodec = avcodec_find_decoder(pAudioCodecParam->codec_id);
					this->pAudioCtx = avcodec_alloc_context3(audioCodec);
					if (!this->pAudioCtx) {
						throw cxxexcept::RuntimeException("Failed to create audio decode context");
					}

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
					this->audio_channels = pAudioCodecParam->channels;
				}

				AVCodecParameters *pVideoCodecParam = video_st->codecpar;

				const AVCodec *pVideoCodec = avcodec_find_decoder(pVideoCodecParam->codec_id);
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

				this->video_width = this->pVideoCtx->width;
				this->video_height = this->pVideoCtx->height;

				this->frame = av_frame_alloc();
				this->frameoutput = av_frame_alloc();

				if (this->frame == nullptr || this->frameoutput == nullptr) {
					throw cxxexcept::RuntimeException("Failed to allocate frame");
				}

				/*	*/
				const size_t m_bufferSize =
					av_image_get_buffer_size(AV_PIX_FMT_RGBA, this->pVideoCtx->width, this->pVideoCtx->height, 4);
				av_image_alloc(this->frameoutput->data, this->frameoutput->linesize, this->pVideoCtx->width,
							   this->pVideoCtx->height, AV_PIX_FMT_RGBA, 4);

				/*	*/
				this->sws_ctx = sws_getContext(
					this->pVideoCtx->width, this->pVideoCtx->height, this->pVideoCtx->pix_fmt, this->pVideoCtx->width,
					this->pVideoCtx->height, AV_PIX_FMT_RGBA, SWS_BICUBIC, nullptr, nullptr, nullptr);
				if (this->sws_ctx == nullptr) {
				}
				// Initialize SWR context
				this->swrContext = swr_alloc_set_opts(nullptr, pAudioCtx->channel_layout, AV_SAMPLE_FMT_FLT,
													  pAudioCtx->sample_rate, pAudioCtx->channel_layout,
													  pAudioCtx->sample_fmt, pAudioCtx->sample_rate, 0, nullptr);
				if ((result = swr_init(swrContext)) != 0) {
					char buf[AV_ERROR_MAX_STRING_SIZE];
					av_strerror(result, buf, sizeof(buf));
					throw cxxexcept::RuntimeException("Failed to init SWR : {}", buf);
				}

				result =
					av_samples_alloc_array_and_samples(&destBuffer, &destBufferLinesize, 2, 4096, AV_SAMPLE_FMT_FLT, 0);
				if (result < 0) {
					char buf[AV_ERROR_MAX_STRING_SIZE];
					av_strerror(result, buf, sizeof(buf));
					throw cxxexcept::RuntimeException("Failed to allocate ({}) : {}", result, buf);
				}
			}

			/*	*/
			this->frame_timer = av_gettime() / 1000000.0;

			this->setColorSpace(ColorSpace::RawLinear);
		}

		void Initialize() override {

			/*	*/
			const std::string videoPath = this->getResult()["video"].as<std::string>();
			this->setTitle(fmt::format("VideoPlayback: {}", videoPath));

			/*	*/
			{
				this->audioInterface = std::make_shared<fragcore::OpenALAudioInterface>(nullptr);

				fragcore::AudioListenerDesc list_desc = {fragcore::Vector3(0, 0, 0), fragcore::Quaternion::Identity()};
				list_desc.position = fragcore::Vector3::Zero();
				listener = audioInterface->createAudioListener(&list_desc);
				listener->setVolume(1.0f);

				fragcore::AudioSourceDesc source_desc = {};
				source_desc.position = fragcore::Vector3::Zero();

				this->audioSource = audioInterface->createAudioSource(&source_desc);
				this->audioSource->loop(false);
				this->audioSource->setVolume(1.0f);
				this->mSource = this->audioSource->getNativePtr();

				this->mAudioBuffers.resize(8);
				FAOPAL_VALIDATE(alGenBuffers(this->mAudioBuffers.size(), this->mAudioBuffers.data()));
			}

			this->loadVideo(videoPath.c_str());

			{
				/*	Load shader	*/
				const std::vector<uint32_t> vertex_binary =
					IOUtil::readFileData<uint32_t>(this->vertexShaderPath, this->getFileSystem());
				const std::vector<uint32_t> fragment_binary =
					IOUtil::readFileData<uint32_t>(this->fragmentShaderPath, this->getFileSystem());

				/*	*/
				fragcore::ShaderCompiler::CompilerConvertOption compilerOptions;
				compilerOptions.target = fragcore::ShaderLanguage::GLSL;
				compilerOptions.glslVersion = this->getShaderVersion();

				/*  */
				this->videoplayback_program =
					ShaderLoader::loadGraphicProgram(compilerOptions, &vertex_binary, &fragment_binary);
			}

			/*	Setup graphic pipeline.	*/
			glUseProgram(this->videoplayback_program);
			glUniform1i(glGetUniformLocation(this->videoplayback_program, "diffuse"), 0);
			glUseProgram(0);

			/*	Create array buffer, for rendering static geometry.	*/
			// Common::loadPlan(this->plan,1);
			glGenVertexArrays(1, &this->vao);
			glBindVertexArray(this->vao);
			glBindVertexArray(0);

			/*	Allocate buffers.	*/
			const size_t pixelSize = 4;
			this->videoStageBufferMemorySize = this->video_width * this->video_height * pixelSize;
			glGenBuffers(1, &videoStagingTextureBuffer);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, this->videoStagingTextureBuffer);
			glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB,
						 this->videoStageBufferMemorySize * glsample::VideoPlayback::nrVideoFrames, nullptr,
						 GL_DYNAMIC_COPY);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

			/*	Create round robin texture array.	*/
			glGenTextures(this->videoFrameTextures.size(), this->videoFrameTextures.data());
			for (size_t i = 0; i < this->videoFrameTextures.size(); i++) {
				glBindTexture(GL_TEXTURE_2D, this->videoFrameTextures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->video_width, this->video_height, 0, GL_RGBA,
							 GL_UNSIGNED_BYTE, nullptr);
				/*	wrap and filter	*/
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				/*	Disable LOD.	*/
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

			/*  Validate if created properly.*/
			const int frameStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (frameStatus != GL_FRAMEBUFFER_COMPLETE) {
				throw RuntimeException("Failed to create framebuffer, {}", frameStatus);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
		}

		void onResize(int width, int height) override {}

		void draw() override {

			int width = 0, height = 0;
			this->getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			if (!this->videoplaybackSettingComponent->useBlit) {
				/*	*/
				glDisable(GL_CULL_FACE);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);

				glUseProgram(this->videoplayback_program);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, this->videoFrameTextures[nthVideoFrame]);

				/*	Draw triangle*/
				glBindVertexArray(this->vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glBindVertexArray(0);

			} else {
				/*	Blit video framebuffer to default framebuffer.	*/
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->getDefaultFramebuffer());
				glBindFramebuffer(GL_READ_FRAMEBUFFER, this->videoFramebuffer);
				glReadBuffer(GL_COLOR_ATTACHMENT0 + this->nthVideoFrame);

				glBlitFramebuffer(0, 0, this->video_width, this->video_height, 0, 0, width, height, GL_COLOR_BUFFER_BIT,
								  GL_LINEAR);
				glBindFramebuffer(GL_FRAMEBUFFER, this->getDefaultFramebuffer());
			}
		}

		void update() override {

			/*  */
			AVPacket pkt = {nullptr};
			AVPacket *packet = av_packet_alloc();
			if (!packet) {
				throw cxxexcept::RuntimeException("failed to allocated memory for AVPacket");
			}

			int res = 0, result = 0;

			res = av_read_frame(this->pformatCtx, packet);

			if (res == 0) {

				/*	*/
				if (packet->stream_index == this->videoStream) {

					result = avcodec_send_packet(this->pVideoCtx, packet);
					if (result < 0) {
						throw cxxexcept::RuntimeException("Failed to send packet for decoding image frame : {}",
														  error_message(result));
					}

					while (result >= 0) {
						result = avcodec_receive_frame(this->pVideoCtx, this->frame);
						if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
							this->getLogger().debug("Failed to recv videoframe {}", error_message(result));
							continue;
						}
						if (result < 0) {
							throw cxxexcept::RuntimeException(" : {}", error_message(result));
						}

						if (this->frame->format == AV_PIX_FMT_YUV420P) {

							/*	*/
							this->frame->data[0] =
								this->frame->data[0] +
								static_cast<ptrdiff_t>(this->frame->linesize[0] * (this->pVideoCtx->height - 1));
							this->frame->data[1] =
								this->frame->data[1] + this->frame->linesize[0] * this->pVideoCtx->height / 4 - 1;
							this->frame->data[2] =
								this->frame->data[2] + this->frame->linesize[0] * this->pVideoCtx->height / 4 - 1;

							/*	*/
							this->frame->linesize[0] *= -1;
							this->frame->linesize[1] *= -1;
							this->frame->linesize[2] *= -1;

							sws_scale(this->sws_ctx, this->frame->data, this->frame->linesize, 0, this->frame->height,
									  this->frameoutput->data, this->frameoutput->linesize);

							/*	*/
							glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, this->videoStagingTextureBuffer);
							void *uniformPointer = glMapBufferRange(
								GL_PIXEL_UNPACK_BUFFER_ARB, this->nthVideoFrame * this->videoStageBufferMemorySize,
								this->videoStageBufferMemorySize,
								GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

							/*	Upload the image to staging.	*/
							memcpy(uniformPointer, this->frameoutput->data[0], this->videoStageBufferMemorySize);
							glFlushMappedBufferRange(GL_PIXEL_UNPACK_BUFFER_ARB, 0, this->videoStageBufferMemorySize);
							glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);

							glBindTexture(GL_TEXTURE_2D, this->videoFrameTextures[this->nthVideoFrame]);
							glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->video_width, this->video_height, GL_RGBA,
											GL_UNSIGNED_BYTE, nullptr);
							glBindTexture(GL_TEXTURE_2D, 0);
							glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

							this->nthVideoFrame = (this->nthVideoFrame + 1) % glsample::VideoPlayback::nrVideoFrames;
						}
					}
				} else if (packet->stream_index == this->audioStream) {

					result = avcodec_send_packet(this->pAudioCtx, packet);
					if (result < 0) {
						throw cxxexcept::RuntimeException("Failed to send packet for decoding audio frame : {}",
														  error_message(result));
					}

					/*	*/
					while (result >= 0) {
						result = avcodec_receive_frame(this->pAudioCtx, this->frame);
						if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
							this->getLogger().debug("Failed to recv audio frame {}", error_message(result));
							continue;
						}
						if (result < 0) {
							throw cxxexcept::RuntimeException(" : {}", error_message(result));
						}

						/*	*/
						ALenum alFormat = 0;
						if (this->frame->format == AV_SAMPLE_FMT_U8) {
							alFormat = AL_FORMAT_STEREO8;
						} else if (this->frame->format == AV_SAMPLE_FMT_S16) {
							alFormat = AL_FORMAT_STEREO16;
						} else if (this->frame->format == AV_SAMPLE_FMT_FLT) {
							alFormat = AL_FORMAT_STEREO_FLOAT32;
						}

						const int outputSamples =
							swr_convert(this->swrContext, this->destBuffer, destBufferLinesize,
										(const uint8_t **)frame->extended_data, frame->nb_samples);

						/*	*/
						const size_t channels = 2;
						const int bufferSize =
							av_get_bytes_per_sample((AVSampleFormat)this->frame->format) * channels * outputSamples;
						alFormat = AL_FORMAT_STEREO_FLOAT32;

						ALint processed = 0, queued = 0, currentBuffer = 0;
						FAOPAL_VALIDATE(alGetSourcei((ALuint)this->mSource, AL_BUFFERS_PROCESSED, &processed));
						FAOPAL_VALIDATE(alGetSourcei((ALuint)this->mSource, AL_BUFFERS_QUEUED, &queued));

						while (processed > 0) {
							ALuint bid = 0;
							FAOPAL_VALIDATE(alSourceUnqueueBuffers(this->mSource, 1, &bid));
							--processed;
						}

						if (static_cast<ALuint>(queued) < this->mAudioBuffers.size()) {

							this->getLogger().info("{} {} {}", bufferSize, frame->sample_rate, alFormat);

							const ALuint current_audio_buffer = {this->mAudioBuffers[this->bufferIndex]};
							FAOPAL_VALIDATE(alBufferData(current_audio_buffer, alFormat, this->destBuffer[0],
														 bufferSize, frame->sample_rate));

							FAOPAL_VALIDATE(alSourceQueueBuffers(this->mSource, 1, &current_audio_buffer));

							this->bufferIndex = (this->bufferIndex + 1) % this->mAudioBuffers.size();

							/* Check that the source is playing. */
							int state = 0;
							FAOPAL_VALIDATE(alGetSourcei(this->mSource, AL_SOURCE_STATE, &state));
							if (state == AL_STOPPED) {
								alSourceRewind(this->mSource);

								FAOPAL_VALIDATE(alSourcePlay(this->mSource));
							}
						}

						/*	*/
						ALint playStatus = 0;
						FAOPAL_VALIDATE(alGetSourcei(this->mSource, AL_SOURCE_STATE, &playStatus));
						if (playStatus != AL_PLAYING) {
							FAOPAL_VALIDATE(alSourcePlay(this->mSource));
						}
					}
				}
			} else {
				this->getLogger().debug("Failed to read package {}", error_message(res));
			}

			av_packet_unref(packet);
			av_packet_free(&packet);

			// res = av_seek_frame(this->pformatCtx, -1, timestamp, AVSEEK_FLAG_ANY);
			if (res < 0) {
				this->getLogger().debug("Failed to seek {}", error_message(res));
				return;
			}

			/*	*/
			this->listener->setVolume(this->videoplaybackSettingComponent->volume);

			std::this_thread::sleep_for(8ms);
		}

	}; // namespace glsample

	class VideoPlaybackGLSample : public GLSample<VideoPlayback> {
	  public:
		VideoPlaybackGLSample() : GLSample<VideoPlayback>() {}
		void customOptions(cxxopts::OptionAdder &options) override {
			options("V,video", "Video Path", cxxopts::value<std::string>()->default_value("assets/video.mp4"));
		}
	};
} // namespace glsample

int main(int argc, const char **argv) {
	try {
		glsample::VideoPlaybackGLSample sample;
		sample.run(argc, argv);

	} catch (const std::exception &ex) {

		std::cerr << cxxexcept::getStackMessage(ex) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}