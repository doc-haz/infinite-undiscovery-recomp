#pragma once
#include <rex/runtime.h>
#include <rex/graphics/graphics_system.h>
#include <rex/graphics/command_processor.h>
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <chrono>
#include <thread>
#include <rex/system/xthread.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xevent.h>
extern "C" void sub_827F10D8(PPCContext&, uint8_t*);
namespace vb {
inline std::mutex log_mutex;
template<class... T> inline void log(const char* fmt,T... args){
 std::lock_guard<std::mutex> guard(log_mutex);
 const char* path = std::getenv("IU_DIAG_TRACE_PATH");
 if (!path || path[0] == '\0') {
  return;
 }
 std::FILE* f = std::fopen(path, "a");
 if (!f) {
  return;
 }
 std::fprintf(f,"%llu tid=%lu ",GetTickCount64(),GetCurrentThreadId());
 std::fprintf(f,fmt,args...);std::fprintf(f,"\n");std::fclose(f);
}

inline unsigned read(unsigned a,uint8_t* base){return a?REX_LOAD_U32(a):0;}
struct Job {unsigned job,wrapper,handle,observer;};
inline Job job(uint8_t* base){unsigned j=read(0x82A58440,base),w=read(j?j+24:0,base);return {j,w,read(w,base),read(j?j+28:0,base)};}
inline unsigned display(uint8_t* base){return read(0x82A583C0,base);}
inline unsigned queue(uint8_t* base){unsigned d=display(base);return read(d?d+3516:0,base);}
inline std::atomic<unsigned> ordinal{0};
inline thread_local unsigned frame=0;
inline void identity(const char* tag,PPCContext& ctx,uint8_t* base){auto j=job(base);log("%s frame=%u job=%08X wrapper=%08X sem=%08X observer=%08X r3=%08X lr=%08X",tag,frame,j.job,j.wrapper,j.handle,j.observer,ctx.r3.u32,unsigned(ctx.lr));}
inline void enter(PPCContext& ctx,uint8_t* base){frame=ordinal++;unsigned d=ctx.r3.u32,ob=read(d+3488,base),render=read(0x82A58474,base);auto j=job(base);log("FRAME_BRANCH_ENTER frame=%u display=%08X flag3454=%u observer=%08X observer_wrapper=%08X job=%08X wrapper=%08X sem=%08X job_observer=%08X queue=%08X render_active=%u value3424=%u",frame,d,unsigned(REX_LOAD_U8(d+3454)),ob,read(ob?ob+4:0,base),j.job,j.wrapper,j.handle,j.observer,read(d+3516,base),read(render?render+652:0,base),read(d+3424,base));}
inline void branch(const char* tag,PPCContext& ctx,uint8_t* base){log("BRANCH frame=%u kind=%s display=%08X observer=%08X",frame,tag,ctx.r31.u32,read(ctx.r31.u32+3488,base));}
inline void ring(const char* tag,unsigned q,uint8_t* base){log("QUEUE %s queue=%08X expected=%08X write=%u read=%u state26=%u event=%08X",tag,q,queue(base),read(q+32,base),read(q+36,base),unsigned(REX_LOAD_U8(q+26)),read(q+16,base));}
inline void observer(PPCContext& ctx,uint8_t* base){auto j=job(base);unsigned ob=ctx.r3.u32,w=read(ob+4,base);log("OBSERVER_ENTER observer=%08X vtable=%08X wrapper=%08X sem=%08X matches_job=%u lr=%08X",ob,read(ob,base),w,read(w,base),unsigned(w==j.wrapper),unsigned(ctx.lr));}
inline void release(PPCContext& ctx,uint8_t* base){auto j=job(base);unsigned h=ctx.r3.u32,prev=ctx.r5.u32;bool match=j.wrapper&&h==j.handle;if(match)log("JOB_RELEASE_ENTER frame=%u job=%08X wrapper=%08X sem=%08X count=%u lr=%08X",frame,j.job,j.wrapper,h,ctx.r4.u32,unsigned(ctx.lr));__imp__NtReleaseSemaphore(ctx,base);if(match)log("JOB_RELEASE_RETURN sem=%08X status=%08X previous=%u available=%u",h,ctx.r3.u32,prev&&ctx.r3.u32==0?read(prev,base):999,unsigned(prev!=0));}
inline void setevent(PPCContext& ctx,uint8_t* base){unsigned q=queue(base),h=ctx.r3.u32;bool match=q&&h==read(q+16,base);if(match)log("WORKER_EVENT_SET_ENTER queue=%08X event=%08X lr=%08X",q,h,unsigned(ctx.lr));__imp__NtSetEvent(ctx,base);if(match)log("WORKER_EVENT_SET_RETURN event=%08X status=%08X",h,ctx.r3.u32);}

struct CPProbe:rex::graphics::CommandProcessor {
 static unsigned wp(rex::graphics::CommandProcessor* p){auto m=&CPProbe::write_ptr_index_;return (p->*m).load();}
 static bool running(rex::graphics::CommandProcessor* p){auto m=&CPProbe::worker_running_;return (p->*m).load();}
};
inline rex::graphics::CommandProcessor* host_runtime(rex::Runtime* rt,const char* tag){auto g=rt?rt->graphics_system():nullptr;auto cp=g?static_cast<rex::graphics::GraphicsSystem*>(g)->command_processor():nullptr;auto h=rex::runtime::MMIOHandler::global_handler();auto range=h?h->LookupRange(0x7FC80714):nullptr;log("HOST %s plugin_module=%p graphics=%p cp=%p cp_running=%u cp_wptr=%u mmio_range=%p callback=%p",tag,(void*)GetModuleHandleW(L"rexgpu-xenos.dll"),(void*)g,(void*)cp,cp?unsigned(CPProbe::running(cp)):0,cp?CPProbe::wp(cp):0,(void*)range,range?(void*)range->write:nullptr);return cp;}
inline rex::graphics::CommandProcessor* host(const char* tag){auto k=rex::system::kernel_state();return host_runtime(k?k->emulator():nullptr,tag);}
inline void registration(PPCContext& ctx){log("REGISTER callback=%08X userdata=%08X",ctx.r3.u32,ctx.r4.u32);host("REGISTER");}
inline void wait(PPCContext& ctx,uint8_t* base){auto j=job(base);unsigned h=ctx.r3.u32,lr=unsigned(ctx.lr),sp=ctx.r1.u32;bool match=j.wrapper&&h==j.handle;if(match)log("JOB_WAIT_ENTER frame=%u job=%08X wrapper=%08X sem=%08X lr=%08X sp=%08X",frame,j.job,j.wrapper,h,lr,sp);__imp__NtWaitForSingleObjectEx(ctx,base);if(match)log("JOB_WAIT_RETURN frame=%u job=%08X wrapper=%08X sem=%08X status=%08X",frame,j.job,j.wrapper,h,ctx.r3.u32);}
inline void mmstore(unsigned a,unsigned v,PPCContext& ctx,uint8_t* base){if(a!=0x7FC80714){REX_MM_STORE_U32(a,v);return;}auto cp=host("BEFORE_WPTR");log("WPTR_ENTER value=%u lr=%08X",v,unsigned(ctx.lr));bool accepted=rex::runtime::MMIOHandler::global_handler()->CheckStore(a,v);log("WPTR_RETURN value=%u accepted=%u cp=%p cp_wptr=%u cp_running=%u",v,unsigned(accepted),(void*)cp,cp?CPProbe::wp(cp):0,cp?unsigned(CPProbe::running(cp)):0);}
inline thread_local unsigned ring_device=0,required=0;
inline thread_local uint64_t last_sample=0;
inline void ring_sample(const char* tag,PPCContext& ctx,uint8_t* base,bool force){auto n=GetTickCount64();if(!force&&n-last_sample<500)return;last_sample=n;unsigned p=read(ring_device+10896,base);log("RING_%s ordinal=%u device=%08X required=%u block=%08X rptr=%u block60=%u lr=%08X sp=%08X",tag,ordinal.load(),ring_device,required,p,read(p,base),read(p?p+60:0,base),unsigned(ctx.lr),ctx.r1.u32);}
}
