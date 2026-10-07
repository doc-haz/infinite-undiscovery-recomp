#pragma once

// Guest trace points built on the common framework (iu_trace.h).
// Included once from src/main.cpp AFTER generated/default/infinite_undiscovery_init.h.
//
// Each override uses ReXGlue's weak-alias mechanism (see vesplume_diag.h): the original
// guest body stays callable as __imp__sub_<addr> and is invoked exactly once. When the
// category is disabled the wrapper is a straight pass-through.

#include "iu_trace.h"

struct PPCContext;

// ---- SAVEPOINT -------------------------------------------------------------
// sub_8263CBB8: virtual (vtable slot 33 / +0x84, inferred) of the physical Save Point
// object (vtable 0x8205EB5C, ctor sub_8263CD08). r3 = this, r4 = u16 argument stored
// at this+2968. Original also stores a handle at this+2972.
extern "C" void __imp__sub_8263CBB8(PPCContext& ctx, uint8_t* base);

extern "C" void sub_8263CBB8(PPCContext& ctx, uint8_t* base) {
	using namespace iu::trace;
	if (!enabled(kSavePoint)) {
		__imp__sub_8263CBB8(ctx, base);
		return;
	}

	const uint32_t entry_this = ctx.r3.u32;
	const uint32_t entry_r4 = ctx.r4.u32;
	const uint32_t entry_lr = static_cast<uint32_t>(ctx.lr);

	emit(kSavePoint, "sub_8263CBB8 ENTER this(CSavePointObject?)=0x%08X host=%p r4=0x%08X LR=0x%08X",
	     entry_this, (void*)host_ptr(base, entry_this), entry_r4, entry_lr);

	REX_FUNC_PROLOGUE();
	PPCRegister temp{};
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x826ee1e4
	ctx.lr = 0x8263CBC0;
	__savegprlr_27(ctx, base);
	// stwu r1,-144(r1)
	ea = -144 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// lis r29,-32087
	ctx.r29.s64 = -2102853632;
	// mr r27,r4
	ctx.r27.u64 = ctx.r4.u64;
	// lis r11,-32092
	ctx.r11.s64 = -2103181312;
	// mr r30,r3
	ctx.r30.u64 = ctx.r3.u64;
	// li r28,1
	ctx.r28.s64 = 1;
	// lwz r4,29380(r29)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r29.u32 + 29380);
	// addi r31,r11,-32520
	ctx.r31.s64 = ctx.r11.s64 + -32520;
	// cmplwi cr6,r4,0
	ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
	// beq cr6,0x8263cc0c
	if (ctx.cr6.eq) goto loc_8263CC0C;
	// li r5,24
	ctx.r5.s64 = 24;
	// lwz r3,24(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 24);
	// bl 0x82147140
	ctx.lr = 0x8263CBF4;
	sub_82147140(ctx, base);
	// cmplwi r3,0
	ctx.cr0.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// beq 0x8263cc0c
	if (ctx.cr0.eq) goto loc_8263CC0C;
	// li r11,0
	ctx.r11.s64 = 0;
	// stb r28,3041(r3)
	REX_STORE_U8(ctx.r3.u32 + 3041, ctx.r28.u8);
	// stw r11,3028(r3)
	REX_STORE_U32(ctx.r3.u32 + 3028, ctx.r11.u32);
	// stw r11,29380(r29)
	REX_STORE_U32(ctx.r29.u32 + 29380, ctx.r11.u32);
loc_8263CC0C:
	// lis r11,-32255
	ctx.r11.s64 = -2113863680;
	// li r7,0
	ctx.r7.s64 = 0;
	// li r6,0
	ctx.r6.s64 = 0;
	// addi r3,r1,80
	ctx.r3.s64 = ctx.r1.s64 + 80;
	// lfs f2,-21020(r11)
	ctx.fpscr.disableFlushMode();
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -21020);
	ctx.f2.f64 = double(temp.f32);
	// lis r11,-32252
	ctx.r11.s64 = -2113667072;
	// lfs f1,-29924(r11)
	temp.u32 = REX_LOAD_U32(ctx.r11.u32 + -29924);
	ctx.f1.f64 = double(temp.f32);

	// ---- LOGGING IMMEDIATELY BEFORE sub_82475670 ----
	const uint32_t csave_this = ctx.r30.u32;
	const uint32_t holder_guest = ctx.r3.u32;
	const uint32_t sp = ctx.r1.u32;

	uint32_t h_before[8]{};
	for (int i = 0; i < 8; ++i) {
		peek32(base, holder_guest + i * 4, h_before[i]);
	}

	emit(kSavePoint,
	     "sub_8263CBB8 BEFORE sub_82475670 this=0x%08X (host=%p) SP=0x%08X holder=0x%08X (host=%p) "
	     "r31=0x%08X LR=0x%08X CTR=0x%08X",
	     csave_this, (void*)host_ptr(base, csave_this), sp, holder_guest, (void*)host_ptr(base, holder_guest),
	     ctx.r31.u32, 0x8263CC2C, ctx.ctr.u32);

	emit(kSavePoint,
	     "sub_8263CBB8 BEFORE ARGS r3=0x%08X r4=0x%08X r5=0x%08X r6=0x%08X r7=0x%08X r8=0x%08X r9=0x%08X r10=0x%08X f1=%.4f f2=%.4f",
	     ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32, ctx.r8.u32, ctx.r9.u32, ctx.r10.u32,
	     (float)ctx.f1.f64, (float)ctx.f2.f64);

	emit(kSavePoint,
	     "sub_8263CBB8 BEFORE this_fields: vtable=%s +2968=%s +2972=%s +2976=%s",
	     field32_str(base, csave_this).c_str(),
	     field32_str(base, csave_this + 2968).c_str(),
	     field32_str(base, csave_this + 2972).c_str(),
	     field32_str(base, csave_this + 2976).c_str());

	emit(kSavePoint,
	     "sub_8263CBB8 BEFORE holder_raw: [0]=0x%08X [4]=0x%08X [8]=0x%08X [12]=0x%08X [16]=0x%08X [20]=0x%08X [24]=0x%08X [28]=0x%08X",
	     h_before[0], h_before[1], h_before[2], h_before[3], h_before[4], h_before[5], h_before[6], h_before[7]);

	// bl 0x82475670
	ctx.lr = 0x8263CC2C;
	sub_82475670(ctx, base);

	// ---- LOGGING IMMEDIATELY AFTER sub_82475670 ----
	const uint32_t ret_r3 = ctx.r3.u32;
	uint32_t h_after[16]{};
	for (int i = 0; i < 16; ++i) {
		peek32(base, ret_r3 + i * 4, h_after[i]);
	}

	std::string written_entries;
	char wb[64];
	for (int i = 0; i < 16; ++i) {
		if (i >= 8 || h_after[i] != h_before[i]) {
			std::snprintf(wb, sizeof(wb), " [+%d]=0x%08X", i * 4, h_after[i]);
			written_entries += wb;
		}
	}

	emit(kSavePoint,
	     "sub_8263CBB8 AFTER sub_82475670 ret_r3=0x%08X holder[0]=0x%08X holder[4]=0x%08X LR=0x%08X",
	     ret_r3, h_after[0], h_after[1], static_cast<uint32_t>(ctx.lr));

	emit(kSavePoint,
	     "sub_8263CBB8 AFTER holder_written:%s",
	     written_entries.empty() ? " (none changed)" : written_entries.c_str());

	emit(kSavePoint,
	     "sub_8263CBB8 AFTER this_fields: +2968=%s +2972=%s",
	     field32_str(base, csave_this + 2968).c_str(),
	     field32_str(base, csave_this + 2972).c_str());

	// lwz r4,4(r3)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r3.u32 + 4);
	// li r5,7
	ctx.r5.s64 = 7;
	// stw r4,2972(r30)
	REX_STORE_U32(ctx.r30.u32 + 2972, ctx.r4.u32);

	emit(kSavePoint,
	     "sub_8263CBB8 AFTER stw_2972 this+2972=%s (stored r4=0x%08X from holder[4])",
	     field32_str(base, csave_this + 2972).c_str(), ctx.r4.u32);

	// lwz r3,24(r31)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 24);
	// bl 0x82147140
	ctx.lr = 0x8263CC40;
	sub_82147140(ctx, base);
	// mr r31,r3
	ctx.r31.u64 = ctx.r3.u64;
	// li r4,1
	ctx.r4.s64 = 1;
	// stw r28,12592(r31)
	REX_STORE_U32(ctx.r31.u32 + 12592, ctx.r28.u32);
	// bl 0x8219caf0
	ctx.lr = 0x8263CC50;
	sub_8219CAF0(ctx, base);
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x826d7e08
	ctx.lr = 0x8263CC58;
	sub_826D7E08(ctx, base);
	// lis r11,-32090
	ctx.r11.s64 = -2103050240;
	// li r4,1
	ctx.r4.s64 = 1;
	// addi r31,r11,-29016
	ctx.r31.s64 = ctx.r11.s64 + -29016;
	// addi r3,r31,404
	ctx.r3.s64 = ctx.r31.s64 + 404;
	// bl 0x8247cc38
	ctx.lr = 0x8263CC6C;
	sub_8247CC38(ctx, base);
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// bl 0x825b8288
	ctx.lr = 0x8263CC74;
	sub_825B8288(ctx, base);
	// li r4,0
	ctx.r4.s64 = 0;
	// bl 0x82499198
	ctx.lr = 0x8263CC7C;
	sub_82499198(ctx, base);
	// stw r27,2968(r30)
	REX_STORE_U32(ctx.r30.u32 + 2968, ctx.r27.u32);

	emit(kSavePoint,
	     "sub_8263CBB8 AFTER stw_2968 this+2968=%s (stored r27=0x%08X)",
	     field32_str(base, csave_this + 2968).c_str(), ctx.r27.u32);

	emit(kSavePoint, "sub_8263CBB8 EXIT this=0x%08X ret_r3=0x%08X LR=0x%08X", csave_this, ctx.r3.u32, entry_lr);

	// addi r1,r1,144
	ctx.r1.s64 = ctx.r1.s64 + 144;
	// b 0x826ee234
	__restgprlr_27(ctx, base);
	return;
}

