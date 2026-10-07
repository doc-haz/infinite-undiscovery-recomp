#pragma once
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "disc_swap.h"
namespace iu::swap_state {
inline rex::Runtime* runtime=nullptr;
inline std::atomic<uint32_t> task{0};
inline std::mutex mutex;
inline std::unordered_map<uint32_t,uint32_t> states;
inline uint32_t callback_target=0;
inline PPCFunc* callback_original=nullptr;
inline unsigned parked_count=0;
inline void Status(uint8_t* base,const char* stage);
inline uint32_t Read(uint8_t* base,uint32_t address){uint32_t v=0;iu::trace::peek32(base,address,v);return v;}
inline void Observe(uint8_t* base,uint32_t self,const char* source){
  if(!self || self!=task.load())return;
  const uint32_t value=Read(base,self+0x50);
  std::lock_guard lock(mutex);
  auto [it,inserted]=states.emplace(self,value);
  if(inserted) iu::disc_swap::LogSwap("DISC_SWAP_STATE_INITIAL task=0x%08X state=%u source=%s",self,value,source);
  else if(it->second!=value){iu::disc_swap::LogSwap("DISC_SWAP_STATE old=%u new=%u task=0x%08X source=%s",it->second,value,self,source);it->second=value;parked_count=0;}
}
inline void Callback(PPCContext& ctx,uint8_t* base){
  const uint32_t self=task.load(), object=ctx.r3.u32, arg=ctx.r4.u32;
  const bool relevant=self && object==self+0x38;
  if(relevant){Observe(base,self,"callback-entry");iu::disc_swap::LogSwap("DISC_SWAP_CALLBACK_ENTRY target=0x%08X object=0x%08X r4=0x%08X r4_word=0x%08X r5=0x%08X LR=0x%08X",callback_target,object,arg,Read(base,arg),ctx.r5.u32,uint32_t(ctx.lr));}
  if(relevant)iu::disc_swap::LogSwap("DISC_SWAP_824933E8_FIELDS_BEFORE requested_disc=%u request=0x%08X state=%u",Read(base,self+76),Read(base,self+68),Read(base,self+80));
  callback_original(ctx,base);
  if(relevant)iu::disc_swap::LogSwap("DISC_SWAP_824933E8_FIELDS_AFTER requested_disc=%u request=0x%08X state=%u",Read(base,self+76),Read(base,self+68),Read(base,self+80));
  if(relevant){Observe(base,self,"callback-return");iu::disc_swap::LogSwap("DISC_SWAP_CALLBACK_RETURN target=0x%08X task=0x%08X state=%u r3=0x%08X",callback_target,self,Read(base,self+0x50),ctx.r3.u32);}
}
inline void Track(uint8_t* base,uint32_t self){
  const uint32_t state=Read(base,self+0x50),disc=Read(base,self+0x4c);
  if(!task.load() && disc==2 && (state==8 || state==9)){
    task.store(self);
    const uint32_t object=self+0x38,vt=Read(base,object),target=Read(base,vt);
    iu::disc_swap::LogSwap("DISC_SWAP_CALLBACK_OBJECT task=0x%08X task_vtable=0x%08X object=0x%08X vtable=0x%08X slot0=0x%08X event_field=0x%08X",self,Read(base,self),object,vt,target,Read(base,self+0x44));
    iu::disc_swap::LogSwap("DISC_SWAP_CALLBACK_VTABLE object=0x%08X vtable=0x%08X",object,vt);
    iu::disc_swap::LogSwap("DISC_SWAP_CALLBACK_SLOT0 target=0x%08X",target);
    if(runtime && target && !callback_original){callback_target=target;callback_original=runtime->function_dispatcher()->GetFunction(target);if(callback_original)iu::disc_swap::LogSwap("DISC_SWAP_CALLBACK_HOOK target=0x%08X installed=%d",target,runtime->function_dispatcher()->SetFunction(target,Callback));}
  }
  Observe(base,self,"sub_824937D8-entry");
}
inline void Parked(uint8_t* base){const uint32_t self=task.load();if(!self)return;Observe(base,self,"heartbeat");std::lock_guard lock(mutex);if(parked_count++<15){Status(base,"parked");iu::disc_swap::LogSwap("DISC_SWAP_STATE state=%u task=0x%08X",Read(base,self+0x50),self);iu::disc_swap::LogSwap("DISC_SWAP_STATE_PARKED state=%u task=0x%08X vtable=0x%08X callback=0x%08X",Read(base,self+0x50),self,Read(base,self),self+0x38);}}
}
extern "C" void iu_disc_state_store(uint8_t* base,uint32_t address,uint32_t value,const char* writer){
  if(address!=iu::swap_state::task.load()+0x50 || !iu::swap_state::task.load())return;
  iu::swap_state::Observe(base,address-0x50,writer);
  iu::disc_swap::LogSwap("DISC_SWAP_STATE_WRITE task=0x%08X old=%u new=%u writer=%s",address-0x50,iu::swap_state::Read(base,address),value,writer);
}
namespace iu::swap_state {
inline uint8_t Byte(uint8_t* base,uint32_t address){uint8_t v=0;iu::trace::peek8(base,address,v);return v;}
inline std::atomic<uint32_t> visual_owner{0},load_owner{0};
inline void Status(uint8_t* base,const char* stage){
  const uint32_t root=0x82A58EA8,status=Read(base,root+0x48);
  const uint8_t mask=Byte(base,status+0x3c);
  const uint32_t loader=Read(base,root+0x68),inner=Read(base,loader+4);
  const uint32_t global=Read(base,0x82A58470);
  iu::disc_swap::LogSwap("NOW_LOADING_STATUS stage=%s root=0x%08X status_object=0x%08X status_byte=0x%02X bit0=%u bit1=%u bit2=%u task=0x%08X task_state=%u",stage,root,status,mask,mask&1,(mask>>1)&1,(mask>>2)&1,task.load(),Read(base,task.load()+0x50));
  iu::disc_swap::LogSwap("NOW_LOADING_BUSY stage=%s owner=0x%08X inner=0x%08X busy44=%u global=0x%08X gateE174=%u",stage,loader,inner,Read(base,inner+0x44),global,Byte(base,global+0xE174));
}
inline PPCFunc* load_callback_original=nullptr;
inline uint32_t load_callback_target=0,load_callback_object=0;
inline uint64_t last_condition_log=0;
inline uint32_t last_busy=~0u,last_gate=~0u,last_result=~0u;
inline unsigned condition_count=0;
inline void LoadCallback(PPCContext& ctx,uint8_t* base){
  const uint32_t self=task.load(),object=ctx.r3.u32;
  const bool relevant=object==load_callback_object;
  if(relevant)iu::disc_swap::LogSwap("NOW_LOADING_CALLBACK_ENTRY target=0x%08X object=0x%08X r4=0x%08X r5=0x%08X LR=0x%08X task_state=%u",load_callback_target,object,ctx.r4.u32,ctx.r5.u32,uint32_t(ctx.lr),Read(base,self+0x50));
  load_callback_original(ctx,base);
  if(relevant){Observe(base,self,"load-callback-return");iu::disc_swap::LogSwap("NOW_LOADING_CALLBACK_RETURN target=0x%08X task_state=%u r3=0x%08X",load_callback_target,Read(base,self+0x50),ctx.r3.u32);}
}
}
extern "C" void __imp__sub_82479700(PPCContext&,uint8_t*);
extern "C" void sub_82479700(PPCContext& ctx,uint8_t* base){
  const uint32_t root=ctx.r3.u32,mode=ctx.r4.u32,enable=ctx.r5.u32,lr=uint32_t(ctx.lr);
  const bool relevant=mode==2 && (lr==0x8249403C || lr==0x82494058);
  const uint32_t object=relevant?iu::swap_state::Read(base,root+72):0;
  if(relevant){iu::swap_state::visual_owner.store(object);const auto vt=iu::swap_state::Read(base,object);iu::disc_swap::LogSwap("NOW_LOADING_OWNER root=0x%08X object=0x%08X vtable=0x%08X slot28=0x%08X slot32=0x%08X",root,object,vt,iu::swap_state::Read(base,vt+28),iu::swap_state::Read(base,vt+32));iu::disc_swap::LogSwap("DISC_SWAP_82479700_ENTRY r3=0x%08X r4=%u r5=%u mask60=0x%02X task_state=%u LR=0x%08X",root,mode,enable,iu::swap_state::Byte(base,object+60),iu::swap_state::Read(base,iu::swap_state::task.load()+80),lr);}
  if(relevant)iu::swap_state::Status(base,"82479700-before");
  __imp__sub_82479700(ctx,base);
  if(relevant)iu::swap_state::Status(base,"82479700-after");
  if(relevant)iu::disc_swap::LogSwap("DISC_SWAP_82479700_RETURN object=0x%08X mask60=0x%02X task_state=%u r3=0x%08X",object,iu::swap_state::Byte(base,object+60),iu::swap_state::Read(base,iu::swap_state::task.load()+80),ctx.r3.u32);
}
extern "C" void __imp__sub_8248CC38(PPCContext&,uint8_t*);
extern "C" void sub_8248CC38(PPCContext& ctx,uint8_t* base){
  const bool relevant=uint32_t(ctx.lr)==0x824790F0 && iu::swap_state::task.load();
  const uint32_t owner=ctx.r3.u32,inner=relevant?iu::swap_state::Read(base,owner+4):0;
  const uint32_t busy=relevant?iu::swap_state::Read(base,inner+68):0;
  const uint32_t global=relevant?iu::swap_state::Read(base,0x82A58470):0;
  const uint32_t gate=relevant?iu::swap_state::Byte(base,global+57716):0;
  __imp__sub_8248CC38(ctx,base);
  if(relevant){
    const uint32_t result=ctx.r3.u32;const uint64_t now=GetTickCount64();
    iu::swap_state::load_owner.store(owner);
    if(result!=iu::swap_state::last_result || busy!=iu::swap_state::last_busy || gate!=iu::swap_state::last_gate || (iu::swap_state::condition_count<15 && now-iu::swap_state::last_condition_log>=1000)){
      iu::swap_state::last_condition_log=now;++iu::swap_state::condition_count;
      iu::disc_swap::LogSwap("NOW_LOADING_STATE task=0x%08X task_state=%u predicate=0x8248CC38 owner=0x%08X inner=0x%08X busy44=%u global=0x%08X gateE174=%u result=%u",iu::swap_state::task.load(),iu::swap_state::Read(base,iu::swap_state::task.load()+80),owner,inner,busy,global,gate,result);
      if(result)iu::disc_swap::LogSwap("NOW_LOADING_COMPLETE predicate=0x8248CC38 owner=0x%08X",owner);
      iu::swap_state::last_busy=busy;iu::swap_state::last_gate=gate;iu::swap_state::last_result=result;
    }
  }
}
extern "C" void __imp__sub_826CD6D8(PPCContext&,uint8_t*);
extern "C" void sub_826CD6D8(PPCContext& ctx,uint8_t* base){
  const uint32_t lr=uint32_t(ctx.lr);
  const bool relevant=lr==0x8247907C || lr==0x824790BC;
  if(relevant){
    const uint32_t callback=ctx.r9.u32,vt=iu::swap_state::Read(base,callback),target=iu::swap_state::Read(base,vt);
    iu::disc_swap::LogSwap("NOW_LOADING_ENTER function=0x826CD6D8 owner=0x%08X r4=%u r5=%u r6=0x%08X r7=0x%08X r8=0x%08X callback=0x%08X callback_vtable=0x%08X slot0=0x%08X r10=0x%08X LR=0x%08X",ctx.r3.u32,ctx.r4.u32,ctx.r5.u32,ctx.r6.u32,ctx.r7.u32,ctx.r8.u32,callback,vt,target,ctx.r10.u32,lr);
    if(callback && target && !iu::swap_state::load_callback_original && iu::swap_state::runtime){iu::swap_state::load_callback_object=callback;iu::swap_state::load_callback_target=target;iu::swap_state::load_callback_original=iu::swap_state::runtime->function_dispatcher()->GetFunction(target);if(iu::swap_state::load_callback_original)iu::disc_swap::LogSwap("NOW_LOADING_CALLBACK_HOOK target=0x%08X installed=%d",target,iu::swap_state::runtime->function_dispatcher()->SetFunction(target,iu::swap_state::LoadCallback));}
  }
  __imp__sub_826CD6D8(ctx,base);
  if(relevant)iu::disc_swap::LogSwap("NOW_LOADING_REQUEST_RETURN function=0x826CD6D8 r3=0x%08X task_state=%u",ctx.r3.u32,iu::swap_state::Read(base,iu::swap_state::task.load()+80));
}