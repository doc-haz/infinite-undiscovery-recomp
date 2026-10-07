#include "asset_setup.h"
#include "asset_hash.h"
#include "portable_setup.h"
#include "content_profile.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <wrl/client.h>
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <future>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <cwctype>

namespace iu::assets {
namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(portable::Text(message)); }
uint32_t BE(const uint8_t* p) { return uint32_t(p[0])<<24 | uint32_t(p[1])<<16 | uint32_t(p[2])<<8 | p[3]; }
uint32_t LE(const uint8_t* p) { return uint32_t(p[3])<<24 | uint32_t(p[2])<<16 | uint32_t(p[1])<<8 | p[0]; }
std::vector<uint8_t> Read(const fs::path& p, uint64_t off, uint64_t count) {
  const auto size = fs::file_size(p);
  Require(off <= size && count <= size-off && count <= 32*1024*1024, "Fuente truncada o rango invalido.");
  std::ifstream in(p, std::ios::binary);
  Require(bool(in), "No se puede abrir la fuente.");
  in.seekg(static_cast<std::streamoff>(off));
  std::vector<uint8_t> out(static_cast<size_t>(count));
  in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(count));
  Require(bool(in), "No se puede leer la fuente.");
  return out;
}
std::string Hex(const uint8_t* p, size_t n) {
  std::ostringstream s; s << std::hex << std::setfill('0');
  for (size_t i=0;i<n;++i) s << std::setw(2) << unsigned(p[i]);
  return s.str();
}
class Hash {
  BCRYPT_ALG_HANDLE alg_ = nullptr;
  BCRYPT_HASH_HANDLE hash_ = nullptr;
 public:
  Hash() {
    Require(BCryptOpenAlgorithmProvider(&alg_, BCRYPT_SHA256_ALGORITHM, nullptr, 0)>=0, "SHA256 no disponible.");
    if (BCryptCreateHash(alg_, &hash_, nullptr, 0, nullptr, 0, 0)<0) {
      BCryptCloseAlgorithmProvider(alg_,0); throw std::runtime_error("Error creando SHA256.");
    }
  }
  ~Hash() { if(hash_) BCryptDestroyHash(hash_); if(alg_) BCryptCloseAlgorithmProvider(alg_,0); }
  void Add(const void* p, size_t n) { Require(BCryptHashData(hash_, reinterpret_cast<PUCHAR>(const_cast<void*>(p)), static_cast<ULONG>(n), 0)>=0, "Error SHA256."); }
  std::string Finish() { std::array<uint8_t,32> b{}; Require(BCryptFinishHash(hash_,b.data(),32,0)>=0,"Error SHA256."); return Hex(b.data(),b.size()); }
};
std::string Digest(const std::vector<uint8_t>& b) { Hash h; h.Add(b.data(),b.size()); return h.Finish(); }
std::string Lower(std::string s) { for(auto& c:s) if(c>='A' && c<='Z') c+=32; return s; }
const File& Find(const Disc& d, const std::string& name) {
  auto it=std::find_if(d.files.begin(), d.files.end(), [&](const File& f){return f.name==name;});
  Require(it!=d.files.end(), "Falta default.xex, ud1.bin o ud2.bin."); return *it;
}
const char* ExpectedBinHash(const std::string& edition, unsigned number, const std::string& name) {
  Require(number>=1 && number<=2 && (name=="ud1.bin" || name=="ud2.bin"),"Perfil de contenedor invalido.");
  if (edition == "USA-UNDUB") {
    const char* expected[2][2] = {
      {"72b6089ca718897d63c150e63daa033036e99b3316f6bf436ff29231d5da6360", "2ae9486edf0678869b4314a6cfecb9e915a631098d850fa36ce3ce3942dbfb6b"},
      {"aff26b0764a12656b66443c56ad9e7bd5ff4368709ef5b4ae463c89aca9fe5fe", "d9e37b4214a3b140e9a4b6ab82c8639b9cfc776af0d1755eedca063b39377e93"}};
    return expected[number-1][name=="ud1.bin"?0:1];
  }
  if (edition == "JAPAN") {
    const char* expected[2][2] = {
      {"95c432606f5a007656a032b222db59a70b5e27e6ad8105aca3b2420ebfc70bef", "2ae9486edf0678869b4314a6cfecb9e915a631098d850fa36ce3ce3942dbfb6b"},
      {"f6999e60d032cf7dddbe318d518a07dc2609986ee0cf6345bd269ce95a158251", "d9e37b4214a3b140e9a4b6ab82c8639b9cfc776af0d1755eedca063b39377e93"}};
    return expected[number-1][name=="ud1.bin"?0:1];
  }
  // USA, EUROPE, ASIA
  const char* expected[2][2] = {
    {"e3fbcbba6dddef6bb04159f5fe210dfecbc27c9e6547ee75867e026bd57a6a0f", "1b433977cc5991722730813bad1748a97c7bbd245bd467b9ad43a91726d0f0e1"},
    {"30783ac333050512c41ffe5df05c7008e202f95db1818308e4f89d916e2c659b", "f01e9f1640bd7cef8dc9d9853d249ee72036e157507b0e629f4eed6fa25d2a4f"}};
  return expected[number-1][name=="ud1.bin"?0:1];
}
void ImageFiles(Disc& d) {
  const auto size = fs::file_size(d.source);
  uint64_t base=UINT64_MAX;
  for(uint64_t candidate : {0ull,0x02080000ull,0x0FD90000ull,0x18300000ull}) {
    if(candidate+33*2048>size) continue;
    const auto b=Read(d.source,candidate+32*2048,2048);
    if(!std::memcmp(b.data(),"MICROSOFT*XBOX*MEDIA",20) && !std::memcmp(b.data()+0x7ec,"MICROSOFT*XBOX*MEDIA",20)) { base=candidate; break; }
  }
  Require(base!=UINT64_MAX,"No se encontro XDVDFS. Seleccione una ISO compatible o carpeta extraida.");
  auto volume=Read(d.source,base+32*2048,2048);
  uint64_t root=base+uint64_t(LE(volume.data()+20))*2048;
  const auto root_size=LE(volume.data()+24);
  Require(root_size>0 && root_size<=16*1024*1024,"Directorio XDVDFS invalido.");
  auto b=Read(d.source,root,root_size);
  std::vector<uint32_t> pending{0}; std::set<uint32_t> seen;
  while(!pending.empty()) {
    uint32_t off=pending.back(); pending.pop_back();
    Require(seen.insert(off).second && off<=b.size() && b.size()-off>=14,"Arbol XDVDFS corrupto.");
    const auto* p=b.data()+off;
    unsigned left=p[0]|p[1]<<8, right=p[2]|p[3]<<8;
    for(unsigned child:{left,right}) if(child!=0 && child!=0xffff) pending.push_back(child*4);
    unsigned n=p[13]; Require(n && n<=b.size()-off-14,"Nombre XDVDFS truncado.");
    if(p[12]&0x10) continue; // Only the three root files; never follow image paths.
    std::string name=Lower(std::string(reinterpret_cast<const char*>(p+14),n));
    if(name!="default.xex" && name!="ud1.bin" && name!="ud2.bin") continue;
    uint64_t offset=base+uint64_t(LE(p+4))*2048, length=LE(p+8);
    Require(offset<=size && length<=size-offset,"Archivo XDVDFS truncado.");
    Require(std::none_of(d.files.begin(),d.files.end(),[&](const File& f){return f.name==name;}),"Archivo XDVDFS duplicado.");
    d.files.push_back({name,offset,length,d.source});
  }
}
bool SameOrInside(const fs::path& child, const fs::path& parent) {
  auto c=fs::weakly_canonical(child).wstring(), p=fs::weakly_canonical(parent).wstring();
  std::transform(c.begin(),c.end(),c.begin(),towlower); std::transform(p.begin(),p.end(),p.begin(),towlower);
  if(c==p) return true;
  if(!p.empty() && p.back()!=L'\\') p+=L'\\';
  return c.starts_with(p);
}
}