// ---- SAVEPOINT DISPATCH ----------------------------------------------------
// sub_825B93F8: immediate dispatch context reaching CSavePointObject slot +0x148 / sub_8263CBB8.
extern "C" void __imp__sub_825B93F8(PPCContext& ctx, uint8_t* base);

extern "C" void sub_825B93F8(PPCContext& ctx, uint8_t* base) {
	using namespace iu::trace;
	if (!enabled(kSavePoint)) {
		__imp__sub_825B93F8(ctx, base);
		return;
	}
	bool is_slot_148 = false;
	REX_FUNC_PROLOGUE();
	uint32_t ea{};
	// mflr r12
	ctx.r12.u64 = ctx.lr;
	// bl 0x826ee1c8
	ctx.lr = 0x825B9400;
	__savegprlr_20(ctx, base);
	// stwu r1,-208(r1)
	ea = -208 + ctx.r1.u32;
	REX_STORE_U32(ea, ctx.r1.u32);
	ctx.r1.u32 = ea;
	// mr r23,r3
	ctx.r23.u64 = ctx.r3.u64;
	// mr r26,r4
	ctx.r26.u64 = ctx.r4.u64;
	// mr r20,r5
	ctx.r20.u64 = ctx.r5.u64;
	// li r25,0
	ctx.r25.s64 = 0;
	// lwz r11,4(r23)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r23.u32 + 4);
	// cmplwi r11,0
	ctx.cr0.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq 0x825b9428
	if (ctx.cr0.eq) goto loc_825B9428;
	// lwz r4,16(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 16);
	// b 0x825b942c
	goto loc_825B942C;
loc_825B9428:
	// mr r4,r25
	ctx.r4.u64 = ctx.r25.u64;
loc_825B942C:
	// lis r11,-32092
	ctx.r11.s64 = -2103181312;
	// li r5,19
	ctx.r5.s64 = 19;
	// addi r27,r11,-32520
	ctx.r27.s64 = ctx.r11.s64 + -32520;
	// lwz r3,24(r27)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r27.u32 + 24);
	// bl 0x82147140
	ctx.lr = 0x825B9440;
	sub_82147140(ctx, base);
	// mr r21,r3
	ctx.r21.u64 = ctx.r3.u64;
	// mr r24,r25
	ctx.r24.u64 = ctx.r25.u64;
	// li r22,1
	ctx.r22.s64 = 1;
	// cmplwi cr6,r26,0
	ctx.cr6.compare<uint32_t>(ctx.r26.u32, 0, ctx.xer);
	// beq cr6,0x825b9498
	if (ctx.cr6.eq) goto loc_825B9498;
	// li r5,17
	ctx.r5.s64 = 17;
	// lwz r3,24(r27)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r27.u32 + 24);
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// bl 0x82147140
	ctx.lr = 0x825B9464;
	sub_82147140(ctx, base);
	// mr. r24,r3
	ctx.r24.u64 = ctx.r3.u64;
	ctx.cr0.compare<int32_t>(ctx.r24.s32, 0, ctx.xer);
	// beq 0x825b9498
	if (ctx.cr0.eq) goto loc_825B9498;
	// ld r11,24(r24)
	ctx.r11.u64 = REX_LOAD_U64(ctx.r24.u32 + 24);
	// rlwinm r11,r11,0,13,13
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 0) & 0x40000;
	// cmpldi cr6,r11,0
	ctx.cr6.compare<uint64_t>(ctx.r11.u64, 0, ctx.xer);
	// mr r11,r22
	ctx.r11.u64 = ctx.r22.u64;
	// bne cr6,0x825b9484
	if (!ctx.cr6.eq) goto loc_825B9484;
	// mr r11,r25
	ctx.r11.u64 = ctx.r25.u64;
