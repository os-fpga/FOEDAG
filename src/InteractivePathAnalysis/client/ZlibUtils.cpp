/**
  * @file ZlibUtils.cpp
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ZlibUtils.h"

#include <zlib.h>

#include <cstring>  // Include cstring for memset

namespace FOEDAG {

std::optional<std::string> tryCompress(const std::string& decompressed) {
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
    return std::nullopt;
  }

  zs.next_in = (Bytef*)decompressed.data();
  zs.avail_in = decompressed.size();

  int retCode;
  char resultBuffer[32768];
  std::string result;

  do {
    zs.next_out = reinterpret_cast<Bytef*>(resultBuffer);
    zs.avail_out = sizeof(resultBuffer);

    retCode = deflate(&zs, Z_FINISH);

    if (result.size() < zs.total_out) {
      result.append(resultBuffer, zs.total_out - result.size());
    }
  } while (retCode == Z_OK);

  deflateEnd(&zs);

  if (retCode != Z_STREAM_END) {
    return std::nullopt;
  }

  return result;
}

std::optional<std::string> tryDecompress(const std::string& compressed) {
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (inflateInit(&zs) != Z_OK) {
    return std::nullopt;
  }

  zs.next_in = (Bytef*)compressed.data();
  zs.avail_in = compressed.size();

  int retCode;
  char resultBuffer[32768];
  std::string result;

  do {
    zs.next_out = reinterpret_cast<Bytef*>(resultBuffer);
    zs.avail_out = sizeof(resultBuffer);

    retCode = inflate(&zs, 0);

    if (result.size() < zs.total_out) {
      result.append(resultBuffer, zs.total_out - result.size());
    }

  } while (retCode == Z_OK);

  inflateEnd(&zs);

  if (retCode != Z_STREAM_END) {
    return std::nullopt;
  }

  return result;
}

}  // namespace FOEDAG