Disc Inspect(const fs::path& source) {
  Disc d; d.source=fs::absolute(source);
  Require(fs::exists(d.source),"La ruta no existe.");
  if(fs::is_directory(d.source)) {
    for(const auto& e:fs::directory_iterator(d.source)) {
      if(!e.is_regular_file()) continue;
      auto name=Lower(e.path().filename().string());
      if(name=="default.xex" || name=="ud1.bin" || name=="ud2.bin") d.files.push_back({name,0,e.file_size(),e.path()});
    }
  } else { d.image=true; ImageFiles(d); }
  const auto& f=Find(d,"default.xex");
  Require(f.size>=24 && f.size<=32*1024*1024,"Tamano XEX invalido.");
  auto b=Read(f.host,f.offset,f.size);
  Require(!std::memcmp(b.data(),"XEX2",4),"La fuente no contiene un XEX2.");
  uint32_t count=BE(b.data()+20), security=BE(b.data()+16);
  Require(count<=4096 && 24ull+8ull*count<=b.size(),"Cabeceras XEX truncadas.");
  Require(security<=b.size() && b.size()-security>=0x180,"Seguridad XEX truncada.");
  d.region=BE(b.data()+security+0x178);
  d.media_id=Hex(b.data()+security+0x140,16);
  bool execution=false;
  for(uint32_t i=0;i<count;++i) {
    const auto* p=b.data()+24+i*8;
    if(BE(p)==0x000406ff) {
      uint32_t off=BE(p+4); Require(off<=b.size() && b.size()-off>=36,"Media IDs multidisco truncados.");
      Require(BE(b.data()+off)==36,"Tabla multidisco no soportada.");
      for(unsigned disc=0;disc<2;++disc) d.multidisc_ids[disc]=Hex(b.data()+off+4+disc*16,16);
    }
    if(BE(p)!=0x00040006) continue;
    Require(!execution,"Execution info duplicada."); execution=true;
    uint32_t off=BE(p+4); Require(off<=b.size() && b.size()-off>=24,"Execution info truncada.");
    d.title=BE(b.data()+off+12); d.number=b[off+18];
    d.version=BE(b.data()+off+4); d.base_version=BE(b.data()+off+8);
    Require(d.title==0x535107DB,"El Title ID no es Infinite Undiscovery (535107DB).");
    Require(b[off+19]==2 && (d.number==1 || d.number==2),"Numero de disco invalido.");
  }
  Require(execution,"Falta execution info XEX.");
  d.xex_sha256=Digest(b);

  bool is_undub = (d.xex_sha256 == "42dfaa90814f5b78592bb637ac371df20c30acfa897f922fff96d3251553473b" ||
                   d.xex_sha256 == "a12e9253839a0e47d5bdcbfb92638b349ddecea6e384a1ea09e3e8e7441bbb5f");
  if (is_undub) {
    d.edition = "USA-UNDUB";
    Require((d.number == 1 && d.xex_sha256 == "42dfaa90814f5b78592bb637ac371df20c30acfa897f922fff96d3251553473b") ||
            (d.number == 2 && d.xex_sha256 == "a12e9253839a0e47d5bdcbfb92638b349ddecea6e384a1ea09e3e8e7441bbb5f"),
            "Hash XEX no coincide con el numero de disco UNDUB.");
    Require(d.multidisc_ids[0] == "9bc8428aeaa7bd6247f450ad625d4d59" &&
            d.multidisc_ids[1] == "7433c19ca506567415eaee1058af47d6",
            "Tabla multidisco no corresponde a Infinite Undiscovery USA.");
    d.media_id = d.multidisc_ids[d.number - 1];
    const uint64_t expected_undub[2][2] = {{2214635520ull, 2775879680ull}, {3227109376ull, 3265337344ull}};
    Require(Find(d, "ud1.bin").size == expected_undub[d.number - 1][0] &&
            Find(d, "ud2.bin").size == expected_undub[d.number - 1][1],
            "Contenedores ausentes o tamanos no reconocidos para este disco UNDUB.");
  } else {
    if (d.region == 0x00ff0000) {
      d.edition = "EUROPE";
    } else if (d.region == 0x000000ff) {
      d.edition = "USA";
    } else if (d.region == 0x0000fd00) {
      if (d.multidisc_ids[0] == "ca82a3db65f9c645eb3b590242d60dc3" &&
          d.multidisc_ids[1] == "5546a467358fa42d5fbc2b61245a1fd3") {
        d.edition = "JAPAN";
      } else if (d.multidisc_ids[0] == "428e2765894274bd38637b692f1c00f9" &&
                 d.multidisc_ids[1] == "84626b613e9b2189dedb871c25d0db07") {
        d.edition = "ASIA";
      } else {
        throw std::runtime_error("Region NTSC-J no reconocida en Infinite Undiscovery.");
      }
    } else {
      throw std::runtime_error("Region ambigua o no soportada; no se inferira por el nombre.");
    }
    Require(d.media_id != "00000000000000000000000000000000", "Media ID vacio.");
    Require(d.multidisc_ids[d.number - 1] == d.media_id, "El Media ID no corresponde al numero de disco.");

    if (d.edition == "JAPAN") {
      const uint64_t expected_japan[2][2] = {{2216155136ull, 2775879680ull}, {3228628992ull, 3265337344ull}};
      Require(Find(d, "ud1.bin").size == expected_japan[d.number - 1][0] &&
              Find(d, "ud2.bin").size == expected_japan[d.number - 1][1],
              "Contenedores ausentes o tamanos no reconocidos para Japón.");
    } else {
      const uint64_t expected[2][2] = {{2207584256ull, 2800330752ull}, {3217651712ull, 3289788416ull}};
      Require(Find(d, "ud1.bin").size == expected[d.number - 1][0] &&
              Find(d, "ud2.bin").size == expected[d.number - 1][1],
              "Contenedores ausentes o tamanos no reconocidos para este disco.");
    }
  }
  return d;
}