loc_825B9484:
	// clrlwi. r11,r11,24
	ctx.r11.u64 = ctx.r11.u32 & 0xFF;
	ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// beq 0x825b9498
	if (ctx.cr0.eq) goto loc_825B9498;
	// lbz r11,3225(r24)
	ctx.r11.u64 = REX_LOAD_U8(ctx.r24.u32 + 3225);
	// cmplwi r11,0
	ctx.cr0.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne 0x825b9788
	if (!ctx.cr0.eq) goto loc_825B9788;
loc_825B9498:
	// lis r11,-32090
	ctx.r11.s64 = -2103050240;
	// li r4,3
	ctx.r4.s64 = 3;
	// addi r28,r11,-29016
	ctx.r28.s64 = ctx.r11.s64 + -29016;
	// mr r30,r25
	ctx.r30.u64 = ctx.r25.u64;
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// bl 0x821475d8
	ctx.lr = 0x825B94B0;
	sub_821475D8(ctx, base);
	// clrlwi r11,r3,24
	ctx.r11.u64 = ctx.r3.u32 & 0xFF;
	// cmplwi cr6,r11,1
	ctx.cr6.compare<uint32_t>(ctx.r11.u32, 1, ctx.xer);
	// bne cr6,0x825b94c0
	if (!ctx.cr6.eq) goto loc_825B94C0;
	// mr r30,r22
	ctx.r30.u64 = ctx.r22.u64;
