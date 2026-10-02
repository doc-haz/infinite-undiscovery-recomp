#include "asset_setup.h"
#include "asset_dlc.h"
#include "asset_hash.h"
#include <rex/runtime.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/content_manager.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <commctrl.h>
#include <atomic>
#include <thread>

namespace fs=std::filesystem;
using iu::assets::Inspect;
void Check(bool ok,const char* reason) { if(!ok)throw std::runtime_error(reason); }
template<class F> void Rejected(const char* name,F fn) {
  try { fn(); } catch(const std::exception& e) { std::cout<<"PASS rejected "<<name<<": "<<e.what()<<'\n'; return; }
  throw std::runtime_error(std::string("Expected rejection: ")+name);
}
void Print(const iu::assets::Disc& d) {
  std::cout<<"disc="<<d.number<<" edition="<<d.edition<<" title="<<std::hex<<d.title<<std::dec<<" media="<<d.media_id<<" xex="<<d.xex_sha256<<'\n';
}
void RuntimeDlc(const fs::path& game,const fs::path& user,const fs::path& packages) {
  rex::Runtime rt(game,user); rex::RuntimeConfig cfg;cfg.tool_mode=true;
  Check(rt.Setup(std::move(cfg))==0,"Runtime setup failed");
  Check(rt.LoadXexImage("game:\\default.xex")==0,"XEX load failed");
  Check(rt.kernel_state()->title_id()==0x535107db,"Wrong runtime title");
  iu::dlc::InstallPending(packages,rt.kernel_state(),user);
  auto manager=rt.kernel_state()->content_manager();
  auto list=manager->ListContent(1,0,rex::system::XContentType::kMarketplaceContent,0x535107db);
  for(auto& p:list) { uint32_t mask=0;Check(manager->OpenContent("test",0,p,mask)==0,"OpenContent failed");
    manager->CloseContent("test");std::cout<<"DLC "<<p.file_name()<<" mask="<<std::hex<<mask<<std::dec<<'\n'; }
  rt.Shutdown();std::cout<<"PASS runtime load and DLC; count="<<list.size()<<'\n';
}
int wmain(int argc,wchar_t** argv) {
  try {
    rex::InitLogging(nullptr,spdlog::level::warn);
    if(argc<3)return 2;
    std::wstring mode=argv[1];
    if(mode==L"exe-cancel" && argc==4) {
      fs::create_directories(argv[3]);
      auto testroot=fs::absolute(argv[3]);
      std::wstring command=L"\""+fs::absolute(argv[2]).wstring()+L"\" --game_data_root \""+
        (testroot/L"missing").wstring()+L"\" --user_data_root \""+(testroot/L"user").wstring()+
        L"\" --log_file \""+(testroot/L"runtime.log").wstring()+L"\"";
      STARTUPINFOW start{sizeof(start)};start.dwFlags=STARTF_USESHOWWINDOW;start.wShowWindow=SW_HIDE;
      PROCESS_INFORMATION child{};
      Check(CreateProcessW(nullptr,command.data(),nullptr,nullptr,FALSE,0,nullptr,nullptr,&start,&child)!=0,"Cannot launch test EXE");
      struct State {DWORD pid;bool found=false;} state{child.dwProcessId};
      DWORD wait=WAIT_TIMEOUT;
      for(unsigned i=0;i<200 && wait==WAIT_TIMEOUT;++i) {
        EnumWindows([](HWND hwnd,LPARAM opaque)->BOOL {
          auto& state=*reinterpret_cast<State*>(opaque);DWORD pid=0;GetWindowThreadProcessId(hwnd,&pid);
          wchar_t title[256]{};GetWindowTextW(hwnd,title,256);
          if(pid==state.pid && std::wstring(title)==L"Infinite Undiscovery - Asset Setup") {
            state.found=true;PostMessageW(hwnd,TDM_CLICK_BUTTON,IDCANCEL,0);
          }
          return TRUE;
        },reinterpret_cast<LPARAM>(&state));
        wait=WaitForSingleObject(child.hProcess,100);
      }
      if(wait==WAIT_TIMEOUT)TerminateProcess(child.hProcess,1);
      CloseHandle(child.hThread);CloseHandle(child.hProcess);
      Check(state.found && wait==WAIT_OBJECT_0,"EXE wizard did not open/cancel and exit");
      std::cout<<"PASS public EXE with missing assets opened native wizard and exited on cancel\n";return 0;
    }
    if(mode==L"wizard-cancel") {
      std::atomic<bool> found=false,finished=false;
      std::thread driver([&]{
        for(unsigned i=0;i<150 && !finished.load();++i) {
          EnumWindows([](HWND hwnd,LPARAM state)->BOOL {
            DWORD pid=0;GetWindowThreadProcessId(hwnd,&pid);
            wchar_t title[256]{};GetWindowTextW(hwnd,title,256);
            if(pid==GetCurrentProcessId() && std::wstring(title)==L"Infinite Undiscovery - Asset Setup") {
              *reinterpret_cast<std::atomic<bool>*>(state)=true;
              PostMessageW(hwnd,TDM_CLICK_BUTTON,IDCANCEL,0);
            }
            return TRUE;
          },reinterpret_cast<LPARAM>(&found));
          Sleep(100);
        }
      });
      std::optional<fs::path> result;
      try { result=iu::assets::Wizard(argv[2]); }
      catch(...) {finished=true;driver.join();throw;}
      finished=true;driver.join();
      Check(found && !result,"Native wizard did not cancel");
      std::cout<<"PASS native wizard opened and cancelled without installing\n";return 0;
    }
    if(mode==L"inspect") {Print(Inspect(argv[2]));return 0;}
    if(mode==L"dlc") {auto p=iu::dlc::Inspect(argv[2]); std::wcout<<p.display_name<<L'\n';std::cout<<p.sha256<<" mask="<<std::hex<<p.license_mask<<'\n';return 0;}
    if(mode==L"runtime" && argc==5){RuntimeDlc(argv[2],argv[3],argv[4]);return 0;}
    if(mode==L"ready") { std::string why; bool ok=iu::assets::Ready(argv[2],&why); std::cout<<"ready="<<ok<<" "<<why<<'\n';return ok?0:1;}
   if(mode==L"install" && argc==6) {
  auto first=Inspect(argv[2]); std::optional<iu::assets::Disc> second;
  if(std::wstring(argv[3])!=L"-")second=Inspect(argv[3]);
  std::vector<iu::dlc::Package> packages;
  if(std::wstring(argv[5])!=L"-")for(const auto& file:fs::directory_iterator(argv[5]))packages.push_back(iu::dlc::Inspect(file.path()));
  auto root=iu::assets::Install(first,second,argv[4],[](uint64_t,uint64_t){},[]{return false;},packages);
  std::wcout<<root.wstring()<<L'\n';
  Check(iu::assets::Ready(root),"Installed assets not ready");
  return 0;
}
    if(mode==L"suite" && argc==7) {
      auto first=Inspect(argv[2]),usa=Inspect(argv[3]); Print(first);Print(usa);
      Check(first.edition=="PAL" && usa.edition=="USA","Region detection failed");
      Check(iu::assets::CanLaunch(first) && iu::assets::CanLaunch(usa),"Known XEX profile rejected");
      auto usa2=Inspect(fs::path(argv[3]).parent_path()/"disc2");Print(usa2);iu::assets::ValidatePair(usa,usa2);
      auto pal2=Inspect(argv[4]);Print(pal2);iu::assets::ValidatePair(first,pal2);
      Rejected("mixed regions",[&]{iu::assets::ValidatePair(first,usa2);});
      Rejected("wrong disc slot",[&]{iu::assets::ValidatePair(usa2,usa);});
      auto test=fs::absolute(argv[6]);fs::create_directories(test);
      Rejected("invalid path",[&]{Inspect(test/"missing");});
      Check(!iu::assets::Ready(test),"Missing assets incorrectly ready");std::cout<<"PASS absent assets\n";
      auto invalid_iso=test/"truncated.iso";
      {std::ofstream out(invalid_iso,std::ios::binary);out.write("XEX2",4);}
      Rejected("truncated disc image",[&]{Inspect(invalid_iso);});
      auto foreign=test/"named-USA-but-PAL";fs::create_directories(foreign);
      auto fixture=foreign/"default.xex";
      fs::copy_file(first.source/"default.xex",fixture,fs::copy_options::overwrite_existing);
      for(const auto name:{"ud1.bin","ud2.bin"})if(!fs::exists(foreign/name))fs::create_hard_link(first.source/name,foreign/name);
      Check(Inspect(foreign).edition=="PAL","Folder name affected region detection");
      {std::fstream out(fixture,std::ios::binary|std::ios::in|std::ios::out);out.seekg(-1,std::ios::end);char v=0;out.get(v);out.seekp(-1,std::ios::end);out.put(v^1);}
      auto unknown=Inspect(foreign);
      Check(!iu::assets::CanLaunch(unknown),"Modified XEX accepted as a known build");
      Rejected("modified XEX build",[&]{iu::assets::Install(unknown,pal2,test,[](uint64_t,uint64_t){},[]{return false;});});
      fs::copy_file(first.source/"default.xex",fixture,fs::copy_options::overwrite_existing);
      {std::fstream out(fixture,std::ios::binary|std::ios::in|std::ios::out);
        auto be=[&](uint64_t pos){unsigned char b[4]{};out.seekg(pos);out.read(reinterpret_cast<char*>(b),4);return uint32_t(b[0])<<24|uint32_t(b[1])<<16|uint32_t(b[2])<<8|b[3];};
        auto count=be(20);bool changed=false;
        for(uint32_t i=0;i<count;++i)if(be(24+i*8)==0x40006){auto off=be(28+i*8);out.seekp(off+12);out.put('\0');changed=true;break;}
        Check(changed,"No execution info in test fixture");}
      Rejected("foreign disc title",[&]{Inspect(foreign);});
      auto changed_source=first;
      for(auto& file:changed_source.files)if(file.name=="ud1.bin")file.host=invalid_iso;
      Rejected("source truncated after inspection",[&]{iu::assets::Install(changed_source,pal2,test,[](uint64_t,uint64_t){},[]{return false;});});
      std::vector<iu::dlc::Package> dlc;
      for(const auto& file:fs::directory_iterator(argv[5])) { auto p=iu::dlc::Inspect(file.path()); std::cout<<"PASS DLC mask="<<std::hex<<p.license_mask<<std::dec<<'\n';dlc.push_back(p); }
      Check(dlc.size()==2,"Expected A/B");iu::dlc::ValidateSelection(dlc);
      Rejected("duplicate DLC",[&]{auto dup=dlc;dup.push_back(dlc[0]);iu::dlc::ValidateSelection(dup);});
      auto bad=test/"bad-dlc";fs::copy_file(dlc[0].source,bad,fs::copy_options::overwrite_existing);
      {std::fstream out(bad,std::ios::binary|std::ios::in|std::ios::out);out.seekp(0xd000);out.put('\0');}
      Rejected("corrupt DLC block",[&]{iu::dlc::Inspect(bad);});
      fs::copy_file(dlc[0].source,bad,fs::copy_options::overwrite_existing);
      {std::fstream out(bad,std::ios::binary|std::ios::in|std::ios::out);out.seekp(0x360);out.put('\0');}
      Rejected("foreign DLC title",[&]{iu::dlc::Inspect(bad);});
      {std::ofstream out(bad,std::ios::binary);out<<"LIVE";}
      Rejected("truncated DLC",[&]{iu::dlc::Inspect(bad);});
      bool cancel=false; auto count=[&]{size_t n=0;for(const auto& e:fs::directory_iterator(test))if(e.is_directory())++n;return n;};auto before=count();
      Rejected("cancel during copy",[&]{iu::assets::Install(first,pal2,test,[&](uint64_t,uint64_t){cancel=true;},[&]{return cancel;},dlc);});
      Check(count()==before,"Cancellation left staging behind");std::cout<<"PASS cancelled staging cleaned\n";
      RuntimeDlc(first.source,test/"runtime-pal",argv[5]);
      RuntimeDlc(first.source,test/"runtime-pal",argv[5]);std::cout<<"PASS DLC idempotence/reopen\n";
      RuntimeDlc(usa.source,test/"runtime-usa",argv[5]);
      RuntimeDlc(first.source,test/"runtime-no-dlc",test/"no-dlc");
      auto conflict=test/"runtime-pal"/"0000000000000000"/"535107DB"/"00000002"/dlc[0].filename/"banner_t03.png";
      {std::fstream out(conflict,std::ios::binary|std::ios::in|std::ios::out);out.put('\0');}auto hash=iu::assets::HashFile(conflict);
      Rejected("existing DLC conflict",[&]{RuntimeDlc(first.source,test/"runtime-pal",argv[5]);});
      Check(iu::assets::HashFile(conflict)==hash,"Existing DLC overwritten");std::cout<<"PASS existing conflicting DLC preserved\n";
      return 0;
    }
    return 2;
  } catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