FolderScanResult ScanFolderForMedia(const fs::path& folder,
                                    const std::optional<std::string>& target_edition) {
  FolderScanResult res;
  if (!fs::exists(folder) || !fs::is_directory(folder)) {
    res.error_message = "La carpeta especificada no existe.";
    return res;
  }

  std::vector<fs::path> candidates;
  std::vector<fs::path> dlc_search_dirs;

  // 1. Check if folder itself is a direct disc folder
  try {
    Disc d = Inspect(folder);
    if (!target_edition || d.edition == *target_edition) {
      if (d.number == 1) res.disc1 = d;
      else if (d.number == 2) res.disc2 = d;
    }
  } catch (...) {
    // Not a direct disc root, continue scanning
  }

  // 2. Scan immediate children for ISOs and subdirectories
  for (const auto& e : fs::directory_iterator(folder)) {
    if (e.is_regular_file()) {
      auto ext = Lower(e.path().extension().string());
      if (ext == ".iso") candidates.push_back(e.path());
    } else if (e.is_directory()) {
      auto name = Lower(e.path().filename().string());
      if (name.find("dlc") != std::string::npos) {
        dlc_search_dirs.push_back(e.path());
      } else {
        candidates.push_back(e.path());
      }
    }
  }

  // If folder itself has DLC subdir, also add it
  if (fs::exists(folder / "dlc") && fs::is_directory(folder / "dlc")) {
    dlc_search_dirs.push_back(folder / "dlc");
  }

  std::vector<Disc> found_discs;
  for (const auto& cand : candidates) {
    try {
      Disc d = Inspect(cand);
      found_discs.push_back(std::move(d));
    } catch (...) {}
  }

  if (target_edition) {
    for (const auto& d : found_discs) {
      if (d.edition == *target_edition) {
        if (d.number == 1 && !res.disc1) res.disc1 = d;
        else if (d.number == 2 && !res.disc2) res.disc2 = d;
      }
    }
  } else {
    // Group discs by edition
    std::map<std::string, std::pair<std::optional<Disc>, std::optional<Disc>>> by_edition;
    for (const auto& d : found_discs) {
      auto& pair = by_edition[d.edition];
      if (d.number == 1 && !pair.first) pair.first = d;
      else if (d.number == 2 && !pair.second) pair.second = d;
    }

    for (const auto& [ed, _] : by_edition) {
      res.found_editions.push_back(ed);
    }

    // Pick first edition with disc1, or just first available
    for (const auto& [ed, pair] : by_edition) {
      if (pair.first) {
        res.disc1 = pair.first;
        res.disc2 = pair.second;
        break;
      }
    }
    if (!res.disc1 && !by_edition.empty()) {
      res.disc1 = by_edition.begin()->second.first;
      res.disc2 = by_edition.begin()->second.second;
    }
  }

  // 3. Scan for DLC packages in dlc_search_dirs or subdirectories
  for (const auto& dlc_dir : dlc_search_dirs) {
    try {
      for (const auto& sub : fs::recursive_directory_iterator(dlc_dir)) {
        if (sub.is_regular_file()) {
          try {
            auto pkg = iu::dlc::Inspect(sub.path());
            res.dlc_packages.push_back(pkg);
          } catch (...) {}
        }
      }
    } catch (...) {}
  }

  return res;
}