loc_825B94C0:
	// lwz r11,3260(r21)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r21.u32 + 3260);
	// lwz r10,48(r11)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 48);
	// cmpwi cr6,r10,0
	ctx.cr6.compare<int32_t>(ctx.r10.s32, 0, ctx.xer);
	// ble cr6,0x825b94dc
	if (!ctx.cr6.gt) goto loc_825B94DC;
	// lwz r11,3128(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 3128);
	// rlwinm. r11,r11,23,31,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 23) & 0x1;
	ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// beq 0x825b94e0
	if (ctx.cr0.eq) goto loc_825B94E0;
loc_825B94DC:
	// mr r30,r22
	ctx.r30.u64 = ctx.r22.u64;
loc_825B94E0:
	// lwz r11,4(r23)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r23.u32 + 4);
	// cmplwi r11,0
	ctx.cr0.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq 0x825b94f4
	if (ctx.cr0.eq) goto loc_825B94F4;
	// lwz r31,16(r11)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r11.u32 + 16);
	// b 0x825b94f8
	goto loc_825B94F8;
loc_825B94F4:
	// mr r31,r25
	ctx.r31.u64 = ctx.r25.u64;
loc_825B94F8:
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// bl 0x82838868
	ctx.lr = 0x825B9500;
	sub_82838868(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// beq cr6,0x825b954c
	if (ctx.cr6.eq) goto loc_825B954C;
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// lwz r31,24(r27)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r27.u32 + 24);
	// bl 0x82838868
	ctx.lr = 0x825B9514;
	sub_82838868(ctx, base);
	// mr r4,r3
	ctx.r4.u64 = ctx.r3.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// li r5,19
	ctx.r5.s64 = 19;
	// bl 0x82147140
	ctx.lr = 0x825B9524;
	sub_82147140(ctx, base);
	// cmplwi r3,0
	ctx.cr0.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// beq 0x825b9548
	if (ctx.cr0.eq) goto loc_825B9548;
	// lwz r11,3260(r3)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 3260);
	// lwz r10,48(r11)
	ctx.r10.u64 = REX_LOAD_U32(ctx.r11.u32 + 48);
	// cmpwi cr6,r10,0
	ctx.cr6.compare<int32_t>(ctx.r10.s32, 0, ctx.xer);
	// ble cr6,0x825b9548
	if (!ctx.cr6.gt) goto loc_825B9548;
	// lwz r11,3128(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 3128);
	// rlwinm. r11,r11,23,31,31
	ctx.r11.u64 = __builtin_rotateleft64(ctx.r11.u32 | (ctx.r11.u64 << 32), 23) & 0x1;
	ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// beq 0x825b954c
	if (ctx.cr0.eq) goto loc_825B954C;
loc_825B9548:
	// mr r30,r22
	ctx.r30.u64 = ctx.r22.u64;
loc_825B954C:
	// clrlwi. r11,r30,24
	ctx.r11.u64 = ctx.r30.u32 & 0xFF;
	ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// mr r4,r26
	ctx.r4.u64 = ctx.r26.u64;
	// beq 0x825b95b8
	if (ctx.cr0.eq) goto loc_825B95B8;
	// li r11,3025
	ctx.r11.s64 = 3025;
	// lwz r3,44(r27)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r27.u32 + 44);
	// li r6,0
	ctx.r6.s64 = 0;
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// stw r11,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r11.u32);
	// bl 0x8217de80
	ctx.lr = 0x825B9570;
	sub_8217DE80(ctx, base);
	// li r10,1302
	ctx.r10.s64 = 1302;
	// lwz r11,4(r23)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r23.u32 + 4);
	// stw r25,96(r1)
	REX_STORE_U32(ctx.r1.u32 + 96, ctx.r25.u32);
	// stw r26,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r26.u32);
	// cmplwi r11,0
	ctx.cr0.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// stw r22,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r22.u32);
	// stw r10,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r10.u32);
	// li r10,7
	ctx.r10.s64 = 7;
	// stw r10,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r10.u32);
	// beq 0x825b95a0
	if (ctx.cr0.eq) goto loc_825B95A0;
	// lwz r4,16(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 16);
	// b 0x825b95a4
	goto loc_825B95A4;
loc_825B95A0:
	// mr r4,r25
	ctx.r4.u64 = ctx.r25.u64;
loc_825B95A4:
	// li r6,0
	ctx.r6.s64 = 0;
	// lwz r3,44(r27)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r27.u32 + 44);
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// bl 0x8217de80
	ctx.lr = 0x825B95B4;
	sub_8217DE80(ctx, base);
	// b 0x825b9788
	goto loc_825B9788;
loc_825B95B8:
	// li r5,17
	ctx.r5.s64 = 17;
	// lwz r3,24(r27)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r27.u32 + 24);
	// bl 0x82147140
	ctx.lr = 0x825B95C4;
	sub_82147140(ctx, base);
	// mr r29,r25
	ctx.r29.u64 = ctx.r25.u64;
	// cmplwi r3,0
	ctx.cr0.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
	// beq 0x825b95fc
	if (ctx.cr0.eq) goto loc_825B95FC;
	// li r12,1
	ctx.r12.s64 = 1;
	// ld r11,24(r3)
	ctx.r11.u64 = REX_LOAD_U64(ctx.r3.u32 + 24);
	// rldicr r12,r12,36,63
	ctx.r12.u64 = __builtin_rotateleft64(ctx.r12.u64, 36) & 0xFFFFFFFFFFFFFFFF;
	// and r11,r11,r12
	ctx.r11.u64 = ctx.r11.u64 & ctx.r12.u64;
	// cmpldi cr6,r11,0
	ctx.cr6.compare<uint64_t>(ctx.r11.u64, 0, ctx.xer);
	// mr r11,r22
	ctx.r11.u64 = ctx.r22.u64;
	// bne cr6,0x825b95f0
	if (!ctx.cr6.eq) goto loc_825B95F0;
	// mr r11,r25
	ctx.r11.u64 = ctx.r25.u64;
loc_825B95F0:
	// clrlwi. r11,r11,24
	ctx.r11.u64 = ctx.r11.u32 & 0xFF;
	ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// beq 0x825b95fc
	if (ctx.cr0.eq) goto loc_825B95FC;
	// mr r29,r22
	ctx.r29.u64 = ctx.r22.u64;
loc_825B95FC:
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// lwz r31,24(r27)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r27.u32 + 24);
	// bl 0x824796b0
	ctx.lr = 0x825B9608;
	sub_824796B0(ctx, base);
	// mr r4,r3
	ctx.r4.u64 = ctx.r3.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// li r5,19
	ctx.r5.s64 = 19;
	// bl 0x82147140
	ctx.lr = 0x825B9618;
	sub_82147140(ctx, base);
	// mr r30,r3
	ctx.r30.u64 = ctx.r3.u64;
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// lwz r31,16(r21)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r21.u32 + 16);
	// bl 0x82838868
	ctx.lr = 0x825B9628;
	sub_82838868(ctx, base);
	// cmplw cr6,r31,r3
	ctx.cr6.compare<uint32_t>(ctx.r31.u32, ctx.r3.u32, ctx.xer);
	// bne cr6,0x825b9684
	if (!ctx.cr6.eq) goto loc_825B9684;
	// cmplwi cr6,r30,0
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, 0, ctx.xer);
	// beq cr6,0x825b9684
	if (ctx.cr6.eq) goto loc_825B9684;
	// lwz r3,3236(r30)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r30.u32 + 3236);
	// bl 0x824e3fb0
	ctx.lr = 0x825B9640;
	sub_824E3FB0(ctx, base);
	// clrlwi. r11,r3,24
	ctx.r11.u64 = ctx.r3.u32 & 0xFF;
	ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// beq 0x825b9684
	if (ctx.cr0.eq) goto loc_825B9684;
	// lwz r11,3236(r21)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r21.u32 + 3236);
	// lwz r11,584(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 584);
	// cmpwi cr6,r11,32
	ctx.cr6.compare<int32_t>(ctx.r11.s32, 32, ctx.xer);
	// beq cr6,0x825b9684
	if (ctx.cr6.eq) goto loc_825B9684;
	// clrlwi. r11,r29,24
	ctx.r11.u64 = ctx.r29.u32 & 0xFF;
	ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
	// bne 0x825b9684
	if (!ctx.cr0.eq) goto loc_825B9684;
	// cmplwi cr6,r24,0
	ctx.cr6.compare<uint32_t>(ctx.r24.u32, 0, ctx.xer);
	// beq cr6,0x825b9684
	if (ctx.cr6.eq) goto loc_825B9684;
	// lbz r11,9(r23)
	ctx.r11.u64 = REX_LOAD_U8(ctx.r23.u32 + 9);
	// cmplwi r11,0
	ctx.cr0.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// bne 0x825b9684
	if (!ctx.cr0.eq) goto loc_825B9684;
	// lwz r4,16(r24)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r24.u32 + 16);
	// lwz r3,3236(r30)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r30.u32 + 3236);
	// bl 0x824e3838
	ctx.lr = 0x825B9680;
	sub_824E3838(ctx, base);
	// b 0x825b9788
	goto loc_825B9788;
