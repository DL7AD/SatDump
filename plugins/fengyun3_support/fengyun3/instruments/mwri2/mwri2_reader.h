#pragma once

#include <cstdint>
#include "common/image/image.h"
#include <vector>
#include "common/resizeable_buffer.h"

namespace fengyun3
{
    namespace mwri2
    {
        class MWRI2Reader
        {
        private:
            std::vector<unsigned short> channels[26];

        public:
            MWRI2Reader();
            ~MWRI2Reader();
            int lines;
            void work(std::vector<uint8_t> &packet);
            image::Image getChannel(int channel);

            std::vector<double> timestamps;
        };
    } // namespace virr
} // namespace fengyun