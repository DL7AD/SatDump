#include "image.h"
#include <cstring>
#include <cmath>
#include <limits>
#include <vector>
#include "resources.h"

#define STB_TRUETYPE_IMPLEMENTATION // force following include to generate implementation
#include "common/image/font/imstb_truetype.h"

#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "font/utf8.h"

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
#include <iostream>
#include <fstream>

namespace image
{
    template <typename T>
    void Image<T>::draw_pixel(int x, int y, T color[])
    {
        // Check we're not out of bounds
        if (x < 0 || y < 0 || x >= (int)d_width || y >= (int)d_height)
            return;

        for (int c = 0; c < d_channels; c++)
            channel(c)[y * d_width + x] = color[c];
    }

    template <typename T>
    void Image<T>::draw_line(int x0, int y0, int x1, int y1, T color[])
    {
        if (x0 < 0 || x0 >= (int)d_width)
            return;
        if (x1 < 0 || x1 >= (int)d_width)
            return;
        if (y0 < 0 || y0 >= (int)d_height)
            return;
        if (y1 < 0 || y1 >= (int)d_height)
            return;

        int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = (dx > dy ? dx : -dy) / 2, e2;

        while (!(x0 == x1 && y0 == y1))
        {
            draw_pixel(x0, y0, color);

            e2 = err;

            if (e2 > -dx)
            {
                err -= dy;
                x0 += sx;
            }

            if (e2 < dy)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    template <typename T>
    void Image<T>::draw_circle(int x0, int y0, int radius, T color[], bool fill)
    {
        if (fill) // Filled circle
        {
            int x = radius;
            int y = 0;
            int err = 1 - x;

            while (x >= y)
            {
                draw_line(-x + x0, y + y0, x + x0, y + y0, color);
                if (y != 0)
                    draw_line(-x + x0, -y + y0, x + x0, -y + y0, color);

                y++;

                if (err < 0)
                    err += 2 * y + 1;
                else
                {
                    if (x >= y)
                    {
                        draw_line(-y + 1 + x0, x + y0, y - 1 + x0, x + y0, color);
                        draw_line(-y + 1 + x0, -x + y0, y - 1 + x0, -x + y0, color);
                    }

                    x--;
                    err += 2 * (y - x + 1);
                }
            }
        }
        else // Hollow circle
        {
            int err = 1 - radius;
            int xx = 0;
            int yy = -2 * radius;
            int x = 0;
            int y = radius;

            draw_pixel(x0, y0 + radius, color);
            draw_pixel(x0, y0 - radius, color);
            draw_pixel(x0 + radius, y0, color);
            draw_pixel(x0 - radius, y0, color);

            while (x < y)
            {
                if (err >= 0)
                {
                    y--;
                    yy += 2;
                    err += yy;
                }

                x++;
                xx += 2;
                err += xx + 1;

                draw_pixel(x0 + x, y0 + y, color);
                draw_pixel(x0 - x, y0 + y, color);
                draw_pixel(x0 + x, y0 - y, color);
                draw_pixel(x0 - x, y0 - y, color);
                draw_pixel(x0 + y, y0 + x, color);
                draw_pixel(x0 - y, y0 + x, color);
                draw_pixel(x0 + y, y0 - x, color);
                draw_pixel(x0 - y, y0 - x, color);
            }
        }
    }

    template <typename T>
    void Image<T>::draw_image(int c, Image<T> image, int x0, int y0)
    {
        // Get min height and width, mostly for safety
        int width = std::min<int>(d_width, x0 + image.width()) - x0;
        int height = std::min<int>(d_height, y0 + image.height()) - y0;

        for (int x = 0; x < width; x++)
            for (int y = 0; y < height; y++)
                if (y + y0 >= 0 && x + x0 >= 0)
                    channel(c)[(y + y0) * d_width + x + x0] = image[y * image.width() + x];

        if (c == 0 && image.channels() == d_channels) // Special case for non-grayscale images
        {
            for (int c = 1; c < d_channels; c++)
                for (int x = 0; x < width; x++)
                    for (int y = 0; y < height; y++)
                        if (y + y0 >= 0 && x + x0 >= 0)
                            channel(c)[(y + y0) * d_width + x + x0] = image.channel(c)[y * image.width() + x];
        }
    }

    std::vector<Image<uint8_t>> make_font(int size, bool text_mode)
    {
        Image<uint8_t> fontFile;
        fontFile.load_png(resources::getResourcePath("fonts/Roboto-Regular.png"));

        std::vector<Image<uint8_t>> font;

        int char_size = 120;
        int char_count = 95;
        int char_edge_crop = 30;

        for (int i = 0; i < char_count; i++)
        {
            Image<uint8_t> char_img = fontFile;
            char_img.crop(i * char_size, 0, (i + 1) * char_size, char_size);
            char_img.crop(char_edge_crop, char_img.width() - char_edge_crop);
            char_img.resize_bilinear((float)char_img.width() * ((float)size / (float)char_size), size, text_mode);
            font.push_back(char_img);
        }

        return font;
    }

    template <typename T>
    void Image<T>::draw_text(int xs0, int ys0, T color[], int s, std::string text)
    {
        if (!has_font)
            return;
        int CP = 0;
        std::vector<char> cstr(text.c_str(), text.c_str() + text.size() + 1);
        char *c = cstr.data();
        float SF = stbtt_ScaleForPixelHeight(&font.fontp, s);
        int BL = SF * font.y1;

        char_el info;

        while (c < c + cstr.size())
        {
            try
            {
                info.char_nb = utf8::next(c, c + cstr.size());
            }
            catch (utf8::invalid_utf8 &)
            {
                break;
            }

            if (info.char_nb == '\0')
                break;

            if (info.char_nb == 0xA)
            { // EOL
                ys0 += SF * (font.asc - font.dsc + font.lg);
                CP = 0;
                continue;
            }

            bool f = false;
            for (unsigned int k = 0; k < font.chars.size(); k++)
                if (font.chars[k].char_nb == info.char_nb)
                {
                    if (font.chars[k].size != s)
                    {
                        font.chars.erase(font.chars.begin() + k);
                        break;
                    }
                    f = true;
                    info = font.chars[k];
                    break;
                }

            if (!f)
            {
                // bitmap = stbtt_GetGlyphBitmap(&font.fontp, 0, SF, info.char_nb, &info.w, &info.h, 0, 0);
                info.glyph_nb = stbtt_FindGlyphIndex(&font.fontp, info.char_nb);
                stbtt_GetGlyphBox(&font.fontp, info.glyph_nb, &info.cx0, &info.cy0, &info.cx1, &info.cy1);
                stbtt_GetGlyphBitmapBox(&font.fontp, info.glyph_nb, SF, SF, &info.ix0, &info.iy0, &info.ix1, &info.iy1);
                stbtt_GetGlyphHMetrics(&font.fontp, info.glyph_nb, &info.advance, &info.lsb);
                info.w = abs(info.ix1 - info.ix0);
                info.h = abs(info.iy1 - info.iy0);
                info.bitmap = (unsigned char *)malloc(info.w * info.h);
                info.size = s;
                memset(info.bitmap, 0, info.w * info.h);
                stbtt_MakeGlyphBitmap(&font.fontp, info.bitmap, info.w, info.h, info.w, SF, SF, info.glyph_nb);
                font.chars.push_back(info);
            }

            int pos = 0;
            for (int j = 0; j < info.h; ++j)
                for (int i = 0; i < info.w; ++i)
                {
                    unsigned char m = info.bitmap[pos];
                    int x = xs0 + i + CP + SF * info.lsb, y = j + BL - info.cy1 * SF + ys0;
                    unsigned int pos2 = y * width() + x;
                    if (m != 0 && pos2 < width() * height())
                    {
                        float mf = m / 255.0;
                        T col[] = {static_cast<T>((color[0] - channel(0)[pos2]) * mf + channel(0)[pos2]),
                                   d_channels > 1 ? static_cast<T>((color[1] - channel(1)[pos2]) * mf + channel(1)[pos2]) : std::numeric_limits<T>::max(),
                                   d_channels > 2 ? static_cast<T>((color[2] - channel(2)[pos2]) * mf + channel(2)[pos2]) : std::numeric_limits<T>::max(),
                                   d_channels > 3 ? static_cast<T>((color[3] - channel(3)[pos2]) * mf + channel(3)[pos2]) : std::numeric_limits<T>::max()};
                        draw_pixel(x, y, col);
                    }
                    pos++;
                }
            // if (!f)
            // stbtt_FreeBitmap(bitmap, font.fontp.userdata);

            CP += SF * info.advance;
        }
    }

    template <typename T>
    void Image<T>::draw_rectangle(int x0, int y0, int x1, int y1, T color[], bool fill){
        if (fill){
            for (int y = std::min(y0, y1); y < std::max(y0, y1); y++)
                draw_line(x0, y, x1, y, color);
        }else{
            draw_line(x0, y0, x0, y1, color);
            draw_line(x0, y1, x1, y1, color);
            draw_line(x1, y1, x1, y0, color);
            draw_line(x1, y0, x0, y0, color);
        }
    }

    template <typename T>
    void Image<T>::draw_text(int x0, int y0, T color[], std::vector<Image<uint8_t>> font, std::string text)
    {
        int pos = x0;
        T *colorf = new T[d_channels];

        for (char character : text)
        {
            if (size_t(character - 31) > font.size())
                continue;

            Image<uint8_t> &img = font[character - 31];

            for (int x = 0; x < (int)img.width(); x++)
            {
                for (int y = 0; y < (int)img.height(); y++)
                {
                    float value = img[y * img.width() + x] / 255.0f;

                    if (value == 0)
                        continue;

                    for (int c = 0; c < d_channels; c++)
                        colorf[c] = color[c] * value;

                    draw_pixel(pos + x, y0 + y, colorf);
                }
            }

            pos += img.width();
        }

        delete[] colorf;
    }

    template <typename T>
    Image<T> generate_text_image(std::string text, T color[], int height, int padX, int padY)
    {
        std::vector<Image<uint8_t>> font = make_font(height - 2 * padY, false);
        int width = font[0].width() * text.length() + 2 * padX;
        Image<T> out(width, height, 1);
        out.fill(0);
        out.draw_text(padX, 0, color, font, text);
        return out;
    }

    template Image<uint8_t> generate_text_image(std::string text, uint8_t color[], int height, int padX, int padY);
    template Image<uint16_t> generate_text_image(std::string text, uint16_t color[], int height, int padX, int padY);

    // Generate Images for uint16_t and uint8_t
    template class Image<uint8_t>;
    template class Image<uint16_t>;
}