loc_825B9684:
	// clrlwi. r30,r20,24
	ctx.r30.u64 = ctx.r20.u32 & 0xFF;
	ctx.cr0.compare<int32_t>(ctx.r30.s32, 0, ctx.xer);
	// bne 0x825b96e0
	if (!ctx.cr0.eq) goto loc_825B96E0;
	// lwz r11,4(r23)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r23.u32 + 4);
	// cmplwi r11,0
	ctx.cr0.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// beq 0x825b96a0
	if (ctx.cr0.eq) goto loc_825B96A0;
	// lwz r31,16(r11)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r11.u32 + 16);
	// b 0x825b96a4
	goto loc_825B96A4;
loc_825B96A0:
	// mr r31,r25
	ctx.r31.u64 = ctx.r25.u64;
loc_825B96A4:
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// bl 0x824796b0
	ctx.lr = 0x825B96AC;
	sub_824796B0(ctx, base);
	// cmplw cr6,r3,r31
	ctx.cr6.compare<uint32_t>(ctx.r3.u32, ctx.r31.u32, ctx.xer);
	// bne cr6,0x825b96e0
	if (!ctx.cr6.eq) goto loc_825B96E0;
	// li r11,1290
	ctx.r11.s64 = 1290;
	// lwz r31,44(r27)
	ctx.r31.u64 = REX_LOAD_U32(ctx.r27.u32 + 44);
	// mr r3,r28
	ctx.r3.u64 = ctx.r28.u64;
	// addi r29,r1,80
	ctx.r29.s64 = ctx.r1.s64 + 80;
	// stw r11,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r11.u32);
	// bl 0x82838868
	ctx.lr = 0x825B96CC;
	sub_82838868(ctx, base);
	// mr r4,r3
	ctx.r4.u64 = ctx.r3.u64;
	// mr r3,r31
	ctx.r3.u64 = ctx.r31.u64;
	// mr r5,r29
	ctx.r5.u64 = ctx.r29.u64;
	// li r6,0
	ctx.r6.s64 = 0;
	// bl 0x8217de80
	ctx.lr = 0x825B96E0;
	sub_8217DE80(ctx, base);
