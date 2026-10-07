#pragma once
#include "disc_swap_state_trace.h"
#include <unordered_set>
namespace iu::package_trace {
inline uint32_t requested_sub=0,load_task=0;
inline uint64_t last_log=0;
inline unsigned samples=0;
struct Match {uint32_t node=0,state=0;bool complete=false;};
inline Match Find(uint8_t* base,uint32_t manager,uint32_t kind,uint32_t sub){
  Match out;std::unordered_set<uint32_t> groups,nodes;
  uint32_t link=iu::swap_state::Read(base,manager+12);unsigned budget=4096;
  while(link!=manager+4){
    if(link<24 || !groups.insert(link).second || !budget-- || !iu::trace::readable(base,link-24,36))return out;
    const uint32_t group=link-24,sentinel=group+4;uint32_t node=iu::swap_state::Read(base,group+12);
    while(node!=sentinel){
      if(!node || !nodes.insert(node).second || !budget-- || !iu::trace::readable(base,node,172))return out;
      if(iu::swap_state::Read(base,node+168)==kind && iu::swap_state::Read(base,node+160)==sub){out.node=node;out.state=iu::swap_state::Read(base,node+164);out.complete=true;return out;}
      node=iu::swap_state::Read(base,node+8);
    }
    link=iu::swap_state::Read(base,group+32);
  }
  out.complete=true;return out;
}
}
extern "C" void __imp__sub_826CBED8(PPCContext&,uint8_t*);
extern "C" void sub_826CBED8(PPCContext& ctx,uint8_t* base){
  const uint32_t lr=uint32_t(ctx.lr),manager=ctx.r3.u32,kind=ctx.r4.u32,sub=ctx.r5.u32,flag=ctx.r6.u32,self=ctx.r31.u32;
  const bool relevant=(lr==0x82637084 || lr==0x82636F68) && iu::swap_state::task.load();
  __imp__sub_826CBED8(ctx,base);
  if(!relevant)return;
  const uint64_t now=GetTickCount64();
  const bool changed=iu::package_trace::requested_sub!=sub || iu::package_trace::load_task!=self;
  if(!changed && (iu::package_trace::samples>=15 || now-iu::package_trace::last_log<1000))return;
  if(changed)iu::package_trace::samples=0;
  iu::package_trace::requested_sub=sub;iu::package_trace::load_task=self;iu::package_trace::last_log=now;++iu::package_trace::samples;
  const auto match=iu::package_trace::Find(base,manager,kind,sub);
  const uint32_t busy_owner=iu::swap_state::Read(base,0x82A58EA8+16);
  iu::disc_swap::LogSwap("NOW_LOADING_PACKAGE_QUERY load_task=0x%08X requested_sub=%u kind=%u flag=%u result=%u manager=0x%08X global_manager=0x%08X task_sub40=%u busy_owner=0x%08X busy=%u LR=0x%08X",self,sub,kind,flag,ctx.r3.u32,manager,iu::swap_state::Read(base,0x82A380F8+28),iu::swap_state::Read(base,self+64),busy_owner,iu::swap_state::Byte(base,busy_owner+88),lr);
  if(match.node)iu::disc_swap::LogSwap("NOW_LOADING_PACKAGE_NODE node=0x%08X kind=%u sub=%u state=%u vtable=0x%08X owner12=0x%08X data140=0x%08X data144=0x%08X",match.node,iu::swap_state::Read(base,match.node+168),iu::swap_state::Read(base,match.node+160),match.state,iu::swap_state::Read(base,match.node),iu::swap_state::Read(base,match.node+12),iu::swap_state::Read(base,match.node+140),iu::swap_state::Read(base,match.node+144));
  else iu::disc_swap::LogSwap(match.complete?"NOW_LOADING_PACKAGE_NODE_MISSING kind=%u sub=%u manager=0x%08X":"NOW_LOADING_PACKAGE_SCAN_INCOMPLETE kind=%u sub=%u manager=0x%08X",kind,sub,manager);
}
extern "C" void __imp__sub_826CCC38(PPCContext&,uint8_t*);
extern "C" void sub_826CCC38(PPCContext& ctx,uint8_t* base){
  const uint32_t manager=ctx.r3.u32,kind=ctx.r9.u32,sub=ctx.r10.u32,lr=uint32_t(ctx.lr);
  const bool relevant=kind==3 && iu::swap_state::task.load();
  if(relevant)iu::disc_swap::LogSwap("NOW_LOADING_PACKAGE_ENQUEUE_ENTRY function=0x826CCC38 manager=0x%08X kind=%u sub=%u r4=0x%08X r5=0x%08X r6=0x%08X r7=0x%08X r8=0x%08X LR=0x%08X",manager,kind,sub,ctx.r4.u32,ctx.r5.u32,ctx.r6.u32,ctx.r7.u32,ctx.r8.u32,lr);
  __imp__sub_826CCC38(ctx,base);
  if(relevant){const uint32_t node=ctx.r3.u32;iu::disc_swap::LogSwap("NOW_LOADING_PACKAGE_ENQUEUE_RETURN function=0x826CCC38 node=0x%08X kind=%u sub=%u state=%u",node,kind,sub,node?iu::swap_state::Read(base,node+164):0);}
}
extern "C" void __imp__sub_82636F20(PPCContext&,uint8_t*);
extern "C" void sub_82636F20(PPCContext& ctx,uint8_t* base){
  const uint32_t self=ctx.r3.u32,owner=iu::swap_state::Read(base,0x82A58EA8+608),vt=iu::swap_state::Read(base,owner);
  const bool relevant=iu::swap_state::task.load() && !iu::swap_state::Byte(base,self+76);
  if(relevant)iu::disc_swap::LogSwap("NOW_LOADING_PACKAGE_PREPARE load_task=0x%08X requested_sub=%u request_sent=%u owner=0x%08X vtable=0x%08X request_target=0x%08X arg5=0x%08X arg6=0x%08X",self,iu::swap_state::Read(base,self+64),iu::swap_state::Byte(base,self+76),owner,vt,iu::swap_state::Read(base,vt+24),iu::swap_state::Read(base,self+68),iu::swap_state::Read(base,self+72));
  __imp__sub_82636F20(ctx,base);
  if(relevant)iu::disc_swap::LogSwap("NOW_LOADING_PACKAGE_PREPARE_RETURN load_task=0x%08X request_sent=%u r3=0x%08X",self,iu::swap_state::Byte(base,self+76),ctx.r3.u32);
}