#include "visionscope/view/protocol_sequences.hpp"

#include <zlib.h>

#include <algorithm>
#include <cstring>
#include <string>

namespace visionscope::view {
namespace {

constexpr char kBase64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void append_be32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
  out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
  out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
  out.push_back(static_cast<std::uint8_t>(value & 0xff));
}

std::uint32_t crc32_png(const std::uint8_t* data, std::size_t len) {
  std::uint32_t crc = 0xffffffffu;
  for (std::size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int b = 0; b < 8; ++b) {
      const std::uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xedb88320u & mask);
    }
  }
  return ~crc;
}

void write_chunk(std::vector<std::uint8_t>& out, const char type[4],
                 const std::vector<std::uint8_t>& data) {
  append_be32(out, static_cast<std::uint32_t>(data.size()));
  const std::size_t type_pos = out.size();
  out.insert(out.end(), type, type + 4);
  out.insert(out.end(), data.begin(), data.end());
  const std::uint32_t crc =
      crc32_png(out.data() + type_pos, 4 + data.size());
  append_be32(out, crc);
}

// Fast zlib (level 1): much smaller OSC payloads than stored DEFLATE blocks.
std::vector<std::uint8_t> zlib_compress(const std::uint8_t* data, std::size_t len) {
  if (len == 0) {
    uLongf bound = compressBound(1);
    std::vector<std::uint8_t> out(bound);
    uLongf out_len = bound;
    const std::uint8_t empty = 0;
    if (compress2(out.data(), &out_len, &empty, 0, 1) != Z_OK) {
      return {};
    }
    out.resize(out_len);
    return out;
  }
  uLongf bound = compressBound(static_cast<uLong>(len));
  std::vector<std::uint8_t> out(bound);
  uLongf out_len = bound;
  if (compress2(out.data(), &out_len, data, static_cast<uLong>(len), 1) !=
      Z_OK) {
    return {};
  }
  out.resize(out_len);
  return out;
}

int bytes_per_pixel(model::PixelFormat format) {
  return format == model::PixelFormat::kRgba32 ? 4 : 3;
}

}  // namespace

std::string base64_encode(const std::uint8_t* data, std::size_t len) {
  std::string out;
  out.reserve(((len + 2) / 3) * 4);
  std::size_t i = 0;
  while (i + 2 < len) {
    const std::uint32_t n = (std::uint32_t{data[i]} << 16) |
                            (std::uint32_t{data[i + 1]} << 8) |
                            std::uint32_t{data[i + 2]};
    out.push_back(kBase64Table[(n >> 18) & 63]);
    out.push_back(kBase64Table[(n >> 12) & 63]);
    out.push_back(kBase64Table[(n >> 6) & 63]);
    out.push_back(kBase64Table[n & 63]);
    i += 3;
  }
  if (i < len) {
    const std::uint32_t n = std::uint32_t{data[i]} << 16 |
                            ((i + 1 < len) ? (std::uint32_t{data[i + 1]} << 8) : 0);
    out.push_back(kBase64Table[(n >> 18) & 63]);
    out.push_back(kBase64Table[(n >> 12) & 63]);
    out.push_back(i + 1 < len ? kBase64Table[(n >> 6) & 63] : '=');
    out.push_back('=');
  }
  return out;
}