void ValidatePair(const Disc& first, const std::optional<Disc>& second) {
  Require(first.number==1,"La primera fuente debe ser Disc 1.");
  if(second) {
    Require(second->number==2,"La segunda fuente debe ser Disc 2.");
    Require(first.title==second->title && first.region==second->region && first.edition==second->edition,"No mezcle discos de diferentes regiones o ediciones.");
    Require(first.media_id!=second->media_id,"Ambas fuentes tienen el mismo Media ID.");
    Require(first.multidisc_ids==second->multidisc_ids && first.version==second->version && first.base_version==second->base_version,"Los discos no pertenecen al mismo conjunto/version.");
  }
}

bool CanLaunch(const Disc& disc) {
  // Disc 1 XEXs across all editions share the identical decrypted executable codebase
  // and binary-backed entry points.
  return disc.number == 1 &&
      (disc.xex_sha256 == "22893bb8d96a1440ecbdbcae543baeaf89d26588c89c99a2c96fecf611475325" ||
       disc.xex_sha256 == "9523b45e6a724d4988ce9cf70d55b672e461b89303b02dc2f0f9c84329c25555" ||
       disc.xex_sha256 == "42dfaa90814f5b78592bb637ac371df20c30acfa897f922fff96d3251553473b" ||
       disc.xex_sha256 == "cbb789e5e8842398253839b2e320874b3851bd10aaa2392aadf5cdeed438cc91" ||
       disc.xex_sha256 == "fd063de17d98ab1201795efab4ffd06d76e85eeb4da0ce751926792deeeccd33");
}

