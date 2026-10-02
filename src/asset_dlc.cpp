#include "asset_dlc.h"
#include "asset_hash.h"
#include "portable_setup.h"
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/content_manager.h>
#include <algorithm>
#include <chrono>
#include <map>
#include <set>

namespace iu::dlc {
namespace fs = std::filesystem;
namespace {
void Check(bool ok, const char* error) { if (!ok) throw std::runtime_error(portable::Text(error)); }
uint32_t BE(const uint8_t* p) { return uint32_t(p[0])<<24 | uint32_t(p[1])<<16 | uint32_t(p[2])<<8 | p[3]; }
uint32_t LE24(const uint8_t* p) { return uint32_t(p[2])<<16 | uint32_t(p[1])<<8 | p[0]; }
std::string Hash(const void* p, size_t n, bool sha1 = false) { assets::DigestState h(sha1); h.Add(p,n); return h.Finish(); }
std::string Hex(const uint8_t* p,size_t n) {
  std::ostringstream s; s<<std::hex<<std::setfill('0'); for(size_t i=0;i<n;++i) s<<std::setw(2)<<unsigned(p[i]); return s.str();
}
bool SafeName(const std::string& n) {
  if(n.empty() || n.size()>42 || n=="." || n==".." || n.back()=='.' || n.back()==' ') return false;
  for(auto c:n) if(c<32 || std::string("/\\:*?\"<>|").find(c)!=std::string::npos) return false;
  std::string base=n.substr(0,n.find('.')); for(auto& c:base) c=static_cast<char>(toupper(static_cast<unsigned char>(c)));
  if(base=="CON" || base=="PRN" || base=="AUX" || base=="NUL") return false;
  if(base.size()==4 && (base.starts_with("COM") || base.starts_with("LPT")) && base[3]>='1' && base[3]<='9') return false;
  return true;
}
bool SameFile(const fs::path& a,const fs::path& b) {
  return fs::is_regular_file(a) && fs::is_regular_file(b) && fs::file_size(a)==fs::file_size(b) && assets::HashFile(a)==assets::HashFile(b);
}
void NoLinks(const fs::path& root) {
  for(auto p=fs::absolute(root); !p.empty();) {
    auto attributes=GetFileAttributesW(p.c_str());
    Check(attributes==INVALID_FILE_ATTRIBUTES || !(attributes&FILE_ATTRIBUTE_REPARSE_POINT),"El destino contiene un enlace o junction; elija otra carpeta.");
    auto parent=p.parent_path(); if(parent==p) break; p=parent;
  }
}
}
Package Inspect(const fs::path& source) {
  Check(fs::is_regular_file(source),"El DLC debe ser un paquete STFS, no RAR ni carpeta.");
  auto size=fs::file_size(source); Check(size>=0x971a && size<=2*1024*1024,"DLC truncado o formato STFS grande no soportado por esta etapa.");
  std::ifstream in(source,std::ios::binary); std::vector<uint8_t> b(static_cast<size_t>(size));
  in.read(reinterpret_cast<char*>(b.data()),b.size()); Check(bool(in),"No se pudo leer el DLC.");
  Check(!memcmp(b.data(),"LIVE",4) || !memcmp(b.data(),"PIRS",4) || !memcmp(b.data(),"CON ",4),"No es un contenedor STFS compatible.");
  Check(BE(b.data()+0x360)==0x535107db,"El DLC no corresponde a Infinite Undiscovery (Title ID 535107DB).");
  Check(BE(b.data()+0x344)==2,"El paquete no es DLC Marketplace Content.");
  Check(BE(b.data()+0x348)==1 || BE(b.data()+0x348)==2,"Version STFS no soportada.");
  Check(BE(b.data()+0x3a9)==0 && b[0x379]==36 && (b[0x37b]&1) && !(b[0x37b]&2),"Volumen STFS no soportado; se requiere formato read-only con tabla primaria.");
  auto header=BE(b.data()+0x340); uint64_t base=(uint64_t(header)+4095)&~4095ull;
  Check(header>=0x971a && base<=size && size-base>=4096,"Cabecera STFS truncada.");
  auto total=BE(b.data()+0x395); Check(total>0 && total<170 && uint64_t(total+1)*4096<=size-base,"Bloques STFS truncados o estructura grande no soportada.");
  Package p; p.source=fs::absolute(source); p.filename=source.filename().string(); p.size=size;
  Check(SafeName(p.filename),"Nombre de paquete DLC no seguro para el sistema de archivos.");
  p.content_id=Hex(b.data()+0x32c,20); p.sha256=Hash(b.data(),b.size());
  Check(Hash(b.data()+0x344,base-0x344,true)==p.content_id,"El hash de cabecera del DLC no coincide.");
  Check(Hash(b.data()+base,4096,true)==Hex(b.data()+0x381,20),"La tabla hash del DLC esta corrupta.");
  const uint8_t* hashes=b.data()+base;
  for(uint32_t i=0;i<total;++i) Check(Hash(b.data()+base+(uint64_t(i)+1)*4096,4096,true)==Hex(hashes+i*24,20),"Bloque de datos DLC corrupto.");
  for(unsigned i=0;i<128;++i) { auto c=uint16_t(b[0x411+i*2])<<8 | b[0x412+i*2]; if(!c) break; p.display_name.push_back(static_cast<wchar_t>(c)); }
  for(unsigned i=0;i<16;++i) { const auto* license=b.data()+0x22c+i*16; if(BE(license+12)) p.license_mask|=BE(license+8); }
  auto tables=unsigned(b[0x37c]) | unsigned(b[0x37d])<<8;
  Check(tables>0 && tables<=total,"Tabla de archivos STFS invalida.");
  std::set<uint32_t> used; std::set<std::string> names;
  auto claim=[&](uint32_t block) {
    Check(block<total && used.insert(block).second,"Cadena STFS invalida, ciclica o con bloques compartidos.");
    return b.data()+base+(uint64_t(block)+1)*4096;
  };
  uint32_t directory=LE24(b.data()+0x37e);
  std::vector<std::array<uint8_t,64>> entries;
  for(unsigned n=0;n<tables;++n) {
    const auto* table=claim(directory);
    for(unsigned i=0;i<64;++i) { std::array<uint8_t,64> e{}; memcpy(e.data(),table+i*64,64); entries.push_back(e); }
    directory=BE(hashes+directory*24+20)&0xffffff;
  }
  for(const auto& entry:entries) {
    const auto* e=entry.data(); auto n=e[40]&63; if(!n) continue;
    Check(n<=40 && !(e[40]&128) && e[50]==0xff && e[51]==0xff,"Subdirectorios de DLC no soportados en esta etapa.");
    Entry f; f.name=std::string(reinterpret_cast<const char*>(e),n); f.size=BE(e+52);
    Check(SafeName(f.name),"Ruta interna DLC no segura.");
    std::string lower=f.name; for(auto& c:lower) c=static_cast<char>(tolower(static_cast<unsigned char>(c)));
    Check(names.insert(lower).second,"Archivos DLC duplicados.");
    assets::DigestState h; uint32_t next=LE24(e+47), remaining=f.size;
    auto blocks=(uint64_t(f.size)+4095)/4096;
    Check(blocks==LE24(e+41) && blocks<=LE24(e+44),"Longitud de archivo DLC inconsistente.");
    while(remaining) {
      auto data=claim(next); unsigned bytes=std::min(remaining,4096u); h.Add(data,bytes); remaining-=bytes;
      next=(e[40]&64) ? next+1 : BE(hashes+next*24+20)&0xffffff;
    }
    f.sha256=h.Finish(); p.entries.push_back(std::move(f));
  }
  Check(!p.entries.empty(),"DLC sin archivos validos."); return p;
}
void ValidateSelection(const std::vector<Package>& packages) {
  std::map<std::string,std::string> identities; std::set<std::string> names;
  for(const auto& p:packages) {
    Check(!identities.contains(p.content_id),"El mismo contenido DLC fue seleccionado mas de una vez.");
    identities.emplace(p.content_id,p.sha256);
    auto name=p.filename; for(auto& c:name)c=static_cast<char>(tolower(static_cast<unsigned char>(c)));
    Check(names.insert(name).second,"Dos paquetes DLC tienen el mismo nombre de destino.");
  }
}
void InstallPending(const fs::path& packages,rex::system::KernelState* kernel,const fs::path& user_root) {
  if(!fs::is_directory(packages)) return;
  Check(kernel && kernel->title_id()==0x535107db,"El runtime no tiene cargado el titulo Infinite Undiscovery.");
  std::vector<Package> list; for(const auto& entry:fs::directory_iterator(packages)) list.push_back(Inspect(entry.path()));
  ValidateSelection(list); if(list.empty())return;
  auto active=kernel->content_manager(); Check(active!=nullptr,"El gestor de contenido no esta disponible.");
  NoLinks(user_root); fs::create_directories(user_root);
  // Validate existing installations semantically through the SDK. Its serialized
  // header contains padding, so byte equality with a fresh header is not identity.
  const fs::path content_suffix=fs::path("0000000000000000")/"535107DB";
  std::vector<Package> missing;
  for(const auto& p:list) {
    auto dir=user_root/content_suffix/"00000002"/p.filename;
    auto header=user_root/content_suffix/"Headers"/"00000002"/(p.filename+".header");
    NoLinks(dir);NoLinks(header);
    if(!fs::exists(dir) && !fs::exists(header)){missing.push_back(p);continue;}
    Check(fs::is_directory(dir) && fs::is_regular_file(header),"Conflicto: existe una instalacion DLC incompleta; se conserva intacta.");
    rex::system::xam::XCONTENT_AGGREGATE_DATA data{};
bool read_ok = (active->ReadContentHeaderFile(
    p.filename,
    0,
    0x535107db,
    rex::system::XContentType::kMarketplaceContent,
    data) == 0);

Check(read_ok,
      "Conflicto: la cabecera o licencia DLC existente es diferente.");

Check(data.title_id == 0x535107db &&
      data.content_type == rex::system::XContentType::kMarketplaceContent &&
      data.xuid == 0 &&
      data.file_name() == p.filename,
      "Licencia o identidad DLC no coincide.");

auto display = data.display_name();
Check(std::wstring(display.begin(), display.end()) == p.display_name,
      "Licencia o identidad DLC no coincide.");
    size_t count=0;
    for(const auto& file:fs::directory_iterator(dir)){NoLinks(file.path());Check(file.is_regular_file(),"Conflicto: contenido DLC existente inesperado.");++count;}
    Check(count==p.entries.size(),"Conflicto: archivos DLC existentes distintos.");
    for(const auto& file:p.entries)Check(fs::is_regular_file(dir/file.name) && fs::file_size(dir/file.name)==file.size && assets::HashFile(dir/file.name)==file.sha256,"Conflicto: un archivo DLC existente es diferente; no se sobrescribira.");
uint32_t mask = 0;
auto result = active->OpenContent("iu_dlc_reuse", 0, data, mask);
if (result == 0) active->CloseContent("iu_dlc_reuse");
Check(result == 0 && mask == p.license_mask,
      "Licencia o identidad DLC no coincide.");
  }
  if(missing.empty())return;
  list=std::move(missing);
  auto stage=user_root/(".iu-dlc-stage-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Check(fs::create_directory(stage),"No se puede reservar staging del DLC.");
  struct Move { fs::path from,to; }; std::vector<Move> moves; std::vector<fs::path> published;
  try {
    {
      rex::system::xam::ContentManager isolated(kernel,stage);
      for(const auto& p:list) {
        Check(isolated.InstallContent(p.source)==0,"El SDK rechazo la instalacion del DLC.");
        Check(kernel->content_manager()==active,"El staging altero el gestor de contenido activo.");
      }
      auto found=isolated.ListContent(1,0,rex::system::XContentType::kMarketplaceContent,0x535107db);
      Check(found.size()==list.size(),"El SDK no enumera todos los DLC preparados.");
      for(const auto& p:list) {
        auto it=std::find_if(found.begin(),found.end(),[&](const auto& f){return f.file_name()==p.filename;});
        Check(it!=found.end(),"Falta un DLC en la enumeracion."); uint32_t mask=0;
        auto result=isolated.OpenContent("iu_dlc_validate",0,*it,mask);
        Check(result==0,"El SDK no puede abrir el DLC preparado.");
        isolated.CloseContent("iu_dlc_validate"); Check(mask==p.license_mask,"El SDK no preservo la licencia DLC.");
      }
    }
    const fs::path suffix=fs::path("0000000000000000")/"535107DB";
    // Preflight every package before publishing any. Headers are published last.
    std::vector<Move> headers;
    for(const auto& p:list) {
      auto rel=suffix/"00000002"/p.filename;
      auto header=suffix/"Headers"/"00000002"/(p.filename+".header");
      auto dir=user_root/rel; auto hdr=user_root/header; NoLinks(dir); NoLinks(hdr);
      auto identity=p.content_id; for(auto& c:identity)c=static_cast<char>(toupper(static_cast<unsigned char>(c)));
      const auto container_root=user_root/suffix/"00000002";
      if(fs::is_directory(container_root)) for(const auto& existing:fs::directory_iterator(container_root)) {
        auto name=existing.path().filename().string(); for(auto& c:name)c=static_cast<char>(toupper(static_cast<unsigned char>(c)));
        Check(!name.starts_with(identity) || existing.path().filename()==fs::path(p.filename),"Conflicto: el Content ID ya existe con otro nombre de paquete.");
      }
  Check(!fs::exists(dir) && !fs::exists(hdr),
      "Conflicto: el destino DLC cambio durante la instalacion.");

moves.push_back({stage/rel, dir});
headers.push_back({stage/header, hdr});
}
    moves.insert(moves.end(),headers.begin(),headers.end());
    for(const auto& m:moves) {
      NoLinks(m.to.parent_path()); fs::create_directories(m.to.parent_path());
      Check(!fs::exists(m.to),"Conflicto: el destino DLC cambio durante la instalacion.");
      Check(MoveFileExW(m.from.c_str(),m.to.c_str(),MOVEFILE_WRITE_THROUGH)!=0,"No se pudo publicar el DLC sin sobrescribir.");
      published.push_back(m.to);
    }
    // Same SDK manager observes the new files; no cache assumptions.
    auto found=active->ListContent(1,0,rex::system::XContentType::kMarketplaceContent,0x535107db);
    for(const auto& p:list) {
      auto it=std::find_if(found.begin(),found.end(),[&](const auto& f){return f.file_name()==p.filename;});
      Check(it!=found.end(),"El runtime no enumera el DLC publicado."); uint32_t mask=0;
      auto result=active->OpenContent("iu_dlc_installed",0,*it,mask);
      Check(result==0,"El runtime no puede abrir el DLC publicado."); active->CloseContent("iu_dlc_installed");
      Check(mask==p.license_mask,"La licencia DLC publicada no coincide.");
    }
    // stage is the uniquely created child above; never clean a supplied path.
    fs::remove_all(stage);
  } catch(...) {
    std::error_code ignored;
    for(auto it=published.rbegin();it!=published.rend();++it) fs::remove_all(*it,ignored);
    fs::remove_all(stage,ignored); throw;
  }
}
}
