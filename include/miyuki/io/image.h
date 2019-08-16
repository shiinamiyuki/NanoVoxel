//
// Created by Shiina Miyuki on 2019/3/9.
//

#ifndef MIYUKI_IMAGE_H
#define MIYUKI_IMAGE_H
#include <util.h>
#include <miyuki/math/spectrum.h>
#include <miyuki/vec.hpp>

namespace Miyuki {
    namespace IO {
        enum class ImageFormat {
            none,
            raw,
        };

        template<typename Texel>
        struct GenericImage {
            int width = 0;
            int height = 0;
            std::vector<Texel> pixelData;

            Texel &operator()(int x, int y) {
                x = clamp(x, 0, width - 1);
                y = clamp(y, 0, height - 1);
                return pixelData[x + width * y];
            }

            const Texel &operator()(int x, int y) const {
                x = clamp(x, 0, width - 1);
                y = clamp(y, 0, height - 1);
                return pixelData[x + width * y];
            }

            Texel &operator()(const Point2i &p) {
                return (*this)(p[0], p[1]);
            }

            const Texel &operator()(const Point2i &p) const {
                return (*this)(p[0], p[1]);
            }

            Texel &operator[](int i) {
                Assert(0 <= i && i < pixelData.size());
                return pixelData[i];
            }

            const Texel &operator[](int i) const {
                Assert(0 <= i && i < pixelData.size());
                return pixelData[i];
            }

            GenericImage() : width(0), height(0) {}

            GenericImage(int width, int height) : width(width), height(height), pixelData(width * height) {}

        };

        struct Image : GenericImage<Spectrum> {
            ImageFormat format = ImageFormat::none;
            std::string filename;

            Image() : GenericImage<Spectrum>() {}

            Image(int width, int height) : GenericImage<Spectrum>(width, height) {}

            Image(const std::string &filename, ImageFormat format = ImageFormat::none);

            void save(const std::string &filename);

            void saveAsFormat(const std::string &filename, ImageFormat format);
        };

		using ImagePtr = Image *;

        void LoadHDR(const std::string &filename, Image &);

    }

}
#endif //MIYUKI_IMAGE_H