loc_825B96E0:
	// cmplwi cr6,r24,0
	ctx.cr6.compare<uint32_t>(ctx.r24.u32, 0, ctx.xer);
	// beq cr6,0x825b9788
	if (ctx.cr6.eq) goto loc_825B9788;
	// li r10,1302
	ctx.r10.s64 = 1302;
	// lwz r11,4(r23)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r23.u32 + 4);
	// cmplwi cr6,r30,0
	ctx.cr6.compare<uint32_t>(ctx.r30.u32, 0, ctx.xer);
	// stw r25,96(r1)
	REX_STORE_U32(ctx.r1.u32 + 96, ctx.r25.u32);
	// stw r26,84(r1)
	REX_STORE_U32(ctx.r1.u32 + 84, ctx.r26.u32);
	// cmplwi r11,0
	ctx.cr0.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
	// stw r25,92(r1)
	REX_STORE_U32(ctx.r1.u32 + 92, ctx.r25.u32);
	// stw r10,80(r1)
	REX_STORE_U32(ctx.r1.u32 + 80, ctx.r10.u32);
	// beq cr6,0x825b9744
	if (ctx.cr6.eq) goto loc_825B9744;
	// li r10,3
	ctx.r10.s64 = 3;
	// stb r22,9(r23)
	REX_STORE_U8(ctx.r23.u32 + 9, ctx.r22.u8);
	// stw r10,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r10.u32);
	// beq 0x825b9724
	if (ctx.cr0.eq) goto loc_825B9724;
	// lwz r4,16(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 16);
	// b 0x825b9728
	goto loc_825B9728;