bool Ready(const fs::path& root, std::string* reason) {
  try {
    auto d=Inspect(root); Require(CanLaunch(d),"Edicion reconocida, pero este XEX no corresponde a un build admitido.");
    auto fingerprints=root.parent_path()/"asset-files.txt";
    if(root.filename()=="disc1" && fs::exists(fingerprints)) {
      std::ifstream in(fingerprints); std::string relative; uint64_t size; int64_t timestamp; std::set<std::string> seen;
      while(in>>relative) {
        Require(bool(in>>size>>timestamp),"Registro de integridad truncado.");
        Require(relative=="disc1/default.xex" || relative=="disc1/ud1.bin" || relative=="disc1/ud2.bin" || relative=="disc2/default.xex" || relative=="disc2/ud1.bin" || relative=="disc2/ud2.bin","Registro de integridad invalido.");
        Require(seen.insert(relative).second,"Registro de integridad duplicado.");
        auto file=root.parent_path()/fs::path(relative);
        Require(fs::is_regular_file(file) && fs::file_size(file)==size && fs::last_write_time(file).time_since_epoch().count()==timestamp,"Los assets instalados cambiaron o estan incompletos.");
      }
      Require(in.eof() && (seen.size()==3 || seen.size()==6) && seen.contains("disc1/default.xex") && seen.contains("disc1/ud1.bin") && seen.contains("disc1/ud2.bin"),"Registro de integridad incompleto.");
      if(seen.size()==6) ValidatePair(d,Inspect(root.parent_path()/"disc2"));
    } else {
      for(const auto& f:d.files) if(f.name!="default.xex") Require(HashFile(f.host)==ExpectedBinHash(d.edition,d.number,f.name),"Los assets existentes estan corruptos o no corresponden al perfil admitido.");
    }
    return true;
  }
  catch(const std::exception& e) { if(reason) *reason=e.what(); return false; }
}

