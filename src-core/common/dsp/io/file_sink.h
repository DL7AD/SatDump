#pragma once

#include "common/dsp/block.h"
#include "file_source.h"
#ifdef BUILD_ZIQ
#include "common/ziq.h"
#endif
#include "wav_writer.h"

#include "common/ziq2.h"

#include "logger.h"

namespace dsp
{
    class FileSinkBlock : public Block<complex_t, float>
    {
    private:
        std::mutex rec_mutex;

        BasebandType d_sample_format;
        void work();

        bool should_work = false;

        std::ofstream output_file;

        uint64_t current_size_out = 0;
        uint64_t current_size_out_raw = 0;

        int8_t *buffer_s8;
        int16_t *buffer_s16;

#ifdef BUILD_ZIQ
        ziq::ziq_cfg ziqcfg;
        std::shared_ptr<ziq::ziq_writer> ziqWriter;
#endif

        float *mag_buffer = nullptr;

        std::unique_ptr<WavWriter> wav_writer;

    public:
        FileSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input);
        ~FileSinkBlock();

        void set_output_sample_type(BasebandType sample_format)
        {
            d_sample_format = sample_format;
        }

        std::string start_recording(std::string path_without_ext, uint64_t samplerate, bool override_filename = false)
        {
            rec_mutex.lock();

            std::string finalt;
            finalt = path_without_ext + "." + (std::string)d_sample_format;
            if (override_filename)
                finalt = path_without_ext;

            current_size_out = 0;
            current_size_out_raw = 0;

            output_file = std::ofstream(finalt, std::ios::binary);

            if (d_sample_format == WAV_16)
            {
                wav_writer = std::make_unique<WavWriter>(output_file);
                wav_writer->write_header(samplerate, 2);
            }

#ifdef BUILD_ZIQ
            if (d_sample_format == ZIQ)
            {
                ziqcfg.is_compressed = true;
                ziqcfg.bits_per_sample = d_sample_format.ziq_depth;
                ziqcfg.samplerate = samplerate;
                ziqcfg.annotation = "";

                ziqWriter = std::make_shared<ziq::ziq_writer>(ziqcfg, output_file);
            }
#endif
#ifdef BUILD_ZIQ2
            if (d_sample_format == ZIQ2)
            {
                int sz = ziq2::ziq2_write_file_hdr((uint8_t *)buffer_s8, samplerate);
                output_file.write((char *)buffer_s8, sz);

                if (mag_buffer == nullptr)
                    mag_buffer = create_volk_buffer<float>(STREAM_BUFFER_SIZE);
            }
#endif

            if (!std::filesystem::exists(finalt))
                logger->error("We have created the output baseband file, but it does not exist! There may be a permission issue! File : " + finalt);

            should_work = true;
            rec_mutex.unlock();

            return finalt;
        }

        uint64_t get_written()
        {
            return current_size_out;
        }

        uint64_t get_written_raw()
        {
            return current_size_out_raw;
        }

        void stop_recording()
        {
            if (d_sample_format == WAV_16)
                wav_writer->finish_header(get_written());

            rec_mutex.lock();
            should_work = false;
            current_size_out = 0;
            current_size_out_raw = 0;
            output_file.close();
            rec_mutex.unlock();
        }
    };
}