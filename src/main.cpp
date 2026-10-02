// infinite_undiscovery - ReXGlue Recompiled Project

#include "generated/default/infinite_undiscovery_init.h"

#include "infinite_undiscovery_app.h"
#include "fiber_lr.h"

REXCVAR_DEFINE_BOOL(asset_setup, false, "Content",
                   "Show Asset Setup Wizard even when assets are configured")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

REX_DEFINE_APP(infinite_undiscovery, InfiniteUndiscoveryApp::Create)