fs::path Install(const Disc& first, const std::optional<Disc>& second,
                 const fs::path& parent, const Progress& progress,
                 const std::function<bool()>& cancelled,
                 const std::vector<iu::dlc::Package>& dlc) {
  ValidatePair(first,second);
  Require(CanLaunch(first),"Este XEX Disc 1 no coincide con los builds admitidos.");
  iu::dlc::ValidateSelection(dlc);
  Require(fs::is_directory(parent),"El destino debe ser una carpeta existente.");
  for(auto check=fs::absolute(parent); !check.empty();) {
    auto attributes=GetFileAttributesW(check.c_str());
    Require(attributes==INVALID_FILE_ATTRIBUTES || !(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"El destino contiene enlaces o junctions; elija otra carpeta.");
    auto ancestor=check.parent_path(); if(ancestor==check) break; check=ancestor;
  }
  for(const Disc* d:{&first, second? &*second:nullptr}) if(d)
    Require(!SameOrInside(parent,d->image? d->source: d->source),"El destino no puede estar dentro de la fuente.");
  uint64_t total=0; for(const Disc* d:{&first,second? &*second:nullptr}) if(d) for(const auto& f:d->files) total+=f.size;
  for(const auto& p:dlc) total+=p.size;
  Require(fs::space(parent).available>=total+64*1024*1024,"No hay espacio suficiente en el destino.");
  fs::path final=portable::Layout(parent,iu::ProfileFolderFromEdition(first.edition)).assets, stage; bool reserved=false;
  Require(!fs::exists(final),"La region ya tiene assets; se conservan sin sobrescribir.");
  fs::create_directories(final.parent_path());
  for(unsigned attempt=0;attempt<1000;++attempt) {
    auto token=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(attempt);
    stage=final.parent_path()/(".iu-install-"+token);
    if(fs::exists(final)) continue;
    if(fs::create_directory(stage)) { reserved=true; break; }
  }
  Require(reserved,"No se pudo reservar el destino.");
  uint64_t done=0;
  try {
    std::ofstream manifest(stage/"asset-install.txt",std::ios::binary);
    Require(bool(manifest),"No se puede crear el manifiesto.");
    manifest << "IU-ASSETS-1\nTitleID=535107DB\nEdition=" << first.edition << "\nActiveDisc=1\nDiscSwap=not-integrated\nDLC=SDK-STFS\n";
    std::vector<char> buf(4*1024*1024);
    for(const Disc* d:{&first,second? &*second:nullptr}) if(d) {
      auto dir=stage/("disc"+std::to_string(d->number)); fs::create_directory(dir);
      manifest << "Disc" << d->number << "MediaID=" << d->media_id << '\n';
      for(const auto& f:d->files) {
        Require(!cancelled(),"Instalacion cancelada.");
        std::ifstream in(f.host,std::ios::binary); std::ofstream out(dir/f.name,std::ios::binary);
        Require(bool(in) && bool(out),"No se puede copiar un archivo.");
        in.seekg(static_cast<std::streamoff>(f.offset));
        uint64_t remaining=f.size; Hash hash;
        while(remaining) {
          Require(!cancelled(),"Instalacion cancelada.");
          auto n=static_cast<std::streamsize>(std::min<uint64_t>(remaining,buf.size()));
          in.read(buf.data(),n); Require(bool(in),"Fuente truncada durante la copia.");
          out.write(buf.data(),n); Require(bool(out),"Error de escritura (espacio o permisos).");
          hash.Add(buf.data(),static_cast<size_t>(n)); remaining-=n; done+=n; progress(done,total);
        }
        out.close(); Require(bool(out),"Error cerrando archivo.");
        Require(fs::file_size(dir/f.name)==f.size,"Copia incompleta.");
        auto digest=hash.Finish();
        if(f.name=="default.xex") Require(digest==d->xex_sha256,"La fuente XEX cambio durante la copia.");
        if(f.name=="ud1.bin" || f.name=="ud2.bin") Require(digest==ExpectedBinHash(d->edition,d->number,f.name),"Contenedor BIN corrupto o revision no admitida; no se publicara la instalacion.");
        manifest << "disc" << d->number << '/' << f.name << " SHA256=" << digest << " Size=" << f.size << '\n';
      }
      const auto verified=Inspect(dir);
      Require(verified.xex_sha256==d->xex_sha256,"Identidad instalada distinta de la seleccionada.");
    }
    if(!dlc.empty()) fs::create_directory(stage/"dlc");
    for(const auto& p:dlc) {
      Require(!cancelled(),"Instalacion cancelada.");
      auto target=stage/"dlc"/p.filename;
      Require(fs::copy_file(p.source,target,fs::copy_options::none),"No se puede copiar el paquete DLC.");
      auto copied=iu::dlc::Inspect(target);
      Require(copied.sha256==p.sha256,"El paquete DLC cambio durante la copia.");
      manifest << "dlc/" << p.filename << " SHA256=" << p.sha256 << '\n';
      done+=p.size; progress(done,total);
    }
    // Fast startup integrity check: stat fingerprint invalidates edited files.
    std::ofstream fingerprint(stage/"asset-files.txt",std::ios::binary);
    Require(bool(fingerprint),"No se puede crear el registro de integridad.");
    for(const Disc* d:{&first,second? &*second:nullptr}) if(d) for(const auto& f:d->files) {
      auto relative=fs::path("disc"+std::to_string(d->number))/f.name;
      auto installed=stage/relative;
      fingerprint<<relative.generic_string()<<' '<<fs::file_size(installed)<<' '<<fs::last_write_time(installed).time_since_epoch().count()<<'\n';
    }
    fingerprint.close(); Require(bool(fingerprint),"No se pudo guardar el registro de integridad.");
    manifest.close(); Require(bool(manifest),"No se pudo cerrar el manifiesto.");
    Require(!cancelled(),"Instalacion cancelada.");
    Require(MoveFileExW(stage.c_str(),final.c_str(),MOVEFILE_WRITE_THROUGH)!=0,"No se puede publicar la instalacion sin sobrescribir."); return final/"disc1";
  } catch(...) {
    // Only remove the uniquely reserved staging directory created above.
    std::error_code ignored; fs::remove_all(stage,ignored); throw;
  }
}

namespace {
std::wstring Wide(const std::string& s) { return portable::Wide(s); }
int Dialog(const std::wstring& heading,const std::wstring& text,const std::vector<TASKDIALOG_BUTTON>& buttons) {
  std::vector<portable::Button> choices; for(auto& b:buttons) choices.push_back({b.nButtonID,b.pszButtonText});
  return portable::Dialog(heading,text,choices);
}
}

std::optional<fs::path> Pick(bool folder, void* owner) {
  Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
  Require(SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))),"No se pudo abrir el selector.");
  FILEOPENDIALOGOPTIONS flags{}; dialog->GetOptions(&flags);
  dialog->SetOptions(flags|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST|FOS_FILEMUSTEXIST|(folder?FOS_PICKFOLDERS:0));
  if(!folder) { const COMDLG_FILTERSPEC filters[]={{L"Disc Image or XEX (*.iso, default.xex)",L"*.iso;default.xex"},{L"XDVDFS ISO (*.iso)",L"*.iso"},{L"All files / Todos los archivos",L"*.*"}}; dialog->SetFileTypes(3,filters); }
  auto hr=dialog->Show(reinterpret_cast<HWND>(owner)); if(hr==HRESULT_FROM_WIN32(ERROR_CANCELLED)) return std::nullopt;
  Require(SUCCEEDED(hr),"Error en el selector de archivos.");
  Microsoft::WRL::ComPtr<IShellItem> item; Require(SUCCEEDED(dialog->GetResult(&item)),"No hay archivo seleccionado.");
  PWSTR raw=nullptr; Require(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&raw)),"Ruta de archivo no disponible.");
  fs::path p(raw); CoTaskMemFree(raw);
  if (fs::is_regular_file(p) && Lower(p.filename().string()) == "default.xex") {
    return p.parent_path();
  }
  return p;
}