loc_825B9724:
	// mr r4,r25
	ctx.r4.u64 = ctx.r25.u64;
loc_825B9728:
	// li r6,0
	ctx.r6.s64 = 0;
	// lwz r3,44(r27)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r27.u32 + 44);
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// bl 0x8217de80
	ctx.lr = 0x825B9738;
	sub_8217DE80(ctx, base);
	// lwz r11,0(r24)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r24.u32 + 0);
	// lwz r11,324(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 324);
	// b 0x825b9778
	goto loc_825B9778;
loc_825B9744:
	// li r10,5
	ctx.r10.s64 = 5;
	// stb r25,9(r23)
	REX_STORE_U8(ctx.r23.u32 + 9, ctx.r25.u8);
	// stw r10,88(r1)
	REX_STORE_U32(ctx.r1.u32 + 88, ctx.r10.u32);
	// beq 0x825b975c
	if (ctx.cr0.eq) goto loc_825B975C;
	// lwz r4,16(r11)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 16);
	// b 0x825b9760
	goto loc_825B9760;
loc_825B975C:
	// mr r4,r25
	ctx.r4.u64 = ctx.r25.u64;
loc_825B9760:
	// li r6,0
	ctx.r6.s64 = 0;
	// lwz r3,44(r27)
	ctx.r3.u64 = REX_LOAD_U32(ctx.r27.u32 + 44);
	// addi r5,r1,80
	ctx.r5.s64 = ctx.r1.s64 + 80;
	// bl 0x8217de80
	ctx.lr = 0x825B9770;
	sub_8217DE80(ctx, base);
	// lwz r11,0(r24)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r24.u32 + 0);
	// lwz r11,328(r11)
	ctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 328);
	is_slot_148 = true;
