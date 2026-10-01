#pragma once
DECLARE_REX_FUNC(NtReleaseSemaphore);
DECLARE_REX_FUNC(NtSetEvent);
DECLARE_REX_FUNC(NtWaitForSingleObjectEx);
#include "vblank_diag.h"
namespace obs2360 {
inline std::atomic<unsigned long long> completed{0};
inline std::atomic<bool> seen{false};
inline void first(PPCContext& ctx) {
 if (!seen.exchange(true)) vb::log("FIRST_821F2360 frame_by_wait_returns=%llu r3=%08X r4=%08X lr=%08X", completed.load(),ctx.r3.u32,ctx.r4.u32,unsigned(ctx.lr));
}
inline void wait(PPCContext& ctx,uint8_t* base) {
 auto j=vb::job(base); bool match=j.wrapper&&ctx.r3.u32==j.handle;
 __imp__NtWaitForSingleObjectEx(ctx,base);
 if(match&&ctx.r3.u32==0) {
  auto f=completed.fetch_add(1);
  if(f==0 || f==1 || f==100 || f==200 || f==300 || f==310 || (f>310 && f%100==0)) {
   vb::log("JOB_WAIT_RETURN frame=%llu sem=%08X status=%08X completed_ptr=%p seen_ptr=%p",f,j.handle,ctx.r3.u32,(void*)&completed,(void*)&seen);
  }
 }
}
}
