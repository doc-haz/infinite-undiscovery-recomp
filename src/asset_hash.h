#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace iu::assets {
class DigestState {
  BCRYPT_ALG_HANDLE alg_ = nullptr;
  BCRYPT_HASH_HANDLE hash_ = nullptr;
  ULONG length_ = 0;
 public:
  explicit DigestState(bool sha1 = false) : length_(sha1 ? 20 : 32) {
    if (BCryptOpenAlgorithmProvider(&alg_, sha1 ? BCRYPT_SHA1_ALGORITHM : BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
      throw std::runtime_error("No se puede iniciar la validacion de integridad.");
    if (BCryptCreateHash(alg_, &hash_, nullptr, 0, nullptr, 0, 0) < 0) {
      BCryptCloseAlgorithmProvider(alg_, 0);
      throw std::runtime_error("No se puede iniciar el hash.");
    }
  }
  DigestState(const DigestState&) = delete;
  ~DigestState() { if (hash_) BCryptDestroyHash(hash_); if (alg_) BCryptCloseAlgorithmProvider(alg_, 0); }
  void Add(const void* data, size_t size) {
    if (size > ULONG_MAX || BCryptHashData(hash_, reinterpret_cast<PUCHAR>(const_cast<void*>(data)), static_cast<ULONG>(size), 0) < 0)
      throw std::runtime_error("Error al verificar la integridad.");
  }
  std::string Finish() {
    std::array<unsigned char, 32> bytes{};
    if (BCryptFinishHash(hash_, bytes.data(), length_, 0) < 0) throw std::runtime_error("Error al finalizar el hash.");
    std::ostringstream s; s << std::hex << std::setfill('0');
    for (ULONG i = 0; i < length_; ++i) s << std::setw(2) << unsigned(bytes[i]);
    return s.str();
  }
};
inline std::string HashFile(const std::filesystem::path& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) throw std::runtime_error("No se puede leer el archivo para verificarlo.");
  std::vector<char> bytes(4 * 1024 * 1024); DigestState hash;
  while (in) { in.read(bytes.data(), bytes.size()); if (auto n = in.gcount()) hash.Add(bytes.data(), static_cast<size_t>(n)); }
  if (!in.eof()) throw std::runtime_error("Archivo ilegible durante la verificacion.");
  return hash.Finish();
}
}