loc_825B9778:
	// lwz r4,16(r21)
	ctx.r4.u64 = REX_LOAD_U32(ctx.r21.u32 + 16);
	// mr r3,r24
	ctx.r3.u64 = ctx.r24.u64;
	// mtctr r11
	ctx.ctr.u64 = ctx.r11.u64;
	// bctrl 
	ctx.lr = 0x825B9788;
	if (is_slot_148) {
		const uint32_t r23_this = ctx.r23.u32;
		uint32_t this_p4 = 0;
		peek32(base, r23_this + 4, this_p4);
		uint32_t lookup_id = 0;
		if (this_p4 != 0) {
			peek32(base, this_p4 + 0x10, lookup_id);
		}
		const uint32_t type19 = ctx.r21.u32;
		uint32_t type19_p10 = 0;
		if (type19 != 0) {
			peek32(base, type19 + 0x10, type19_p10);
		}
		const uint32_t csavepoint = ctx.r24.u32;
		const uint32_t final_r4 = ctx.r4.u32;
		const uint32_t lr = static_cast<uint32_t>(ctx.lr);
		const uint32_t ctr = ctx.ctr.u32;
		emit(kSavePoint,
		     "sub_825B93F8 DISPATCH_SLOT_148 this/r23=0x%08X *(this+4)=0x%08X lookup_id=0x%08X "
		     "type19=0x%08X *(type19+0x10)=0x%08X CSavePoint=0x%08X final_r4=0x%08X LR=0x%08X CTR=0x%08X",
		     r23_this, this_p4, lookup_id, type19, type19_p10, csavepoint, final_r4, lr, ctr);
		if (type19_p10 != 0x00000001u) {
			emit(kSavePoint,
			     "sub_825B93F8 NON_ONE_TYPE19 this/r23=0x%08X lookup_id=0x%08X type19=0x%08X "
			     "*(type19+0x10)=0x%08X CSavePoint=0x%08X final_r4=0x%08X LR=0x%08X CTR=0x%08X",
			     r23_this, lookup_id, type19, type19_p10, csavepoint, final_r4, lr, ctr);
		}
	}
	REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
loc_825B9788:
	// addi r1,r1,208
	ctx.r1.s64 = ctx.r1.s64 + 208;
	// b 0x826ee218
	__restgprlr_20(ctx, base);
	return;
}
