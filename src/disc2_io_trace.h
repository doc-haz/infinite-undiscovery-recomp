#pragma once

// Observational only: hooks preserve the SDK calls and activate after the swap
// mutex is released with current_disc == 2. XamSwapDisc is not changed.
#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <rex/system/xfile.h>
#include <rex/filesystem/devices/host_path_entry.h>
#include "disc_swap.h"
#include "disc_swap_state_trace.h"

namespace iu::disc2_io {
inline std::atomic<bool> active{false};
inline std::atomic<uint32_t> manager{0};
inline std::atomic<uint8_t*> guest_base{nullptr};
inline std::atomic<uint64_t> sequence{0};
inline std::atomic<bool> first_ud1{false};
inline std::atomic<unsigned> wait_trace_count{0}, event_trace_count{0};
inline std::jthread monitor;
struct Pending { uint32_t file, event, object, iosb; std::string path; };
struct Wait { uint32_t handle, object, lr; uint64_t started; std::string path; };
inline std::mutex records_mutex;
inline std::unordered_map<uint32_t, Pending> pending;
inline std::unordered_map<uint64_t, Wait> waits;
inline PPCFunc *create_original, *open_original, *read_original, *scatter_original,
               *wait_original, *set_original, *ke_set_original;
inline uint32_t Word(uint8_t* base, uint32_t p) {
  uint32_t value=0; iu::trace::peek32(base,p,value); return value;
}
inline uint32_t Object(uint32_t handle) {
  auto object=rex::system::kernel_state()->object_table()->LookupObject<rex::system::XObject>(handle);
  return object ? object->guest_object() : 0;
}
inline std::string Host(rex::filesystem::Entry* entry) {
  auto* host=dynamic_cast<rex::filesystem::HostPathEntry*>(entry);
  return host ? host->host_path().string() : "<unresolved/non-host>";
}
inline std::string FilePath(uint32_t handle) {
  auto file=rex::system::kernel_state()->object_table()->LookupObject<rex::system::XFile>(handle);
  return file ? Host(file->entry()) : "<not-a-file>";
}
inline void Snapshot(const char* stage) {
  auto* base=guest_base.load(); auto self=manager.load();
  if(base && self && iu::trace::readable(base,self+80))
    iu::disc_swap::LogSwap("DISC2_MANAGER stage=%s this=0x%08X state80=%u",stage,self,Word(base,self+80));
}
inline void Alias(const std::string& alias) {
  auto* fs=rex::system::kernel_state()->file_system();
  auto global_lock=rex::thread::global_critical_region::AcquireDirect();
  std::string target;
  bool found=fs->FindSymbolicLink(alias,target);
  auto* root=fs->ResolvePath(alias);
  iu::disc_swap::LogSwap("DISC2_ALIAS alias=%s target=%s resolved_host_path=%s",
    alias.c_str(),found?target.c_str():"<no-symbolic-link>",Host(root).c_str());
}
inline bool Enabled(uint8_t* base) {
  guest_base.store(base);
  if(active.load()) return true;
  {
    std::unique_lock lock(iu::disc_swap::g_state.mutex, std::try_to_lock);
    if(!lock.owns_lock()) return false; // Observer must never wait inside a swap.
    if(iu::disc_swap::g_state.current_disc!=2) return false;
  }
  if(!active.exchange(true)) {
    Snapshot("first-entry-after-swap-return");
    for(auto alias:{"game:","d:","\\Device\\Cdrom0","\\Device\\Harddisk0\\Partition1"}) Alias(alias);
  }
  return true;
}
inline std::string Name(uint8_t* base,uint32_t attrs,uint32_t& root) {
  root=Word(base,attrs);
  uint32_t name=Word(base,attrs+4), ptr=Word(base,name+4);
  uint16_t length=0;
  if(!name || !iu::trace::peek16(base,name,length) || !ptr ||
      !iu::trace::readable(base,ptr,length)) return "<unreadable>";
  return std::string(reinterpret_cast<char*>(iu::trace::host_ptr(base,ptr)),length);
}
inline void Open(PPCContext& ctx,uint8_t* base,PPCFunc* original,const char* function) {
  if(!Enabled(base)){ original(ctx,base); return; }
  auto id=++sequence;
  const uint32_t output=ctx.r3.u32, attrs=ctx.r5.u32, iosb=ctx.r6.u32, lr=ctx.lr;
  uint32_t root=0; std::string path=Name(base,attrs,root), resolution=path;
  auto* ks=rex::system::kernel_state();
  if(root && root!=0xFFFFFFFD) {
    auto file=ks->object_table()->LookupObject<rex::system::XFile>(root);
    if(file) resolution=file->entry()->absolute_path()+"\\"+path;
  }
  std::string host,device="<unresolved>",alias_target="<no-symbolic-link>";
  uint32_t selected_disc;
  {
    auto global_lock=rex::thread::global_critical_region::AcquireDirect();
    std::lock_guard state_lock(iu::disc_swap::g_state.mutex);
    selected_disc=iu::disc_swap::g_state.current_disc;
    auto* fs=ks->file_system();
    auto* entry=fs->ResolvePath(resolution);
    host=Host(entry);
    if(entry) device=entry->device()->mount_path();
    std::string alias;
    if(auto colon=path.find(':');colon!=std::string::npos) alias=path.substr(0,colon+1);
    else if(rex::string::utf8_starts_with_case(path,"\\Device\\Cdrom0")) alias="\\Device\\Cdrom0";
    if(!alias.empty()) fs->FindSymbolicLink(alias,alias_target);
  }
  iu::disc_swap::LogSwap("DISC2_OPEN_ENTER id=%llu function=%s guest_path=%s root=0x%08X resolution_path=%s alias_target=%s selected_device=%s resolved_host_path=%s active_disc=%u LR=0x%08X",
    id,function,path.c_str(),root,resolution.c_str(),alias_target.c_str(),device.c_str(),host.c_str(),selected_disc,lr);
  if(auto colon=path.find(':');colon!=std::string::npos) Alias(path.substr(0,colon+1));
  original(ctx,base);
  uint32_t status=ctx.r3.u32, handle=Word(base,output);
  if(int32_t(status)>=0 && handle) {
    auto file=ks->object_table()->LookupObject<rex::system::XFile>(handle);
    if(file) {host=Host(file->entry());device=file->device()->mount_path();}
  }
  iu::disc_swap::LogSwap("DISC2_OPEN id=%llu guest_path=%s alias_target=%s selected_device=%s resolved_host_path=%s active_disc=%u result/status=0x%08X handle=0x%08X iosb_status=0x%08X",
    id,path.c_str(),alias_target.c_str(),device.c_str(),host.c_str(),selected_disc,status,handle,Word(base,iosb));
  if(selected_disc==2 && rex::string::utf8_equal_case(path,"game:\\ud1.bin") &&
      !first_ud1.exchange(true))
    iu::disc_swap::LogSwap("DISC_FIRST_POST_SWAP_UD1 guest_path=%s alias_target=%s selected_device=%s resolved_host_path=%s status=0x%08X handle=0x%08X",
      path.c_str(),alias_target.c_str(),device.c_str(),host.c_str(),status,handle);
  Snapshot("open-return");
}
inline void Create(PPCContext& ctx,uint8_t* base){Open(ctx,base,create_original,"NtCreateFile");}
inline void OpenFile(PPCContext& ctx,uint8_t* base){Open(ctx,base,open_original,"NtOpenFile");}
inline void Read(PPCContext& ctx,uint8_t* base,PPCFunc* original,const char* function) {
  if(!Enabled(base)){original(ctx,base);return;}
  auto id=++sequence;
  uint32_t file=ctx.r3.u32,event=ctx.r4.u32,iosb=ctx.r7.u32,offset_ptr=ctx.r10.u32,size=ctx.r9.u32;
  uint64_t offset=offset_ptr ? (uint64_t(Word(base,offset_ptr))<<32)|Word(base,offset_ptr+4) : ~uint64_t(0);
  std::string path=FilePath(file);
  uint32_t event_object=event?Object(event):0;
  {
    std::lock_guard lock(records_mutex);
    pending[iosb]={file,event,event_object,iosb,path};
  }
  iu::disc_swap::LogSwap("DISC2_READ_ENTER id=%llu function=%s path=%s handle=0x%08X offset=%llu offset_ptr=0x%08X size=%u event=0x%08X object=0x%08X iosb=0x%08X apc=0x%08X apc_context=0x%08X LR=0x%08X",
    id,function,path.c_str(),file,offset,offset_ptr,size,event,event_object,iosb,ctx.r5.u32,ctx.r6.u32,uint32_t(ctx.lr));
  original(ctx,base);
  uint32_t status=ctx.r3.u32;
  iu::disc_swap::LogSwap("DISC2_READ id=%llu path=%s offset=%llu size=%u result/status=0x%08X iosb_status=0x%08X bytes=%u",
    id,path.c_str(),offset,size,status,Word(base,iosb),Word(base,iosb+4));
  if(status!=0x103) {std::lock_guard lock(records_mutex); pending.erase(iosb);}
}
inline void ReadFile(PPCContext& ctx,uint8_t* base){Read(ctx,base,read_original,"NtReadFile");}
inline void Scatter(PPCContext& ctx,uint8_t* base){Read(ctx,base,scatter_original,"NtReadFileScatter");}
inline void WaitSingle(PPCContext& ctx,uint8_t* base) {
  if(!Enabled(base) || wait_trace_count.fetch_add(1)>=32){wait_original(ctx,base);return;}
  uint64_t id=++sequence;
  uint32_t handle=ctx.r3.u32, object=Object(handle),timeout=ctx.r5.u32;
  std::string path=FilePath(handle);
  {
    std::lock_guard lock(records_mutex);
    for(auto& [key,p]:pending) if(p.event==handle || p.file==handle) {path=p.path;break;}
    waits[id]={handle,object,uint32_t(ctx.lr),GetTickCount64(),path};
  }
  iu::disc_swap::LogSwap("DISC2_WAIT_ENTER id=%llu function=NtWaitForSingleObjectEx handle=0x%08X object=0x%08X path=%s alertable=%u timeout_ptr=0x%08X LR=0x%08X",
    id,handle,object,path.c_str(),ctx.r4.u32,timeout,uint32_t(ctx.lr));
  Snapshot("wait-enter");
  wait_original(ctx,base);
  iu::disc_swap::LogSwap("DISC2_WAIT_RETURN id=%llu result/status=0x%08X",id,ctx.r3.u32);
  {std::lock_guard lock(records_mutex);waits.erase(id);}
}
inline void Set(PPCContext& ctx,uint8_t* base,PPCFunc* original,bool native) {
  if(!Enabled(base) || event_trace_count.fetch_add(1)>=32){original(ctx,base);return;}
  uint32_t arg=ctx.r3.u32,object=native?arg:Object(arg);
  iu::disc_swap::LogSwap("DISC2_EVENT_SET function=%s handle_or_object=0x%08X object=0x%08X LR=0x%08X",
    native?"KeSetEvent":"NtSetEvent",arg,object,uint32_t(ctx.lr));
  original(ctx,base);
}
inline void NtSet(PPCContext& ctx,uint8_t* base){Set(ctx,base,set_original,false);}
inline void KeSet(PPCContext& ctx,uint8_t* base){Set(ctx,base,ke_set_original,true);}
inline void Shutdown() {
  if(monitor.joinable()){monitor.request_stop();monitor.join();}
}
extern "C" void __imp__sub_824937D8(PPCContext&,uint8_t*);
inline void Install(rex::Runtime* runtime) {
  iu::swap_state::runtime=runtime;
  struct Hook { const char* name; uint32_t address; PPCFunc* hook; PPCFunc** original; };
  for(auto h: {Hook{"__imp__NtCreateFile",0x8296853C,Create,&create_original},
    Hook{"__imp__NtOpenFile",0x829684AC,OpenFile,&open_original},
    Hook{"__imp__NtReadFile",0x829685AC,ReadFile,&read_original},
    Hook{"__imp__NtReadFileScatter",0x8296859C,Scatter,&scatter_original},
    Hook{"__imp__NtWaitForSingleObjectEx",0x82967EEC,WaitSingle,&wait_original},
    Hook{"__imp__NtSetEvent",0x82967ECC,NtSet,&set_original},
    Hook{"__imp__KeSetEvent",0x829683FC,KeSet,&ke_set_original}}) {
    *h.original=reinterpret_cast<PPCFunc*>(GetProcAddress(GetModuleHandleW(L"rexruntime.dll"),h.name));
    if(!*h.original) {iu::disc_swap::LogSwap("DISC2_TRACE_HOOK_FAILED name=%s",h.name);continue;}
    bool fd=runtime->function_dispatcher()->SetFunction(h.address,h.hook);
    bool iat=iu::disc_swap::PatchIAT("rexruntime.dll",h.name,reinterpret_cast<void*>(h.hook));
    iu::disc_swap::LogSwap("DISC2_TRACE_HOOK name=%s fd=%d iat=%d",h.name,fd,iat);
  }
  monitor=std::jthread([](std::stop_token stop){
    while(!stop.stop_requested()){
      for(int i=0;i<10 && !stop.stop_requested();++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if(!active.load() || stop.stop_requested())continue;
      iu::swap_state::Parked(guest_base.load());
      std::lock_guard lock(records_mutex);
      for(auto& [id,w]:waits) if(GetTickCount64()-w.started>=1000)
        iu::disc_swap::LogSwap("DISC2_WAIT_PENDING id=%llu handle=0x%08X object=0x%08X path=%s elapsed_ms=%llu LR=0x%08X",
          id,w.handle,w.object,w.path.c_str(),GetTickCount64()-w.started,w.lr);
      for(auto it=pending.begin();it!=pending.end();){
        uint32_t status=Word(guest_base.load(),it->second.iosb);
        iu::disc_swap::LogSwap("DISC2_IO_COMPLETION iosb=0x%08X path=%s event=0x%08X object=0x%08X status=0x%08X",
          it->first,it->second.path.c_str(),it->second.event,it->second.object,status);
        if(status!=0x103) it=pending.erase(it);else ++it;
      }
    }
  });
}
} // namespace iu::disc2_io

extern "C" void sub_824937D8(PPCContext& ctx,uint8_t* base) {
  const uint32_t self=ctx.r3.u32;
  iu::disc2_io::guest_base.store(base);
  iu::swap_state::Track(base,self);
  if(self==iu::swap_state::task.load())iu::disc2_io::manager.store(self);
  const uint32_t before=iu::disc2_io::Word(base,self+80);

  iu::disc2_io::__imp__sub_824937D8(ctx,base);
  iu::swap_state::Observe(base,self,"sub_824937D8-return");
  if(iu::disc2_io::Enabled(base)){
    const uint32_t after=iu::disc2_io::Word(base,self+80);
    if(before!=after)iu::disc_swap::LogSwap("DISC2_MANAGER_TRANSITION this=0x%08X before=%u after=%u",self,before,after);
  }
}