std::vector<std::uint8_t> encode_png_rgb24(const model::Frame& frame) {
  std::vector<std::uint8_t> png;
  if (frame.empty()) {
    return png;
  }

  const int bpp = bytes_per_pixel(frame.format);
  const std::size_t row_rgb = static_cast<std::size_t>(frame.width) * 3;
  std::vector<std::uint8_t> raw(
      static_cast<std::size_t>(frame.height) * (1 + row_rgb));
  std::size_t offset = 0;
  for (int y = 0; y < frame.height; ++y) {
    raw[offset++] = 0;  // filter None
    if (bpp == 3) {
      const auto src = static_cast<std::size_t>(y * frame.width * 3);
      if (src + row_rgb <= frame.pixels.size()) {
        std::memcpy(raw.data() + offset, frame.pixels.data() + src, row_rgb);
        offset += row_rgb;
        continue;
      }
    }
    for (int x = 0; x < frame.width; ++x) {
      const auto i =
          static_cast<std::size_t>((y * frame.width + x) * bpp);
      if (i + 2 >= frame.pixels.size()) {
        raw[offset++] = 0;
        raw[offset++] = 0;
        raw[offset++] = 0;
        continue;
      }
      raw[offset++] = frame.pixels[i + 0];
      raw[offset++] = frame.pixels[i + 1];
      raw[offset++] = frame.pixels[i + 2];
    }
  }
  raw.resize(offset);

  static const std::uint8_t signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
  png.insert(png.end(), signature, signature + 8);

  std::vector<std::uint8_t> ihdr;
  append_be32(ihdr, static_cast<std::uint32_t>(frame.width));
  append_be32(ihdr, static_cast<std::uint32_t>(frame.height));
  ihdr.push_back(8);  // bit depth
  ihdr.push_back(2);  // color type RGB
  ihdr.push_back(0);
  ihdr.push_back(0);
  ihdr.push_back(0);
  write_chunk(png, "IHDR", ihdr);

  auto idat = zlib_compress(raw.data(), raw.size());
  if (idat.empty()) {
    return {};
  }
  write_chunk(png, "IDAT", idat);
  write_chunk(png, "IEND", {});
  return png;
}

std::string build_iterm2_inline_sequence(const model::Frame& frame, int cell_w,
                                         int cell_h,
                                         const IFrameAccelerator& accel) {
  if (frame.empty() || cell_w <= 0 || cell_h <= 0) {
    return {};
  }
  // Keep encodes light — large images every frame stutter and feel like flicker.
  const auto scaled = accel.downscale_rgb24(
      frame, std::min(std::max(cell_w * 4, 96), 240),
      std::min(std::max(cell_h * 8, 54), 135));
  // Prefer platform-accelerated still image; PNG software fallback.
  auto encoded = accel.encode_iterm_image(scaled);
  if (encoded.empty()) {
    encoded = encode_png_rgb24(scaled);
  }
  if (encoded.empty()) {
    return {};
  }
  const auto b64 = base64_encode(encoded.data(), encoded.size());
  std::string out;
  out.reserve(b64.size() + 96);
  out += "\033]1337;File=inline=1;size=";
  out += std::to_string(encoded.size());
  out += ";width=";
  out += std::to_string(cell_w);
  out += ";height=";
  out += std::to_string(cell_h);
  // preserveAspectRatio=0 fills the reserved cell box exactly — letterboxing
  // can make replacements look like they jump/flicker between frames.
  out += ";preserveAspectRatio=0;doNotMoveCursor=1:";
  out += b64;
  out += '\a';
  return out;
}

std::string build_kitty_rgb_sequence(const model::Frame& frame, int cell_w,
                                     int cell_h,
                                     const IFrameAccelerator& accel) {
  if (frame.empty() || cell_w <= 0 || cell_h <= 0) {
    return {};
  }
  const auto scaled = accel.downscale_rgb24(
      frame, std::max(cell_w * 6, 120), std::max(cell_h * 12, 72));
  if (scaled.empty()) {
    return {};
  }

  const auto compressed =
      zlib_compress(scaled.pixels.data(), scaled.pixels.size());
  if (compressed.empty()) {
    return {};
  }
  const auto b64 = base64_encode(compressed.data(), compressed.size());
  constexpr std::size_t kChunk = 4096;
  std::string out;
  out.reserve(b64.size() + 128);

  std::size_t offset = 0;
  bool first = true;
  while (offset < b64.size()) {
    const std::size_t n = std::min(kChunk, b64.size() - offset);
    const bool more = offset + n < b64.size();
    out += "\033_G";
    if (first) {
      // Stable image id + zlib payload so Kitty replaces in place cheaply.
      out += "a=T,f=24,o=z,i=1,s=";
      out += std::to_string(scaled.width);
      out += ",h=";
      out += std::to_string(scaled.height);
      out += ",c=";
      out += std::to_string(cell_w);
      out += ",r=";
      out += std::to_string(cell_h);
      out += ",q=2,";
      first = false;
    }
    out += "m=";
    out += more ? '1' : '0';
    out += ';';
    out.append(b64, offset, n);
    out += "\033\\";
    offset += n;
  }
  return out;
}

}  // namespace visionscope::view
