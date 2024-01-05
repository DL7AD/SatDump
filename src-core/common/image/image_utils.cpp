#include "image_utils.h"

namespace image
{
    image::Image<uint16_t> blend_images(image::Image<uint16_t> &img1, image::Image<uint16_t> &img2)
    {
        size_t width = std::min<int>(img1.width(), img2.width());
        size_t height = std::min<int>(img1.height(), img2.height());
        image::Image<uint16_t> img_b(width, height, img1.channels());
        bool are_rgba = img1.channels() == 4 && img2.channels() == 4;

        for (int c = 0; c < img1.channels(); c++)
        {
            if (are_rgba)
            {
                for (size_t i = 0; i < height * width; i++)
                {
                    if (img1.channel(3)[i] == 0)
                    {
                        img_b.channel(c)[i] = img2.channel(c)[i];
                        img_b.channel(3)[i] = 65535;
                    }
                    else if (img2.channel(3)[i] == 0)
                    {
                        img_b.channel(c)[i] = img1.channel(c)[i];
                        img_b.channel(3)[i] = 65535;
                    }
                    else
                    {
                        img_b.channel(c)[i] = c == 3 ? 65535 : ((size_t(img1.channel(c)[i]) + size_t(img2.channel(c)[i])) / 2);
                        img_b.channel(3)[i] = 65535;
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < height * width; i++)
                {
                    if ((img1.channels() == 3 ? (uint64_t)img1.channel(0)[i] + (uint64_t)img1.channel(1)[i] + (uint64_t)img1.channel(2)[i] : img1.channel(c)[i]) == 0)
                        img_b.channel(c)[i] = img2.channel(c)[i];
                    else if ((img2.channels() == 3 ? (uint64_t)img2.channel(0)[i] + (uint64_t)img2.channel(1)[i] + (uint64_t)img2.channel(2)[i] : img2.channel(c)[i]) == 0)
                        img_b.channel(c)[i] = img1.channel(c)[i];
                    else
                        img_b.channel(c)[i] = (size_t(img1.channel(c)[i]) + size_t(img2.channel(c)[i])) / 2;
                }
            }
        }
        return img_b;
    }

    image::Image<uint16_t> merge_images_opacity(image::Image<uint16_t> &img1, image::Image<uint16_t> &img2, float op)
    {
        size_t width = std::min<int>(img1.width(), img2.width());
        size_t height = std::min<int>(img1.height(), img2.height());
        image::Image<uint16_t> img_b(width, height, img1.channels());
        bool are_rgba = img1.channels() == 4 && img2.channels() == 4;

        for (int c = 0; c < img1.channels(); c++)
        {
            for (size_t i = 0; i < height * width; i++)
            {
                if (are_rgba)
                {
                    if (op == 1.0)
                    {
                        if (img2.channel(3)[i] != 0)
                            img_b.channel(c)[i] = img2.channel(c)[i];
                        else
                            img_b.channel(c)[i] = img1.channel(c)[i];
                    }
                    else if (op == 0.0)
                    {
                        if (img1.channel(3)[i] != 0)
                            img_b.channel(c)[i] = img1.channel(c)[i];
                        else
                            img_b.channel(c)[i] = img2.channel(c)[i];
                    }
                    else
                        img_b.channel(c)[i] = img2.channel(3)[i] != 0
                                                  ? std::min<int>(65535, img1.channel(c)[i] * (1.0f - op) + img2.channel(c)[i] * op)
                                                  : img1.channel(c)[i];
                }
                else
                {
                    if (op == 1.0)
                    {
                        if (img2.channel(c)[i] != 0)
                            img_b.channel(c)[i] = img2.channel(c)[i];
                        else
                            img_b.channel(c)[i] = img1.channel(c)[i];
                    }
                    else if (op == 0.0)
                    {
                        if (img1.channel(c)[i] != 0)
                            img_b.channel(c)[i] = img1.channel(c)[i];
                        else
                            img_b.channel(c)[i] = img2.channel(c)[i];
                    }
                    else
                        img_b.channel(c)[i] = std::min<int>(65535, img1.channel(c)[i] * (1.0f - op) + img2.channel(c)[i] * op);
                }
            }
        }
        return img_b;
    }
}