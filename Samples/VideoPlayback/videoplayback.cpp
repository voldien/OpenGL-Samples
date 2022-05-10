#include <GL/glew.h>
#include <GLSampleWindow.h>
#include <GLWindow.h>
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

namespace glsample {

	class VideoPlayback : public GLWindow {
	  public:
		VideoPlayback() : GLWindow(-1, -1, -1, -1) {
			this->setTitle(fmt::format("VideoPlayback {}", this->videoPath).c_str());
		}
		virtual ~VideoPlayback() {
			if (this->frame)
				av_frame_free(&this->frame);
			avcodec_free_context(&this->pAudioCtx);
			avcodec_free_context(&this->pVideoCtx);

			avformat_close_input(&this->pformatCtx);
			avformat_free_context(this->pformatCtx);
		}

		typedef struct _vertex_t {
			float pos[2];
			float color[3];
		} Vertex;

		static const int nrVideoFrames = 3;
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
		unsigned int vbo;
		unsigned vao;

		/*  */
		unsigned int videoplayback_program;

		/*  */
		std::array<unsigned int, nrVideoFrames> videoFrames;
		std::array<unsigned int, nrVideoFrames> videoStagingFrames; // PBO buffers
		std::array<void *, nrVideoFrames> mapMemory;

		std::string videoPath = "video.mp4";

		const std::string vertexShaderPath = "Shaders/videoplayback/videoplayback.vert";
		const std::string fragmentShaderPath = "Shaders/videoplayback/videoplayback.frag";

		const std::vector<Vertex> vertices = {{-1.0f, -1.0f, -1.0f, 0, 0}, // triangle 1 : begin
											  {-1.0f, -1.0f, 1.0f, 0, 1},
											  {-1.0f, 1.0f, 1.0f, 1, 1}, // triangle 1 : end
											  {1.0f, 1.0f, -1.0f, 1, 1}, // triangle 2 : begin
											  {-1.0f, -1.0f, -1.0f, 1, 0},
											  {-1.0f, 1.0f, -1.0f, 0, 0}, // triangle 2 : end
											  {1.0f, -1.0f, 1.0f, 0, 0},
											  {-1.0f, -1.0f, -1.0f, 0, 1},
											  {1.0f, -1.0f, -1.0f, 1, 1},
											  {1.0f, 1.0f, -1.0f, 0, 0},
											  {1.0f, -1.0f, -1.0f, 1, 1},
											  {-1.0f, -1.0f, -1.0f, 1, 0},
											  {-1.0f, -1.0f, -1.0f, 0, 0},
											  {-1.0f, 1.0f, 1.0f, 0, 1},
											  {-1.0f, 1.0f, -1.0f, 1, 1},
											  {1.0f, -1.0f, 1.0f, 0, 0},
											  {-1.0f, -1.0f, 1.0f, 1, 1},
											  {-1.0f, -1.0f, -1.0f, 0, 1},
											  {-1.0f, 1.0f, 1.0f, 0, 0},
											  {-1.0f, -1.0f, 1.0f, 0, 1},
											  {1.0f, -1.0f, 1.0f, 1, 1},
											  {1.0f, 1.0f, 1.0f, 0, 0},
											  {1.0f, -1.0f, -1.0f, 1, 1},
											  {1.0f, 1.0f, -1.0f, 1, 0},
											  {1.0f, -1.0f, -1.0f, 0, 0},
											  {1.0f, 1.0f, 1.0f, 0, 1},
											  {1.0f, -1.0f, 1.0f, 1, 1},
											  {1.0f, 1.0f, 1.0f, 0, 0},
											  {1.0f, 1.0f, -1.0f, 1, 1},
											  {-1.0f, 1.0f, -1.0f, 0, 1},
											  {1.0f, 1.0f, 1.0f, 0, 0},
											  {-1.0f, 1.0f, -1.0f, 0, 1},
											  {-1.0f, 1.0f, 1.0f, 1, 1},
											  {1.0f, 1.0f, 1.0f, 0, 0},
											  {-1.0f, 1.0f, 1.0f, 1, 1},
											  {1.0f, -1.0f, 1.0f, 1, 0}

		};

		virtual void Release() override {
			glDeleteProgram(this->videoplayback_program);
			glDeleteVertexArrays(1, &this->vao);
			glDeleteBuffers(1, &this->vbo);
			// TODO
			// glDeleteTextures(1, &this->skybox_panoramic);
		}

		virtual void onResize(int width, int height) override {}