std::vector<fs::path> PickPackages(void* owner) {
  Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
  Require(SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))),"No se pudo abrir el selector DLC.");
  FILEOPENDIALOGOPTIONS flags{}; dialog->GetOptions(&flags);
  dialog->SetOptions(flags|FOS_FORCEFILESYSTEM|FOS_FILEMUSTEXIST|FOS_ALLOWMULTISELECT);
  auto title=portable::Text(L"Seleccione paquetes DLC STFS extraidos (A Voucher y/o B Voucher)"); dialog->SetTitle(title.c_str());
  auto hr=dialog->Show(reinterpret_cast<HWND>(owner)); if(hr==HRESULT_FROM_WIN32(ERROR_CANCELLED)) return {};
  Require(SUCCEEDED(hr),"Error seleccionando DLC.");
  Microsoft::WRL::ComPtr<IShellItemArray> items; Require(SUCCEEDED(dialog->GetResults(&items)),"No hay paquetes seleccionados.");
  DWORD count=0; items->GetCount(&count); Require(count<=32,"Seleccione como maximo 32 paquetes.");
  std::vector<fs::path> out;
  for(DWORD i=0;i<count;++i) {
    Microsoft::WRL::ComPtr<IShellItem> item; Require(SUCCEEDED(items->GetItemAt(i,&item)),"No se puede leer la seleccion DLC.");
    PWSTR raw=nullptr; Require(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&raw)),"Ruta DLC no disponible.");
    out.emplace_back(raw); CoTaskMemFree(raw);
  }
  return out;
}

std::optional<fs::path> Wizard(const fs::path& suggested) {
  return portable::RunWizard(suggested);
}

} // namespace iu::assets