		void loadVideo(const char *path) {
			int result;

			this->pformatCtx = avformat_alloc_context();
			if (!pformatCtx) {
				throw cxxexcept::RuntimeException("Failed to allocate memory for the 'AVFormatContext'");
			}
			// Determine the input-format:
			this->pformatCtx->iformat = av_find_input_format(path);

			result = avformat_open_input(&this->pformatCtx, path, nullptr, nullptr);
			if (result != 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to open input : %s", buf);
			}

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
			if (!video_st)
				throw cxxexcept::RuntimeException("Failed to find a video stream in {}.", path);

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
			if (pVideoCodec == nullptr)
				throw cxxexcept::RuntimeException("failed to find decoder");
			this->pVideoCtx = avcodec_alloc_context3(pVideoCodec);
			if (this->pVideoCtx == nullptr)
				throw cxxexcept::RuntimeException("Failed to allocate video decoder context");

			// AV_PIX_FMT_FLAG_RGB
			/*  Modify the target pixel format. */
			// this->pVideoCtx->get_format = get_format;
			//	pVideoCodecParam->format = AV_PIX_FMT_BGR24;
			//	pVideoCodecParam->codec_tag = avcodec_pix_fmt_to_codec_tag(AV_PIX_FMT_BGR24);
			//	pVideoCodecParam->color_space = AVCOL_SPC_RGB;
			result = avcodec_parameters_to_context(this->pVideoCtx, pVideoCodecParam);
			if (result < 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to set codec parameters : {}", buf);
			}
			// av_find_best_pix_fmt_of_2
			// avcodec_default_get_format()

			if ((result = avcodec_open2(this->pVideoCtx, pVideoCodec, nullptr)) != 0) {
				char buf[AV_ERROR_MAX_STRING_SIZE];
				av_strerror(result, buf, sizeof(buf));
				throw cxxexcept::RuntimeException("Failed to retrieve info from stream info : {}", buf);
			}

			video_width = this->pVideoCtx->width;
			video_height = this->pVideoCtx->height;

			this->frame = av_frame_alloc();
			this->frameoutput = av_frame_alloc();

			if (this->frame == nullptr || this->frameoutput == nullptr)
				throw cxxexcept::RuntimeException("Failed to allocate frame");

			size_t m_bufferSize =
				av_image_get_buffer_size(AV_PIX_FMT_RGBA, this->pVideoCtx->width, this->pVideoCtx->height, 4);
			av_image_alloc(this->frameoutput->data, this->frameoutput->linesize, this->pVideoCtx->width,
						   this->pVideoCtx->height, AV_PIX_FMT_RGBA, 4);

			// AVPacket *pPacket = av_packet_alloc();
			this->sws_ctx = sws_getContext(this->pVideoCtx->width, this->pVideoCtx->height, this->pVideoCtx->pix_fmt,
										   this->pVideoCtx->width, this->pVideoCtx->height, AV_PIX_FMT_RGBA,
										   SWS_BICUBIC, nullptr, nullptr, nullptr);

			this->frame_timer = av_gettime() / 1000000.0;
		}

		virtual void Initialize() override {
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

			/*	Load shader	*/
			std::vector<char> vertex_source = IOUtil::readFile(vertexShaderPath);
			std::vector<char> fragment_source = IOUtil::readFile(fragmentShaderPath);

			/*  */
			this->videoplayback_program = ShaderLoader::loadGraphicProgram(&vertex_source, &fragment_source);

			/*	*/
			glUseProgram(this->videoplayback_program);
			glUniform1iARB(glGetUniformLocation(this->videoplayback_program, "panorama"), 0);
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
			// glVertexAttribPointerARB(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), 12);

			glBindVertexArray(0);

			/*	Allocate buffers.	*/
			size_t pixelSize = 0;
			glGenBuffers(this->videoStagingFrames.size(),
						 this->videoStagingFrames.data()); // TODO ffix t oa single buffer.
			glGenTextures(this->videoFrames.size(), this->videoFrames.data());
			for (size_t i = 0; i < this->videoStagingFrames.size(); i++) {
				glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, this->videoStagingFrames[i]);
				glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, pixelSize, nullptr, GL_STREAM_COPY);
				glBindTexture(GL_TEXTURE_2D, this->videoFrames[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width(), height(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

				mapMemory[i] = glMapBufferRange(GL_PIXEL_PACK_BUFFER_ARB, 0, pixelSize,
												GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT); // TODO fix access bit
			}
		}

		virtual void draw() override {

			update();

			int width, height;
			getSize(&width, &height);

			/*	*/
			glViewport(0, 0, width, height);

			/*	*/
			glDisable(GL_CULL_FACE);
			// glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glEnable(GL_STENCIL);

			glUseProgram(this->videoplayback_program);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, this->videoplayback_program);

			/*	Draw triangle*/
			glBindVertexArray(this->vao);
			glDrawArrays(GL_TRIANGLES, 0, this->vertices.size());
			glBindVertexArray(0);

			/*  */
			this->nthVideoFrame = (this->nthVideoFrame + 1) % this->nrVideoFrames;
		}

		virtual void update() {

			/*  */
			AVPacket *packet = av_packet_alloc();
			if (!packet) {
				throw cxxexcept::RuntimeException("failed to allocated memory for AVPacket");
			}

			int res, result;
			// res = av_seek_frame(this->pformatCtx, this->videoStream, 60000, AVSEEK_FLAG_FRAME);

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

							/*	Upload the image to staging.	*/
							size_t pixelSize = video_width * video_height * 4;
							memcpy(mapMemory[nthVideoFrame], this->frameoutput->data[0],
								   video_width * video_height * 4);
							glFlushMappedBufferRange(GL_PIXEL_PACK_BUFFER_ARB, nthVideoFrame * pixelSize, pixelSize);
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
						if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
							break;
						if (result < 0) {
							char buf[AV_ERROR_MAX_STRING_SIZE];
							av_strerror(result, buf, sizeof(buf));
							throw cxxexcept::RuntimeException(" : {}", buf);
						}
						int data_size = av_get_bytes_per_sample(pAudioCtx->sample_fmt);

						av_get_channel_layout_nb_channels(this->frame->channel_layout);
						this->frame->format != AV_SAMPLE_FMT_S16P;
						this->frame->channel_layout;

						/*	Assign new audio data.	*/
						for (int i = 0; i < frame->nb_samples; i++)
							for (int ch = 0; ch < pAudioCtx->channels; ch++)
								continue;
						// clip->setData(this->frame->data[0], data_size, 0);
					}
					// this->audioSource->play();
				}
			}
			av_packet_unref(packet);
			av_packet_free(&packet);
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