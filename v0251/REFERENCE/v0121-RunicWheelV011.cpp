// Runic Wheel LE v0.12.0; validated v0.11.2 game actions + optional read-only item labels.
// Copyright (c) 2026 Mhacks79. No third-party game/mod assets or Wheeler source.
// Uses the official SKSE 1.7.3 header for ABI; only Win32 C and D3D9 COM interfaces cross DLL boundaries.
// Native equipment actions gated by ownership/variant checks and executed on SKSE game tasks.
#define RUNIC_VERSION "0.12.0-readable-wheel"
#include "InputLogic.h"
#include "SlotCatalog.h"
#include "CatalogSave.h"
#include "CatalogEditor.h"
#include "BindingModel.h"
#include "SaveV2.h"
#include "ActionPolicy.h"
#include "NativeActionPolicy.h"
#include "SpellMembership.h"
#include "DisplayName.h"
#include "InventoryCountPolicy.h"
#include "BindSyntax.h"
#include "ContextPolicy.h"
#include "MenuTracker.h"
#include "TimePolicy.h"
#include "UITaskPolicy.h"
typedef unsigned long UInt32;
typedef unsigned long long UInt64;
#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "Signatures.h"

#define W __stdcall
#define IMPORT extern "C" __declspec(dllimport)
typedef void* Handle;
typedef long HResult;
typedef long Long;
typedef unsigned long DWord;
typedef unsigned long ULong;
typedef unsigned int UInit;
typedef unsigned long SizeT;
struct MBI {void* base; void* alloc; DWord allocProtect; SizeT regionSize; DWord state; DWord protect; DWord type;};
struct Rectangle {Long l,t,r,b;};
struct DeviceParameters {UInit adapter; UInit deviceType; Handle window; DWord behavior;};
struct Viewport {DWord x,y,w,h; float minZ,maxZ;};
struct Vertex {float x,y,z,rhw; DWord color;};
IMPORT void W OutputDebugStringA(const char*);
IMPORT DWord W GetCurrentThreadId();
IMPORT DWord W GetTickCount();
IMPORT UInit W GetTempPathA(UInit,char*);
IMPORT UInit W GetModuleFileNameA(Handle,char*,UInit);
IMPORT Long W GetModuleHandleExA(DWord,const char*,Handle*);
IMPORT Handle W CreateFileA(const char*,DWord,DWord,void*,DWord,DWord,Handle);
IMPORT Long W WriteFile(Handle,const void*,DWord,DWord*,void*);
IMPORT Long W CloseHandle(Handle);
IMPORT SizeT W VirtualQuery(const void*,MBI*,SizeT);
IMPORT Long W VirtualProtect(void*,SizeT,DWord,DWord*);
IMPORT Long W FlushInstructionCache(Handle,const void*,SizeT);
IMPORT Handle W GetCurrentProcess();
IMPORT Long W InterlockedCompareExchange(volatile Long*,Long,Long);
IMPORT Long W InterlockedExchange(volatile Long*,Long);
IMPORT Handle W GetForegroundWindow();
IMPORT Long W IsWindow(Handle);
IMPORT unsigned int W GetPrivateProfileIntA(const char*,const char*,int,const char*);
IMPORT unsigned int W GetPrivateProfileStringA(const char*,const char*,const char*,char*,unsigned int,const char*);
IMPORT Handle W GetKeyboardLayout(DWord);
IMPORT short W VkKeyScanExA(char,Handle);
struct NativeSRWLock {void* ptr;};
IMPORT void W AcquireSRWLockShared(NativeSRWLock*);
IMPORT void W ReleaseSRWLockShared(NativeSRWLock*);
IMPORT void W AcquireSRWLockExclusive(NativeSRWLock*);
IMPORT void W ReleaseSRWLockExclusive(NativeSRWLock*);
IMPORT unsigned int W MapVirtualKeyExA(unsigned int,unsigned int,Handle);
static_assert(sizeof(void*) == 4, "Win32 required");
static_assert(sizeof(MBI) == 28, "Win32 VirtualQuery layout");
static_assert(sizeof(DeviceParameters) == 16, "D3D9 creation params layout");
static_assert(sizeof(Vertex) == 20, "D3D9 XYZRHW+DIFFUSE stride");
extern "C" int _fltused=0; // Required by MSVC-target x86 code using floating point, no CRT entry point.

namespace {
using Raw = void*;
using Present = HResult (W*)(Raw,const Rectangle*,const Rectangle*,Handle,const void*);
// COM slot ordinals from IDirect3DDevice9 / IDirect3DStateBlock9 in the D3D9 SDK (not TESV virtual tables).
constexpr UInit kCreateParameters=9,kGetBackBuffer=18,kSetTarget=37,kGetTarget=38,kSetDepth=39,kGetDepth=40;
constexpr UInit kBeginScene=41,kEndScene=42,kViewport=48,kRenderState=57,kCreateBlock=59;
constexpr UInit kSetTexture=65,kTextureStage=67,kDrawUP=83,kFVF=89,kVertexShader=92,kPixelShader=107;
using GetViewport = HResult (W*)(Raw,Viewport*);
using GetParams = HResult (W*)(Raw,DeviceParameters*);
using GetBack = HResult (W*)(Raw,UInit,UInit,UInit,Raw*);
using GetSurface = HResult (W*)(Raw,UInit,Raw*);
using GetDepth = HResult (W*)(Raw,Raw*);
using SetSurface = HResult (W*)(Raw,UInit,Raw);
using SetDepth = HResult (W*)(Raw,Raw);
using Scene = HResult (W*)(Raw);
using CreateBlock = HResult (W*)(Raw,UInit,Raw*);
using Release = ULong (W*)(Raw);
using BlockAction = HResult (W*)(Raw);
using SetVal = HResult (W*)(Raw,DWord,DWord);
using SetFVF = HResult (W*)(Raw,DWord);
using SetObj = HResult (W*)(Raw,Raw);
using SetTexture = HResult (W*)(Raw,UInit,Raw);
using SetTextureStage = HResult (W*)(Raw,DWord,DWord,DWord);
using DrawUp = HResult (W*)(Raw,UInit,UInit,const void*,UInit);
static PluginHandle g_handle = kPluginHandle_Invalid;
static Present g_previous = nullptr;
static Raw g_device = nullptr;
static Handle g_window = nullptr;
static volatile Long g_installed=0, g_drawBusy=0, g_faults=0, g_visible=0;
static volatile Long g_firstFrame=0, g_select=0, g_activeWheel=0, g_item=0, g_actionFlash=0;
static volatile Long g_installBusy=0, g_pendingReset=0, g_resetInput=0, g_renderLogged=0, g_inputInstalled=0;
static Vertex g_geometry[48];
static Vertex g_pips[3*(8*4+12)];
static Vertex g_labels[30000];
static volatile Long g_feedback=0, g_feedbackTick=0;
// Presentation-only cache: game-thread copies fixed-size names, renderer reads under catalog lock.
static char g_displayNames[rw05::Wheels][rw05::Sectors][rw05::Capacity][rw12ui::NameBytes]{};
static volatile Long g_namesEnabled=1, g_namesReported=0;
static volatile Long g_namesResolved=0;
static int g_showNames=1;
static volatile Long g_uiKind=0;
static rw03::Catalog g_catalog{};
static volatile Long g_cursors[3][8]{};
static volatile Long g_formId=0;
static Long g_catalogSkipped=0;
// Zero-initialized SRWLock: Win32 Vista+; protect against callback/Present/input thread races.
static NativeSRWLock g_catalogLock{};
static rw05::State g_defaultState{};
static rw05::State g_queuedState{};
static rw05::State g_model{};
static rw05::Editor g_editor{}; // input owner thread only; bounded eight-step undo/redo
static volatile Long g_pendingApply=0;
// Open/close context from SKSE events; hovered form is read ONLY on the UI Render thread.
static volatile Long g_uiHover=0, g_uiSeen=0, g_uiReads=0, g_uiMisses=0, g_uiFaults=0;
static volatile Long g_uiEnabled=1;
static volatile Long g_uiTaskQueued=0, g_uiTaskTick=0, g_timeTaskQueued=0;
static volatile Long g_invMenuName=0, g_magicMenuName=0;
static SKSETaskInterface* g_tasks=nullptr;
static volatile Long g_timeActive=0;
static volatile Long g_menuFlags=0, g_menuSinkReady=0;
static volatile Long g_slowSignatureOK=0, g_timeFailures=0, g_timeBusy=0;
static float g_timePrevious=1.f, g_timeRequested=0.1f;
constexpr DWord kGlobalMultiplier=v07sig::kGlobalTimeMultiplier, kBSTimer=v07sig::kBSTimer;
constexpr DWord kSlowSetter=v07sig::kSetGlobalTimeMultiplier;
// These menu vtables are verified in TESV.exe LE (RTTI-derived, menu vtable slot 6).
static int g_slowPercent=10;

constexpr UInt32 kSaveRecord=0x45485752u; // 'RWHE' little-endian
constexpr UInt32 kSaveUnique=0x314C5752u; // 'RWL1' unique id
constexpr UInt32 kSaveVersion=2;
static char g_logPath[270];
static char g_configPath[270];
static int g_logReady=0;
static char g_fallbackLogPath[270]{};
static int g_enableSlowTime=1;
static int g_enableSpellActions=1; // spell-only rollback; leave working inventory actions enabled
static int g_hoverCaptureEnabled=0;
static volatile Long g_fallbackLogReady=0, g_contextWarningLogged=0;

void log_to_path(const char* path,const char* s){
    if(!path || !*path)return;
    Handle f=CreateFileA(path,4u,3,nullptr,4,0x80,nullptr);
    if(f==(Handle)(-1))return;
    DWord n=0;while(s[n] && n<500)n++;
    DWord done=0;(void)WriteFile(f,s,n,&done,nullptr);CloseHandle(f);
}
void file_log(const char* s) {
    OutputDebugStringA(s);
    if(g_logReady)log_to_path(g_logPath,s);
    if(g_fallbackLogReady)log_to_path(g_fallbackLogPath,s);
}
void begin_log() {
    // Query runs before Load. Always attempt a writable fallback independent
    // of SKSE/MO2's virtual Data folder, which may hide plugin-local logs.
    if(InterlockedCompareExchange(&g_fallbackLogReady,1,0)==0){
        UInit n=GetTempPathA((UInit)sizeof(g_fallbackLogPath),g_fallbackLogPath);
        const char suffix[]="RunicWheelLE-diagnostic.log";
        if(n && n+sizeof(suffix)<sizeof(g_fallbackLogPath)){
            for(UInit i=0;i<sizeof(suffix);++i)g_fallbackLogPath[n+i]=suffix[i];
            Handle f=CreateFileA(g_fallbackLogPath,0x40000000u,3,nullptr,2,0x80,nullptr);
            if(f!=(Handle)(-1))CloseHandle(f);
        }else{g_fallbackLogPath[0]=0;}
    }
    if(g_logReady)return;
    Handle h=nullptr;
    if(!GetModuleHandleExA(4|2,reinterpret_cast<const char*>(&begin_log),&h) || !h)return;
    UInit len=GetModuleFileNameA(h,g_logPath,sizeof(g_logPath));
    if(!len||len>sizeof(g_logPath)-22)return;
    UInit start=0; for(UInit i=0;i<len;++i)if(g_logPath[i]=='\\'||g_logPath[i]=='/')start=i+1;
    const char suffix[]="RunicWheelLE.log";
    for(UInit i=0;suffix[i];++i)g_logPath[start+i]=suffix[i];
    g_logPath[start+sizeof(suffix)-1]=0;
    const char iniName[]="RunicWheelLE.ini";
    for(UInit i=0;iniName[i];++i)g_configPath[start+i]=iniName[i];
    g_configPath[start+sizeof(iniName)-1]=0;
    g_logReady=1;
    Handle f=CreateFileA(g_logPath,0x40000000u,3,nullptr,2,0x80,nullptr);
    if(f!=(Handle)(-1))CloseHandle(f);
}
void format_thread_log(const char* prefix) {
    char buf[150]; UInit p=0;
    while(prefix[p]&&p<90){buf[p]=prefix[p];++p;}
    DWord id=GetCurrentThreadId(); char digits[16];UInit n=0;
    do {digits[n++]='0'+char(id%10);id/=10;}while(id&&n<15);
    while(n)buf[p++]=digits[--n];
    buf[p++]='\r';buf[p++]='\n';buf[p]=0;file_log(buf);
}
Raw get(Raw obj,UInit slot) {return (*reinterpret_cast<Raw**>(obj))[slot];}
int succeeded(HResult code){return code>=0;}
int readable(const void* p,SizeT n,int code){
    if(!p||n==0)return 0;
    unsigned long addr=(unsigned long)p;
    if(addr<0x10000u||addr>=0xffff0000u||n>0xffff0000u-addr)return 0;
    MBI m{};
    if(VirtualQuery(p,&m,sizeof(m))!=sizeof(m)||m.state!=0x1000 || (m.protect & (0x100|1)))return 0;
    if(addr+n > (unsigned long)m.base + m.regionSize)return 0;
    DWord protection=m.protect & 255;
    if(code)return protection==16||protection==32||protection==64||protection==128;
    return protection==2||protection==4||protection==8||protection==32||protection==64||protection==128;
}
void drop(Raw obj){if(obj)reinterpret_cast<Release>(get(obj,2))(obj);}

// Unsafe V0.7 Render detours were removed from this compilation unit.


// Use the standard SKSE MenuManager MenuOpenCloseEvent dispatcher, not
// unverified per-frame InventoryMenu::Render vtable detours.
// Event name is a BSFixedString pointing to an interned char* (SKSE GameEvents.h).
struct MenuEventData {const char* name; unsigned char opening; unsigned char pad[3];};
struct MenuSink {Raw* methods;};
static void __fastcall menu_sink_dtor(MenuSink*,void*){}
static int __fastcall menu_sink_receive(MenuSink*,void*,const MenuEventData* e,Raw){
    if(!e || !readable(e,sizeof(MenuEventData),0) || !e->name || !readable(e->name,1,0))return 0;
    const unsigned flags=rw08::identify(e->name);
    if(flags){
        Long previous=0,newFlags=0,exchange=0;
        do{previous=InterlockedCompareExchange(&g_menuFlags,0,0);
           newFlags=e->opening?(previous|(Long)flags):(previous&~(Long)flags);
           exchange=InterlockedCompareExchange(&g_menuFlags,newFlags,previous);
        }while(exchange!=previous);
        if(flags&1)InterlockedExchange(&g_invMenuName,e->opening?(Long)e->name:0);
        if(flags&2)InterlockedExchange(&g_magicMenuName,e->opening?(Long)e->name:0);
        InterlockedExchange(&g_uiHover,0);
        InterlockedExchange(&g_uiSeen,0);
        file_log(e->opening ? "RunicWheelLE: Inventory/Magic context OPEN (event)\r\n": "RunicWheelLE: Inventory/Magic context CLOSED (event)\r\n");
    }
    return 0; // BSTEventSink::kEvent_Continue
}
static Raw g_menuSinkMethods[2]={(Raw)&menu_sink_dtor,(Raw)&menu_sink_receive};
static MenuSink g_menuSink={g_menuSinkMethods};
int menu_register_fault(){file_log("RunicWheelLE: menu event registration fault; context tracking unavailable\r\n");return 1;}
void install_menu_lifecycle(){
    if(g_menuSinkReady)return;
    Raw global=(Raw)v07sig::kMenuManagerSingleton; // CommonLibLE g_menuManager
    if(!readable(global,4,0))return;
    Raw manager=*(Raw*)global;
    if(!readable(manager,0x34,0))return;
    // TESV LE 0x006E3E30, thiscall(EventDispatcher*, BSTEventSink*), ret 4
    // EventDispatcher is embedded at MenuManager+0x04 (official SKSE LE SDK).
    Raw add=(Raw)v07sig::kEventDispatcherAddEventSink;
    if(!readable(add,9,1))return;
    const unsigned char bytes[]={0x53,0x8b,0x5c,0x24,0x08,0x57,0x8b,0xf9};
    for(unsigned i=0;i<sizeof(bytes);++i)if(((const unsigned char*)add)[i]!=bytes[i])return;
    __try {
        reinterpret_cast<void (__thiscall*)(Raw,MenuSink*)>(add)((Raw)((unsigned char*)manager+4),&g_menuSink);
        InterlockedExchange(&g_menuSinkReady,1);
        file_log("RunicWheelLE: SKSE MenuOpenCloseEvent lifecycle sink registered\r\n");
    }__except(menu_register_fault()){}
}
// SKSE's AddUITask API (interface v2, SKSE 1.7.3) is the only code path
// touching a GFxMovieView. No InventoryMenu::Render detour or game vtable write.
// Every UI task is bounded, reads only one live menu and publishes scalar values.
// Menu names are interned pointers from MenuOpenCloseEvent, used as identity
// only (never dereferenced on our UI scan); closed menus are rejected.
#pragma pack(push,4)
struct GfxNumber {Raw iface; DWord type; union {double number;Raw object;unsigned char bytes[8];} data;};
#pragma pack(pop)
static_assert(sizeof(GfxNumber)==16,"SKSE LE GFxValue must be 0x10");
using GetVariable=bool (__thiscall*)(Raw,GfxNumber*,const char*);
using FxRelease=void (__thiscall*)(Raw,GfxNumber*,Raw);
void clear_ui_value(GfxNumber& v){
    if((v.type&0x40) && v.iface && v.data.object && readable(v.iface,4,0) && readable((void*)v07sig::kGFxReleaseManaged,4,1))
        reinterpret_cast<FxRelease>(v07sig::kGFxReleaseManaged)(v.iface,&v,v.data.object);
}
rw05::Kind form_kind(DWord kind){
    switch(kind){case 0x29:return rw05::Weapon;case 0x1A:return rw05::Armor;
      case 0x16:return rw05::Spell;case 0x77:return rw05::Shout;
      case 0x2E:case 0x1E:return rw05::Consumable;case 0x2A:return rw05::Ammo;
      default:return rw05::Unspecified;}
}
DWord ui_numeric(const GfxNumber& v){
    if((v.type&0x8Fu)!=3u || !(v.data.number>=1.0 && v.data.number<=4294967295.0))return 0;
    return (DWord)v.data.number;
}
Raw active_movie(){
    const Long flags=InterlockedCompareExchange(&g_menuFlags,0,0);
    const DWord wanted=(DWord)InterlockedCompareExchange((flags&1)?&g_invMenuName:&g_magicMenuName,0,0);
    if(!(flags&3) || !wanted)return nullptr;
    Raw global=(Raw)v07sig::kMenuManagerSingleton;
    if(!readable(global,4,0))return nullptr;
    Raw manager=*(Raw*)global;
    if(!readable(manager,0xC0,0))return nullptr;
    // LE SKSE 1.7.3 GameMenus.h: MenuManager::menuTable at +0xA4,
    // tHashSet.m_size at +4, m_entries at +0x18, MenuTableItem 12 bytes.
    const unsigned char* table=(const unsigned char*)manager+0xA4;
    const DWord capacity=*(const DWord*)(table+4);
    Raw entries=*(Raw const*)(table+0x18);
    if(!rw10::valid_capacity(capacity) || !readable(entries,16,0))return nullptr;
    const unsigned char* row=(const unsigned char*)entries;
    for(DWord i=0;i<capacity;++i,row+=16){
        if(!readable(row,16,0))return nullptr;
        if(*(const DWord*)row!=wanted)continue;
        Raw menu=*(Raw const*)(row+4);
        if(!readable(menu,0x0C,0))return nullptr;
        Raw view=*(Raw const*)((const unsigned char*)menu+0x08);
        if(!readable(view,4,0))return nullptr;
        return view;
    }
    return nullptr;
}
void ui_sample_form(){
    if(!g_uiEnabled || !g_hoverCaptureEnabled || !(g_menuFlags&3))return;
    Raw view=active_movie();
    if(!view)return;
    Raw* vt=*(Raw**)view;
    // SKSE ScaleformMovie.h: GFxMovieView::GetVariable virtual slot 17.
    if(!readable(vt,18*4,0)||!readable(vt[17],1,1))return;
    const char* paths[]={
        "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId",
        "_root.Menu_mc.inventoryLists.itemList.selectedEntry.baseId",
        "_root.Menu_mc.inventoryLists.selectedEntry.formId",
        "_root.Menu_mc.itemList.selectedEntry.formId"};
    DWord id=0;
    for(unsigned i=0;i<sizeof(paths)/sizeof(paths[0]);++i){
        GfxNumber v{};
        const bool found=reinterpret_cast<GetVariable>(vt[17])(view,&v,paths[i]);
        id=found?ui_numeric(v):0;
        clear_ui_value(v);
        if(id)break;
    }
    int kind=0;
    if(id && readable((void*)v07sig::kLookupFormByID,8,1)){
        Raw form=reinterpret_cast<Raw (__cdecl*)(DWord)>(v07sig::kLookupFormByID)(id);
        if(readable(form,0x14,0) && *(const DWord*)((const unsigned char*)form+0x0C)==id)
            kind=(int)form_kind(*(const unsigned char*)((const unsigned char*)form+0x12));
    }
    InterlockedExchange(&g_uiKind,kind);
    InterlockedExchange(&g_uiHover,(Long)id);
    InterlockedExchange(&g_uiSeen,(Long)GetTickCount());
    if(id && InterlockedCompareExchange(&g_uiReads,1,0)==0)
        file_log("RunicWheelLE: UI task resolved selected FormID (READ-ONLY)\r\n");
    else if(!id && InterlockedCompareExchange(&g_uiMisses,1,0)==0)
        file_log("RunicWheelLE: UI selectedEntry not available; edit add safely refused\r\n");
}
int ui_read_fault(){
    if(InterlockedCompareExchange(&g_uiFaults,1,0)==0)
        file_log("RunicWheelLE: UI task SEH; disabled for session\r\n");
    InterlockedExchange(&g_uiEnabled,0);
    InterlockedExchange(&g_uiHover,0);
    InterlockedExchange(&g_uiKind,0);
    return 1;
}
struct UITask {Raw* methods;};
void __fastcall run_ui_task(UITask*,void*){
    __try{ui_sample_form();}__except(ui_read_fault()){}
}
void __fastcall dispose_ui_task(UITask*,void*){
    InterlockedExchange(&g_uiTaskQueued,0);
}
static Raw g_uiTaskMethods[2]={(Raw)&run_ui_task,(Raw)&dispose_ui_task};
static UITask g_uiTask={g_uiTaskMethods};
void queue_ui_sample(){
    if(!g_tasks || !g_tasks->AddUITask)return;
    const DWord now=GetTickCount();
    if(!rw10::sample_allowed((unsigned)g_menuFlags, g_hoverCaptureEnabled && g_uiEnabled,
                            (DWord)(now-(DWord)g_uiTaskTick)))return; // max ~11 UI tasks per second
    if(InterlockedCompareExchange(&g_uiTaskQueued,1,0)!=0)return;
    InterlockedExchange(&g_uiTaskTick,(Long)now);
    g_tasks->AddUITask((UIDelegate_v1*)&g_uiTask);
}
bool inventory_context(){
    return (InterlockedCompareExchange(&g_menuFlags,0,0)&3)!=0;
}
// BSTimer setter comes from the published and game-tested Studio Mode LE code.
// This call occurs only on the game's input dispatch thread, never from Present.
bool time_signature_ok(){
    const unsigned char* code=(const unsigned char*)kSlowSetter;
    if(!readable(code,11,1)||!readable((void*)kBSTimer,0x2d,0)||!readable((void*)kGlobalMultiplier,4,0))return false;
    const unsigned char expected[]={0x80,0x79,0x2c,0x00,0x74,0x18,0x80,0x7c,0x24,0x08,0x00};
    for(unsigned i=0;i<sizeof(expected);++i)if(code[i]!=expected[i])return false;
    return true;
}
void sync_time_impl(){
    // Restoring our time change must NEVER depend on MenuOpenCloseEvent hooking.
    // Also permit slowdown in world space when the menu observer failed to
    // register; print the uncertainty so users can isolate it from time bugs.
    if(!g_enableSlowTime && !g_timeActive)return;
    if(!g_menuSinkReady && InterlockedCompareExchange(&g_contextWarningLogged,1,0)==0)
        file_log("RunicWheelLE: menu listener unavailable; time controller active, context UNKNOWN\r\n");
    if(!g_slowSignatureOK){
        if(time_signature_ok()){InterlockedExchange(&g_slowSignatureOK,1);file_log("RunicWheelLE: BSTimer time setter signature VERIFIED\r\n");}
        else {if(InterlockedCompareExchange(&g_timeFailures,1,0)==0)file_log("RunicWheelLE: BSTimer signature/memory INVALID; slowdown disabled\r\n");return;}
    }
    bool requested=rw09::wants_slowdown(g_visible!=0,
        (unsigned)InterlockedCompareExchange(&g_menuFlags,0,0),
        g_enableSlowTime!=0);
    float current=*(volatile float*)kGlobalMultiplier;
    if(!(current>0.009f && current<100.0f)){
        if(InterlockedCompareExchange(&g_timeFailures,1,0)==0)file_log("RunicWheelLE: invalid current game time multiplier\r\n");return;
    }
    if(requested && !g_timeActive){
        g_timePrevious=current;
        g_timeRequested=rw09::target_speed(current,g_slowPercent);
        reinterpret_cast<void (__thiscall*)(Raw,float,bool)>(kSlowSetter)((Raw)kBSTimer,g_timeRequested,true);
        float observed=*(volatile float*)kGlobalMultiplier;
        if(observed>g_timeRequested-0.005f && observed<g_timeRequested+0.005f){
            InterlockedExchange(&g_timeActive,1);
            file_log("RunicWheelLE: SLOWDOWN applied and verified by readback\r\n");
        }else file_log("RunicWheelLE: slow-time setter had no expected observable effect\r\n");
    }else if(!requested && g_timeActive){
        if(rw09::still_owns_multiplier(current,g_timeRequested)){
            reinterpret_cast<void (__thiscall*)(Raw,float,bool)>(kSlowSetter)((Raw)kBSTimer,g_timePrevious,true);
            file_log("RunicWheelLE: world time RESTORE requested\r\n");
        }else file_log("RunicWheelLE: another mod changed time; our restore skipped\r\n");
        InterlockedExchange(&g_timeActive,0);
    }
}
int time_fault(){
    g_enableSlowTime=0;
    file_log("RunicWheelLE: time setter SEH exception; disabled for rest of session\r\n");
    return 1;
}
void sync_time(){
    if(InterlockedCompareExchange(&g_timeBusy,1,0)!=0)return;
    __try {sync_time_impl();} __except(time_fault()) {}
    InterlockedExchange(&g_timeBusy,0);
}

// Reconcile a lost-focus or load-reset time change on SKSE's game-task
// thread, never on D3D Present thread. Only schedule when there is a
// previously applied multiplier that requires restoration.
struct GameTimeTask {Raw* methods;};
void __fastcall run_time_task(GameTimeTask*,void*){ sync_time(); }
void __fastcall dispose_time_task(GameTimeTask*,void*){
    InterlockedExchange(&g_timeTaskQueued,0);
}
static Raw g_timeTaskMethods[2]={(Raw)&run_time_task,(Raw)&dispose_time_task};
static GameTimeTask g_timeTask={g_timeTaskMethods};
void schedule_time_reconcile(){
    if(!g_tasks || !g_tasks->AddTask || !g_timeActive || g_visible)return;
    if(InterlockedCompareExchange(&g_timeTaskQueued,1,0)!=0)return;
    g_tasks->AddTask((TaskDelegate*)&g_timeTask);
}

// Skyrim LE's verified input dispatcher, not a Windows cursor: relative mouse
// motion survives the game locking/recentering its hardware cursor.
struct InputEvent { Raw vtbl; DWord device; DWord type; InputEvent* next; const char* control; };
struct ButtonEvent { InputEvent base; DWord scan; float value; float duration; };
struct MouseMove { InputEvent base; DWord scan; Long dx; Long dy; };
struct StickEvent { InputEvent base; DWord mask; float x; float y; };
static_assert(sizeof(InputEvent)==20 && sizeof(ButtonEvent)==32 && sizeof(MouseMove)==32 && sizeof(StickEvent)==32,"LE event layout");
using SendEvents=void (__thiscall*)(void*,InputEvent**);
static SendEvents g_previousInput=nullptr;
static rw02::Activation g_activation{}; // exclusively owned by the input dispatch thread
static Long g_mouseX=0,g_mouseY=0;
struct Settings {
    int openMouse, keyboardOpen, previousKey, nextKey, keyboardMode;
    int prevMouse,nextMouse,holdMs,sensitivity,deadzone;
    int suppress,gamepad,padOpen,padPrev,padNext,padStick;
    int primaryMouse,secondaryMouse,wheelUp,wheelDown,closeKey;
    int padPrimary,padSecondary,padDeadzone,padItemPrevious,padItemNext;
    int wheelScale;
    int nativeActions;
    int wheelOpacity,feedbackDuration;
};
// An explicit constant initializer is mandatory with /NOENTRY (no C++ constructors).
static Settings g_cfg={2,0,16,18,2, 3,4,260,100,22, 1,1,9,0,10,12, 0,1,8,9,1, 512,256,45,4,8,130,1,70,900};

int value(const char* key,int def,int lo,int hi){return rw02::clamp((int)GetPrivateProfileIntA("Controls",key,def,g_configPath),lo,hi);}
void load_catalog(){
    g_catalog={};g_catalogSkipped=0;
    for(int w=0;w<rw03::Wheels;++w)for(int s=0;s<rw03::Slots;++s){
        // Keys Wheel1Slot1 ... Wheel3Slot8, comma-separated FormIDs.
        char key[]="Wheel1Slot1";
        key[5]=char('1'+w);key[10]=char('1'+s);
        char line[160]{};
        GetPrivateProfileStringA("Bindings",key,"",line,sizeof(line),g_configPath);
        g_catalogSkipped+=rw03::parse_slot(line,g_catalog.slots[w][s]);
        InterlockedExchange(&g_cursors[w][s],0);
    }
}
void visual_feedback(int code){
    InterlockedExchange(&g_feedback,code);
    InterlockedExchange(&g_feedbackTick,(Long)GetTickCount());
}
void select_current(){
    AcquireSRWLockShared(&g_catalogLock);
    __try {
        int wheel=rw02::clamp((int)g_activeWheel,0,rw03::Wheels-1);
        int sector=rw02::clamp((int)g_select,0,rw03::Slots-1);
        int cursor=(int)InterlockedCompareExchange(&g_cursors[wheel][sector],0,0);
        InterlockedExchange(&g_item,cursor);
        InterlockedExchange(&g_formId,(Long)rw03::entry(g_catalog,wheel,sector,cursor));
    } __finally { ReleaseSRWLockShared(&g_catalogLock); }
}
void cycle_entry(int direction){
    int wheel=rw02::clamp((int)g_activeWheel,0,rw03::Wheels-1);
    int sector=rw02::clamp((int)g_select,0,rw03::Slots-1);
    int count=0;
    AcquireSRWLockShared(&g_catalogLock);
    __try {count=(int)g_catalog.slots[wheel][sector].count;}
    __finally {ReleaseSRWLockShared(&g_catalogLock);}
    if(count){
       int next=rw03::wrap((int)g_cursors[wheel][sector]+direction,count);
       InterlockedExchange(&g_cursors[wheel][sector],next);
    }
    select_current();
}
void snapshot_state_locked(rw05::State& state);
// Native operations are always deferred to the SKSE game task queue.
// No TESV equipment call is permitted from DirectX9, the UI callback or the input hook.
struct ActionRequest {rw05::Binding binding{}; unsigned generation=0; int hand=0;};
static ActionRequest g_actionRequest{};
static volatile Long g_actionQueued=0,g_actionGeneration=1,g_actionFaulted=0;
static volatile Long g_refreshQueued=0,g_refreshNeeded=1;
static_assert(sizeof(rw05::Binding)==12,"serialized native Binding ABI");
using GameDynamicCast=Raw (__cdecl*)(Raw,DWord,Raw,Raw,DWord);
using GameEquip=void (__thiscall*)(Raw,Raw,Raw,Raw,Long,Raw,bool,bool,bool,Raw);
// Proven in TESV.exe LE: the native MAGIC MENU routes SpellItem to this distinct method.
// FormType 0x16 -> MagicMenu 0x00873A9A; ret 0x0C (three stack args, this in ECX).
using GameEquipSpell=void (__thiscall*)(Raw,Raw,Raw,Raw);
using GameUnequip=bool (__thiscall*)(Raw,Raw,Raw,Raw,Long,Raw,bool,bool,bool,bool,Raw);

bool native_sites_verified(){
    static const unsigned char equip[]={0x83,0xec,0x18,0x53,0x8b,0x5c,0x24,0x20,0x55,0x8b,0xe9};
    static const unsigned char unequip[]={0x83,0xec,0x18,0x53,0x55,0x8b,0x6c,0x24,0x24,0x32,0xc0};
    static const unsigned char equipSpell[]={0x83,0xec,0x08,0x53,0x8b,0x5c,0x24,0x18,0x55,0x56,0x8b,0x74};
    if(!readable((Raw)0x006EF3E0,11,1)||!readable((Raw)0x006EE560,11,1)||
       !readable((Raw)0x006EFAC0,sizeof(equipSpell),1)||
       !readable((Raw)0x00451A30,6,1)||!readable((Raw)0x00F51DAE,5,1))return false;
    for(unsigned i=0;i<11;++i)if(((unsigned char*)0x006EF3E0)[i]!=equip[i]||((unsigned char*)0x006EE560)[i]!=unequip[i])return false;
    for(unsigned i=0;i<sizeof(equipSpell);++i)if(((unsigned char*)0x006EFAC0)[i]!=equipSpell[i])return false;
    return true;
}
// The two safe pointers below are LE SKSE 1.7.3 globals. Ownership comes from
// actor+0x48 -> ExtraContainerChanges -> Data::objList, NOT LookupFormByID.
Raw extra_container_changes(Raw actor){
    if(!readable(actor,0x54,0))return nullptr;
    Raw next=*(Raw*)((unsigned char*)actor+0x48);
    if(!readable((Raw)0x0123C200,1,0)||!readable((Raw)0x0123DA64,1,0))return nullptr;
    for(unsigned j=0;next && j<96;++j){
        if(!readable(next,8,0))return nullptr;
        Raw result=reinterpret_cast<GameDynamicCast>(0x00F51DAE)(next,0,(Raw)0x0123C200,(Raw)0x0123DA64,0);
        if(result)return result;
        next=*(Raw*)((unsigned char*)next+4);
    }
    return nullptr;
}
bool has_extra_type(Raw list,unsigned type){
    if(!readable(list,8,0))return false;
    Raw presence=*(Raw*)((unsigned char*)list+4);
    if(!readable(presence,0x18,0))return false;
    return ((((const unsigned char*)presence)[type/8]) & (1u<<(type%8)))!=0u;
}
struct OwnedItem {int count=0;Raw wornRight=nullptr,wornLeft=nullptr;bool ambiguous=false;};
// Inventory ownership is BASE TESNPC count + ExtraContainerChanges countDelta.
// SKSE GameExtraData.cpp CreateEquipEntryData/GetEquipItemData use precisely this
// aggregation. Refusing non-positive *delta alone* incorrectly rejects a
// base-owned shield or ingredient. Never dereference TESV pointers from DX9.
struct CountEvidence {int base=0,delta=0,total=0;bool baseReadable=false,deltaFound=false;};
bool npc_base_count(Raw actor,Raw form,int& count){
    count=0;
    if(!readable(actor,0x28,0))return false;
    Raw npc=*(Raw*)((unsigned char*)actor+0x24);
    if(!readable(npc,0x64,0) || *(const unsigned char*)((unsigned char*)npc+0x12)!=0x2B)return false;
    // TESActorBase::container +0x58; TESContainer::entries +4, numEntries +8.
    const unsigned char* container=(const unsigned char*)npc+0x58;
    Raw* entries=*(Raw* const*)(container+4);
    const DWord size=*(const DWord*)(container+8);
    if(size>4096 || (size && !readable(entries,size*sizeof(Raw),0)))return false;
    for(DWord i=0;i<size;++i){
        Raw entry=entries[i];
        if(!entry)continue;
        if(!readable(entry,12,0))return false;
        if(*(Raw*)((unsigned char*)entry+4)==form){
            const DWord n=*(DWord*)entry;
            if(n>100000u || count>100000-(int)n)return false;
            count+=(int)n;
        }
    }
    return true;
}
bool owned_inventory(Raw actor,Raw form,OwnedItem& out){
    CountEvidence count{};
    count.baseReadable=npc_base_count(actor,form,count.base);
    if(!count.baseReadable)return false; // Never claim possession if base inventory inaccessible.
    Raw extra=extra_container_changes(actor);
    if(extra){
      if(!readable(extra,12,0))return false;
      Raw data=*(Raw*)((unsigned char*)extra+8);
      if(!readable(data,4,0))return false;
      Raw objList=*(Raw*)data;
      if(!readable(objList,8,0))return false;
      Raw node=objList;
      for(unsigned i=0;node && i<2048;++i){
        if(!readable(node,8,0))return false;
        Raw item=*(Raw*)node;
        if(readable(item,12,0) && *(Raw*)item==form){
            if(count.deltaFound)return false; // duplicate entry => ambiguous
            count.deltaFound=true;
            const Long delta=*(Long*)((unsigned char*)item+8);
            if(delta < -100000 || delta >100000)return false;
            count.delta=(int)delta;
            Raw extend=*(Raw*)((unsigned char*)item+4);
            if(extend){
                if(!readable(extend,8,0))return false;
                Raw link=extend;
                for(unsigned j=0;j<128 && link;++j){
                    if(!readable(link,8,0))return false;
                    Raw extraList=*(Raw*)link;
                    if(extraList){
                        if(!readable(extraList,8,0))return false;
                        // Names, tempering, enchantment, poison are NOT a unique base form.
                        if(has_extra_type(extraList,0x99)||has_extra_type(extraList,0x9b)||
                           has_extra_type(extraList,0x25)||has_extra_type(extraList,0x3e))out.ambiguous=true;
                        if(has_extra_type(extraList,0x16))out.wornRight=extraList;
                        if(has_extra_type(extraList,0x17))out.wornLeft=extraList;
                    }
                    link=*(Raw*)((unsigned char*)link+4);
                }
                if(link)return false;
            }
        }
        node=*(Raw*)((unsigned char*)node+4);
        if(i==2047 && node)return false;
      }
    }
    if(!rw121::merge_inventory_count(count.base,count.delta,count.total))return false;
    out.count=count.total;
    if(count.base>0)file_log("RunicWheelLE: ownership resolved with TESNPC base+delta inventory\r\n");
    return true;
}
// Diagnostic includes exact formID and TESV form type so two refused clicks can
// be attributed to individual categories rather than inferred from log order.
void log_item_refusal(const char* reason,DWord id,unsigned type){
    char buf[152];unsigned n=0;
    const char* p="RunicWheelLE: ";while(*p&&n<70)buf[n++]=*p++;
    p=reason;while(*p&&n<100)buf[n++]=*p++;
    p=" form=";while(*p)buf[n++]=*p++;
    const char* digit="0123456789ABCDEF";
    for(int i=7;i>=0;--i)buf[n++]=digit[(id>>(i*4))&15u];
    p=" type=";while(*p)buf[n++]=*p++;
    buf[n++]=digit[(type>>4)&15u];buf[n++]=digit[type&15u];
    buf[n++]='\r';buf[n++]='\n';buf[n]=0;file_log(buf);
}
// LE SKSE 1.7.3 exposes three distinct spell stores. Actor::addedSpells
// is NOT equivalent to all learned/available spells (NPC base + race can supply
// spells). A positive match is required; malformed or unreadable lists fail closed.
struct SpellEvidence {unsigned actorCount=0,baseCount=0,raceCount=0;unsigned source=0;};
bool actor_added_spell(Raw actor,Raw form,SpellEvidence& proof){
    if(!readable(actor,0x108,0))return false;
    const unsigned char* data=(const unsigned char*)actor+0xFC;
    const DWord allocated=*(const DWord*)data;
    const DWord count=*(const DWord*)(data+8);
    if(!rw12::valid_count(count))return false;
    proof.actorCount=count;
    Raw entries=*(Raw const*)(data+4);
    if(allocated&0x80000000u)return count==1 && entries==form;
    if(!readable(entries,count*sizeof(Raw),0))return false;
    return rw12::contains((Raw const*)entries,count,form);
}
bool form_spell_list(Raw owner,unsigned offset,Raw form,unsigned& countOut){
    if(!readable(owner,offset+8,0))return false;
    Raw data=*(Raw*)((unsigned char*)owner+offset+4); // TESSpellList::unk04
    if(!readable(data,0x18,0))return false;
    const unsigned count=*(DWord*)((unsigned char*)data+0x0C); // numSpells
    if(!rw12::valid_count(count))return false;
    countOut=count;
    Raw entries=*(Raw*)data; // SpellItem** spells
    if(!readable(entries,count*sizeof(Raw),0))return false;
    return rw12::contains((Raw const*)entries,count,form);
}
bool learned_spell(Raw actor,Raw form,SpellEvidence& proof){
    if(!readable(actor,0x134,0)||!readable(form,0x14,0))return false;
    if(actor_added_spell(actor,form,proof)){proof.source=rw12::ActorAdded;return true;}
    Raw base=*(Raw*)((unsigned char*)actor+0x24); // TESObjectREFR::baseForm
    if(readable(base,0x6C,0) && *((unsigned char*)base+0x12)==0x2B){ // TESNPC
        if(form_spell_list(base,0x64,form,proof.baseCount)){
            proof.source=rw12::NpcBase;return true;
        }
    }
    Raw race=*(Raw*)((unsigned char*)actor+0x130); // Actor::race
    if(readable(race,0x30,0) && *((unsigned char*)race+0x12)==0x0E){ // TESRace
        if(form_spell_list(race,0x28,form,proof.raceCount)){
            proof.source=rw12::Race;return true;
        }
    }
    return false;
}
void spell_failure_log(const SpellEvidence& proof){
    // One bounded diagnostic per rejected click; useful without exposing any game data.
    char buffer[180]; unsigned pos=0;
    const char prefix[]="RunicWheelLE: SPELL refused, checked stores; actor/base/race counts=";
    for(unsigned i=0;prefix[i] && pos<120;++i)buffer[pos++]=prefix[i];
    const unsigned numbers[]={proof.actorCount,proof.baseCount,proof.raceCount};
    for(unsigned n=0;n<3;++n){
       char digits[11];unsigned length=0;unsigned value=numbers[n];
       do{digits[length++]=(char)('0'+value%10u);value/=10u;}while(value && length<10);
       while(length)buffer[pos++]=digits[--length];
       if(n<2)buffer[pos++]='/';
    }
    buffer[pos++]='\r';buffer[pos++]='\n';buffer[pos]=0;file_log(buffer);
}

void action_diagnostic(const char* s,int code){visual_feedback(code);file_log(s);}
void perform_action(const ActionRequest& request){
    if(!g_cfg.nativeActions){action_diagnostic("RunicWheelLE: action feature disabled by INI\r\n",4);return;}
    if(request.generation!=(unsigned)InterlockedCompareExchange(&g_actionGeneration,0,0) || inventory_context())return;
    if(!native_sites_verified()){action_diagnostic("RunicWheelLE: native signature mismatch; no mutation\r\n",4);return;}
    if(!readable((Raw)0x01B2E8E4,4,0)||!readable((Raw)0x012E5FAC,4,0))return;
    Raw actor=*(Raw*)0x01B2E8E4, manager=*(Raw*)0x012E5FAC;
    if(!readable(actor,0x140,0)||!readable(manager,4,0)){
      action_diagnostic("RunicWheelLE: actor/equip manager unavailable\r\n",4);return;
    }
    Raw form=reinterpret_cast<Raw (__cdecl*)(DWord)>(0x00451A30)(request.binding.formId);
    if(!readable(form,0x14,0)||*(DWord*)((unsigned char*)form+0x0C)!=request.binding.formId){
      action_diagnostic("RunicWheelLE: unavailable form rejected\r\n",4);return;
    }
    rw05::Kind kind=form_kind(*((const unsigned char*)form+0x12));
    if(request.binding.kind && (unsigned)kind!=request.binding.kind){
      action_diagnostic("RunicWheelLE: type mismatch refused\r\n",4);return;
    }
    if(request.binding.variantId){
      action_diagnostic("RunicWheelLE: variant hash needs exact instance, refused\r\n",4);return;
    }
    // LE SDK GetLeftHandSlot/GetRightHandSlot are no-argument game functions.
    Raw slot=nullptr;
    if(request.hand==0)slot=reinterpret_cast<Raw (__cdecl*)()>(0x0054C810)();
    else slot=reinterpret_cast<Raw (__cdecl*)()>(0x0054C840)();
    if(!readable(slot,4,0)){action_diagnostic("RunicWheelLE: invalid hand slot\r\n",4);return;}
    OwnedItem owned{};
    if(kind==rw05::Spell){
        if(!g_enableSpellActions){
          action_diagnostic("RunicWheelLE: spell actions disabled by EnableSpellActions=0\r\n",4);return;
        }
        rw11::Facts facts{};
        facts.focused=true;facts.inGame=true;facts.actorReady=true;
        SpellEvidence proof{};
        facts.actualKind=kind;facts.learned=learned_spell(actor,form,proof);
        if(rw11::decide(request.binding,facts)!=rw11::EquipSpell){
          spell_failure_log(proof);
          action_diagnostic("RunicWheelLE: spell not verified in actor, NPC base or race lists\r\n",4);return;
        }
        // Distinguish evidence source so a player log can establish which
        // spell store actually provided the positive membership proof.
        if(proof.source==rw12::ActorAdded)file_log("RunicWheelLE: spell verified via Actor::addedSpells\r\n");
        else if(proof.source==rw12::NpcBase)file_log("RunicWheelLE: spell verified via TESNPC::spellList\r\n");
        else if(proof.source==rw12::Race)file_log("RunicWheelLE: spell verified via TESRace::spellList\r\n");
        // Previous 0.11.1 called EquipItem with a SpellItem; in-game this left
        // magic animations/controls inconsistent. The vanilla MagicMenu calls
        // EquipManager::EquipSpell, NOT EquipManager::EquipItem, for SpellItem.
        // Do not replace with a generic EquipItem call (CommonLibLE F36).
        reinterpret_cast<GameEquipSpell>(0x006EFAC0)(manager,actor,form,slot);
        action_diagnostic("RunicWheelLE: SPELL native EquipSpell request submitted (result needs game validation)\r\n",3);return;
    }
    const unsigned formType=*((const unsigned char*)form+0x12);
    if(kind!=rw05::Weapon && kind!=rw05::Armor && kind!=rw05::Consumable && kind!=rw05::Ammo){
        log_item_refusal("unsupported form type",request.binding.formId,formType);
        visual_feedback(4);return;
    }
    // Ingredient (INGR 0x1E) is displayable but NOT an AlchemyItem/Potion
    // (0x2E). A vanilla ingredient-eating route has not yet been proven. Never
    // pass it to generic EquipItem: that caused the 0.11.1 spell regression.
    if(formType==0x1E){
        log_item_refusal("ingredient eat native route not verified (no action)",request.binding.formId,formType);
        visual_feedback(8);return;
    }
    if(!owned_inventory(actor,form,owned)){
        log_item_refusal("ownership unproven after base+delta check",request.binding.formId,formType);
        visual_feedback(4);return;
    }
    rw11::Facts facts{};facts.focused=true;facts.inGame=true;facts.actorReady=true;
    facts.owned=true;facts.ambiguous=owned.ambiguous;facts.actualKind=kind;
    facts.availableCount=owned.count;
    facts.worn=(owned.wornRight!=nullptr||owned.wornLeft!=nullptr);
    if(rw11::decide(request.binding,facts)==rw11::Refuse){
        action_diagnostic("RunicWheelLE: inventory or variant validation rejected operation\r\n",4);return;
    }
    if(kind==rw05::Weapon){
        Raw process=*(Raw*)((unsigned char*)actor+0x88);
        if(!readable(process,0x70,0)){
          action_diagnostic("RunicWheelLE: equipped weapon state not available\r\n",4);return;
        }
        Raw equipped=*((Raw*)((unsigned char*)process+0x68)+ (request.hand==0?0:1));
        Raw worn=request.hand==0?owned.wornLeft:owned.wornRight;
        if(equipped==form){
            if(!worn){action_diagnostic("RunicWheelLE: exact worn instance missing\r\n",4);return;}
            reinterpret_cast<GameUnequip>(0x006EE560)(manager,actor,form,worn,1,slot,true,false,true,false,nullptr);
            action_diagnostic("RunicWheelLE: WEAPON unequip request submitted\r\n",6);
        }else{
            reinterpret_cast<GameEquip>(0x006EF3E0)(manager,actor,form,nullptr,1,slot,true,false,false,nullptr);
            action_diagnostic("RunicWheelLE: WEAPON equip request submitted\r\n",3);
        }
        return;
    }
    if(kind==rw05::Armor){
        Raw worn=owned.wornRight?owned.wornRight:owned.wornLeft;
        if(worn){
          reinterpret_cast<GameUnequip>(0x006EE560)(manager,actor,form,worn,1,nullptr,true,false,true,false,nullptr);
          action_diagnostic("RunicWheelLE: ARMOR unequip request submitted\r\n",6);
        }else{
          reinterpret_cast<GameEquip>(0x006EF3E0)(manager,actor,form,nullptr,1,nullptr,true,false,false,nullptr);
          action_diagnostic("RunicWheelLE: ARMOR equip request submitted\r\n",3);
        }
        return;
    }
    // Standard SKSE EquipManager route for native consumables/ammunition.
    reinterpret_cast<GameEquip>(0x006EF3E0)(manager,actor,form,nullptr,
         kind==rw05::Ammo?owned.count:1,nullptr,true,false,false,nullptr);
    action_diagnostic("RunicWheelLE: consumable/ammo request submitted (verify effect in game)\r\n",5);
}
int native_action_fault(){
    InterlockedExchange(&g_actionFaulted,1);
    action_diagnostic("RunicWheelLE: native equip SEH; further native actions DISABLED\r\n",4);
    return 1;
}
struct NativeTask {Raw* methods;};
void __fastcall run_native_task(NativeTask*,void*){
    if(InterlockedCompareExchange(&g_actionFaulted,0,0))return;
    const ActionRequest request=g_actionRequest;
    __try {perform_action(request);}__except(native_action_fault()){}
}
void __fastcall dispose_native_task(NativeTask*,void*){
    InterlockedExchange(&g_actionQueued,0);
}
static Raw g_nativeTaskVtable[2]={(Raw)&run_native_task,(Raw)&dispose_native_task};
static NativeTask g_nativeTask{g_nativeTaskVtable};
// Cache missing type labels from a loaded v0.10 co-save on the game task thread.
// Only reads TESForm.formType; does not modify any game inventory or engine object.
struct TypeRefreshTask {Raw* methods;};
// TESForm_GetDisplayName dispatch from the SKSE LE FavoritesMenu adapter:
// skse/Hooks_Scaleform.cpp uses __cdecl GetName(TESForm*) at 0x00452300.
// No vtable guess or renderer-thread TESForm dereference. This route is a
// newly introduced optional/read-only UI feature, not a change to equipment.
int name_lookup_signature_ok(){
    static const unsigned char bytes[]={0x8b,0x44,0x24,0x04,0x6a,0x00};
    if(!readable((Raw)0x00452300,sizeof(bytes),1))return 0;
    for(unsigned i=0;i<sizeof(bytes);++i)if(((const unsigned char*)0x00452300)[i]!=bytes[i])return 0;
    return 1;
}
int cache_names_fault(){
    InterlockedExchange(&g_namesEnabled,0);
    file_log("RunicWheelLE: SEH in optional display-name reader; names disabled for this session\r\n");
    return 1;
}
void fill_names_on_game_thread(){
    if(!native_sites_verified())return;
    const int canRead=g_showNames && InterlockedCompareExchange(&g_namesEnabled,0,0) && name_lookup_signature_ok();
    int resolved=0;
    AcquireSRWLockExclusive(&g_catalogLock);
    __try {
        for(unsigned w=0;w<rw05::Wheels;++w)for(unsigned sector=0;sector<rw05::Sectors;++sector){
            rw05::Slot& slot=g_model.slot[w][sector];
            for(unsigned i=0;i<rw05::Capacity;++i){
                char* name=g_displayNames[w][sector][i];
                name[0]=0; // Never display names of removed or replaced bindings.
                if(i>=slot.count)continue;
                rw05::Binding& b=slot.item[i];
                if(!b.formId)continue;
                Raw form=reinterpret_cast<Raw (__cdecl*)(DWord)>(0x00451A30)(b.formId);
                if(!readable(form,0x14,0)||*(DWord*)((unsigned char*)form+0x0C)!=b.formId)continue;
                if(!b.kind)b.kind=(unsigned char)form_kind(*(const unsigned char*)((unsigned char*)form+0x12));
                if(canRead){
                    const char* rawName=reinterpret_cast<const char* (__cdecl*)(Raw)>(0x00452300)(form);
                    if(readable(rawName,1,0)){
                        rw12ui::normalise_name(rawName,name);
                        if(name[0])++resolved;
                    }
                }
            }
        }
        InterlockedExchange(&g_refreshNeeded,0);
        InterlockedExchange(&g_namesResolved,resolved);
    }__finally {ReleaseSRWLockExclusive(&g_catalogLock);}
}
void __fastcall run_type_refresh(TypeRefreshTask*,void*){
    __try {
        fill_names_on_game_thread();
        if(InterlockedCompareExchange(&g_namesReported,1,0)==0){
            if(!g_showNames)file_log("RunicWheelLE: optional display names disabled by INI\r\n");
            else if(!g_namesEnabled)file_log("RunicWheelLE: name engine signature unavailable; fallback category symbols\r\n");
            else if(g_namesResolved)file_log("RunicWheelLE: game-thread display-name cache READY\r\n");
            else file_log("RunicWheelLE: no display names currently resolved; fallback category symbols\r\n");
        }
    }__except(cache_names_fault()){}
}
void __fastcall dispose_type_refresh(TypeRefreshTask*,void*){
    InterlockedExchange(&g_refreshQueued,0);
}
static Raw g_typeRefreshVtable[2]={(Raw)&run_type_refresh,(Raw)&dispose_type_refresh};
static TypeRefreshTask g_typeRefreshTask{g_typeRefreshVtable};
void schedule_type_refresh(){
    if(!g_tasks || !g_tasks->AddTask || !g_visible ||
       !InterlockedCompareExchange(&g_refreshNeeded,0,0) ||
       InterlockedCompareExchange(&g_pendingApply,0,0))return;
    if(InterlockedCompareExchange(&g_refreshQueued,1,0)!=0)return;
    g_tasks->AddTask((TaskDelegate*)&g_typeRefreshTask);
}

void log_action(const char* hand){
    rw05::Binding binding{};
    AcquireSRWLockShared(&g_catalogLock);
    __try {rw05::State snapshot{};snapshot_state_locked(snapshot);
           binding=rw05::current(snapshot,snapshot.activeWheel,(unsigned)rw02::clamp((int)g_select,0,7));}
    __finally {ReleaseSRWLockShared(&g_catalogLock);}
    if(!binding.formId){action_diagnostic("RunicWheelLE: empty sector (no action)\r\n",4);return;}
    if(!g_tasks || !g_tasks->AddTask || InterlockedCompareExchange(&g_actionQueued,1,0)!=0){
        action_diagnostic("RunicWheelLE: native game task unavailable/busy\r\n",4);return;
    }
    g_actionRequest.binding=binding;
    g_actionRequest.hand=hand[0]=='L'?0:1;
    g_actionRequest.generation=(unsigned)InterlockedCompareExchange(&g_actionGeneration,0,0);
    g_tasks->AddTask((TaskDelegate*)&g_nativeTask);
    file_log("RunicWheelLE: native game task queued\r\n");
    visual_feedback(7);
}
void publish_state_locked(const rw05::State& state);
void load_config(){
    g_cfg.openMouse=value("OpenMouseButton",2,0,4);
    g_cfg.keyboardOpen=value("OpenKeyboardScanCode",0,0,255);
    // Never commandeer F2/F3/F4/F5 (especially Skyrim quicksave F5),
    // even if an obsolete user INI attempts to bind the wheel to one.
    if(g_cfg.keyboardOpen>=0x3C && g_cfg.keyboardOpen<=0x3F){
        g_cfg.keyboardOpen=0;
        file_log("RunicWheelLE: reserved function key OPEN binding ignored (F2-F5)\r\n");
    }
    g_cfg.previousKey=value("PreviousWheelScanCode",16,0,255);
    if(g_cfg.previousKey>=0x3C && g_cfg.previousKey<=0x3F)g_cfg.previousKey=16;
    g_cfg.nextKey=value("NextWheelScanCode",18,0,255);
    if(g_cfg.nextKey>=0x3C && g_cfg.nextKey<=0x3F)g_cfg.nextKey=18;
    g_cfg.keyboardMode=value("KeyboardLayoutMode",2,0,2); // 1 physical; 2 logical Q/E via active Windows layout
    g_cfg.prevMouse=value("PreviousWheelMouseButton",3,0,4);
    g_cfg.nextMouse=value("NextWheelMouseButton",4,0,4);
    g_cfg.holdMs=value("HoldThresholdMs",260,80,1200);
    g_cfg.sensitivity=value("MouseSensitivityPercent",100,20,350);
    g_cfg.deadzone=value("MouseDeadzone",22,0,120);
    g_cfg.suppress=value("ConsumeInputWhileOpen",1,0,1);
    g_cfg.gamepad=value("EnableGamepad",1,0,1);
    g_cfg.padOpen=value("GamepadOpenMask",9,0,65535);
    g_cfg.padPrev=value("GamepadPreviousMask",0,0,65535);
    g_cfg.padNext=value("GamepadNextMask",10,0,65535);
    g_cfg.padStick=value("GamepadRightStickMask",12,0,65535);
    g_cfg.primaryMouse=value("PrimaryMouseButton",0,0,255);
    g_cfg.secondaryMouse=value("SecondaryMouseButton",1,0,255);
    g_cfg.wheelUp=value("CycleEntryUpMouseCode",8,0,255);
    g_cfg.wheelDown=value("CycleEntryDownMouseCode",9,0,255);
    g_cfg.closeKey=value("CloseKeyboardScanCode",1,0,255);
    if(g_cfg.closeKey>=0x3C && g_cfg.closeKey<=0x3F){
        g_cfg.closeKey=1;
        file_log("RunicWheelLE: reserved function key CLOSE binding ignored (F2-F5)\r\n");
    }
    g_cfg.padPrimary=value("GamepadPrimaryMask",512,0,65535);
    g_cfg.padSecondary=value("GamepadSecondaryMask",256,0,65535);
    g_cfg.padDeadzone=value("GamepadDeadzonePercent",22,0,90)*2;
    g_cfg.padItemPrevious=value("GamepadPreviousItemMask",4,0,65535);
    g_cfg.padItemNext=value("GamepadNextItemMask",8,0,65535);
    g_cfg.wheelScale=value("WheelScalePercent",130,75,170);
    g_cfg.nativeActions=value("EnableNativeActions",1,0,1);
    g_enableSpellActions=value("EnableSpellActions",1,0,1);
    g_cfg.wheelOpacity=value("WheelOpacityPercent",70,25,100);
    g_showNames=value("ShowItemNames",1,0,1);
    g_cfg.feedbackDuration=value("ClickFeedbackMs",900,250,2000);
    g_slowPercent=value("SlowTimePercent",10,1,100);
    // Speed is a supported gameplay option, independent of the experimental Scaleform hooks.
    g_enableSlowTime=(g_slowPercent<100)?1:0;
    // Read-only UI task uses SKSE's supported UI queue, not a vtable detour.
    // Explicit feature switch for game-side isolation; no legacy F-key editor.
    g_hoverCaptureEnabled=value("EnableSafeInventoryUITask",1,0,1);
    if(!g_cfg.nativeActions)file_log("RunicWheelLE: native equipment explicitly disabled by INI\r\n");
    if(!g_enableSpellActions)file_log("RunicWheelLE: native spell actions explicitly disabled by INI\r\n");
    if(!g_hoverCaptureEnabled)file_log("RunicWheelLE: UI task disabled by config\r\n");
    if(!g_enableSlowTime)file_log("RunicWheelLE: slow-time disabled by 100 percent speed setting\r\n");
    load_catalog();
    rw04::State legacy{};
    legacy.catalog=g_catalog;
    g_model=rw05::migrate(legacy);
    // Typed V2 overrides are optional; missing keys retain legacy FormID-only entries.
    // Read exactly as user-authored, never invent a variant hash from a FormID.
    for(int w=0;w<rw03::Wheels;++w)for(int sec=0;sec<rw03::Slots;++sec){
        char key[]="Wheel1Slot1";key[5]=char('1'+w);key[10]=char('1'+sec);
        char line[180]{};
        GetPrivateProfileStringA("BindingsV2",key,"~",line,sizeof(line),g_configPath);
        if(line[0]!='~' || line[1]){
            rw05::Slot typed{};
            int rejected=rw05::parse_slot(line,typed);
            // An invalid override cannot silently erase an otherwise valid legacy slot.
            if(!rejected)g_model.slot[w][sec]=typed;
            else file_log("RunicWheelLE: invalid BindingsV2 override ignored\r\n");
        }
    }
    publish_state_locked(g_model);
    file_log("RunicWheelLE: deprecated editor hotkeys removed; only automatic menu context\r\n");
    g_defaultState=g_model;
    select_current();
}
// The canonical V2 state is independent from the D3D9 legacy read-only projection.
void snapshot_state_locked(rw05::State& state){
    state=g_model;
    state.activeWheel=(unsigned char)rw02::clamp((int)g_activeWheel,0,2);
    for(int w=0;w<rw03::Wheels;++w)for(int sec=0;sec<rw03::Slots;++sec)
        state.cursor[w][sec]=(unsigned char)rw02::clamp((int)g_cursors[w][sec],0,3);
}
void publish_state_locked(const rw05::State& state){
    g_model=state;
    g_catalog={};
    for(int w=0;w<rw03::Wheels;++w)for(int sec=0;sec<rw03::Slots;++sec){
        const rw05::Slot& slot=state.slot[w][sec];
        rw03::Slot& projected=g_catalog.slots[w][sec];
        projected.count=slot.count;
        for(unsigned i=0;i<slot.count;++i)projected.form[i]=slot.item[i].formId;
        InterlockedExchange(&g_cursors[w][sec],state.cursor[w][sec]);
    }
    InterlockedExchange(&g_activeWheel,state.activeWheel);
    // A model change invalidates every presentation name; no stale item labels.
    for(unsigned w=0;w<rw05::Wheels;++w)for(unsigned sec=0;sec<rw05::Sectors;++sec)
      for(unsigned i=0;i<rw05::Capacity;++i)g_displayNames[w][sec][i][0]=0;
    InterlockedExchange(&g_refreshNeeded,1);
}
void queue_state(const rw05::State& state){
    AcquireSRWLockExclusive(&g_catalogLock);
    __try {g_queuedState=state;InterlockedExchange(&g_pendingApply,1);}
    __finally {ReleaseSRWLockExclusive(&g_catalogLock);}
    InterlockedExchange(&g_resetInput,1);
}
void apply_pending_state(){
    if(!InterlockedExchange(&g_pendingApply,0))return;
    AcquireSRWLockExclusive(&g_catalogLock);
    __try {publish_state_locked(g_queuedState);} __finally {ReleaseSRWLockExclusive(&g_catalogLock);}
    g_editor.undoCount=0;g_editor.redoCount=0;
    select_current();
    InterlockedExchange(&g_refreshNeeded,1);
}
// In-game editor intentionally fails closed until hovered-object identity is verified.

void edit_click(bool remove){
    if(!inventory_context())return;
    // Never consume an old hovered item from another menu or expired frame.
    const DWord seen=(DWord)InterlockedCompareExchange(&g_uiSeen,0,0);
    const DWord now=GetTickCount();
    const DWord form=(!remove && seen && (DWord)(now-seen)<=500u)?
        (DWord)InterlockedCompareExchange(&g_uiHover,0,0):0;
    if(!remove && !form){visual_feedback(4);file_log("RunicWheelLE: no VERIFIED hovered FormID; add refused\r\n");return;}
    rw05::Binding b{};
    b.formId=form; b.kind=(unsigned char)InterlockedCompareExchange(&g_uiKind,0,0); b.hand=rw05::Automatic;
    const unsigned wheel=(unsigned)rw02::clamp((int)g_activeWheel,0,2);
    const unsigned sector=(unsigned)rw02::clamp((int)g_select,0,7);
    bool changed=false;
    AcquireSRWLockExclusive(&g_catalogLock);
    __try {
        rw05::State state{};snapshot_state_locked(state);
        if(remove)changed=rw05::erase(g_editor,state,wheel,sector,(int)state.cursor[wheel][sector]);
        else if(rw05::valid_binding(b)){
            changed=rw05::insert(g_editor,state,wheel,sector,b);
            if(changed)state.cursor[wheel][sector]=state.slot[wheel][sector].count-1;
        }
        if(changed)publish_state_locked(state);
    }__finally{ReleaseSRWLockExclusive(&g_catalogLock);}
    if(changed){select_current();visual_feedback(remove?2:1);file_log(remove?
        "RunicWheelLE: EDIT remove from selected slot\r\n":
        "RunicWheelLE: EDIT add hovered item to selected slot\r\n");}
    else file_log("RunicWheelLE: edit refused (no item, duplicate or sector full)\r\n");
}

static bool resolve_saved_form(unsigned saved,unsigned* out,void* context){
    auto* serial=(SKSESerializationInterface*)context;
    if(!serial || !serial->ResolveFormId || !out)return false;
    UInt32 current=0;
    bool ok=serial->ResolveFormId((UInt32)saved,&current);
    *out=(unsigned)current;return ok;
}
void save_impl(SKSESerializationInterface* serial){
    if(!serial||!serial->WriteRecord)return;
    rw05::State snapshot{};
    AcquireSRWLockShared(&g_catalogLock);
    __try {
        if(g_pendingApply)snapshot=g_queuedState;
        else snapshot_state_locked(snapshot);
    } __finally {ReleaseSRWLockShared(&g_catalogLock);}
    unsigned char bytes[rw05::PacketBytes]{};rw05::encode(snapshot,bytes);
    if(!serial->WriteRecord(kSaveRecord,kSaveVersion,bytes,rw05::PacketBytes))
        file_log("RunicWheelLE: SKSE co-save write FAILED\r\n");
    else file_log("RunicWheelLE: SKSE co-save record WRITTEN\r\n");
}
void load_impl(SKSESerializationInterface* serial){
    if(!serial||!serial->GetNextRecordInfo||!serial->ReadRecordData)return;
    UInt32 record=0,version=0,length=0;
    while(serial->GetNextRecordInfo(&record,&version,&length)){
        if(record!=kSaveRecord || (version!=1 && version!=kSaveVersion) ||
           (version==1 && length!=rw04::kBytes) || (version==2 && length!=rw05::PacketBytes))continue;
        unsigned char data[rw05::PacketBytes]{};
        if(serial->ReadRecordData(data,length)!=length)continue;
        rw05::State next{};unsigned dropped=0;
        if(!rw05::decode_any(data,length,next,resolve_saved_form,serial,&dropped)){
            file_log("RunicWheelLE: invalid co-save record ignored\r\n");continue;
        }
        queue_state(next);
        if(dropped)file_log("RunicWheelLE: unavailable/duplicate FormIDs removed on load\r\n");
        else file_log("RunicWheelLE: per-save wheel state loaded\r\n");
    }
}
void revert_impl(SKSESerializationInterface*){queue_state(g_defaultState);file_log("RunicWheelLE: SKSE co-save REVERT requested\r\n");}
int serialization_fault(){
    file_log("RunicWheelLE: SEH in SKSE co-save callback; record ignored\r\n");
    return 1;
}
void on_save(SKSESerializationInterface* serial){
    __try {save_impl(serial);} __except(serialization_fault()){}
}
void on_load(SKSESerializationInterface* serial){
    __try {load_impl(serial);} __except(serialization_fault()){}
}
void on_revert(SKSESerializationInterface* serial){
    __try {revert_impl(serial);} __except(serialization_fault()){}
}
void reset_input(){rw02::reset(g_activation);g_mouseX=g_mouseY=0;InterlockedExchange(&g_visible,0);select_current();}
void change_visible(bool visible){
    if((InterlockedExchange(&g_visible,visible?1:0)!=0)!=visible){
        file_log(visible?"RunicWheelLE: input -> wheel OPEN\r\n":"RunicWheelLE: input -> wheel CLOSED\r\n");
    }
}
void activate(bool down){
    bool was=g_activation.visible;
    bool now=rw02::button(g_activation,down,GetTickCount(),(unsigned)g_cfg.holdMs);
    if(!was && now){g_mouseX=g_mouseY=0;InterlockedExchange(&g_renderLogged,0);}
    if(was!=now)change_visible(now);
    if(!was && now)InterlockedExchange(&g_refreshNeeded,1);
}
void close_wheel(){reset_input(); file_log("RunicWheelLE: input -> wheel CANCELLED\r\n");}
void move_pointer(Long dx,Long dy){
    dx=rw02::clamp((int)dx,-200,200);dy=rw02::clamp((int)dy,-200,200);
    g_mouseX=rw02::clamp((int)(g_mouseX+dx*g_cfg.sensitivity/100),-240,240);
    g_mouseY=rw02::clamp((int)(g_mouseY+dy*g_cfg.sensitivity/100),-240,240);
    int sector=rw02::sector((int)g_mouseX,(int)g_mouseY,g_cfg.deadzone);
    if(sector>=0){InterlockedExchange(&g_select,sector);select_current();}
}
int layout_scancode(char c,int fallback){
    if(g_cfg.keyboardMode!=2)return fallback;
    Handle layout=GetKeyboardLayout(0);
    short info=VkKeyScanExA(c,layout);
    if(info==-1)return fallback;
    unsigned sc=MapVirtualKeyExA((unsigned)(info&255),0,layout); // VKEY to physical scan code
    return sc&&sc<=255?(int)sc:fallback;
}
// Return true when an event must not reach vanilla gameplay.
int process_event(InputEvent* event){
    if(!event)return 0;
    if(GetForegroundWindow()!=g_window){
        if(g_activation.visible) reset_input();
        return 0;
    }
    const bool visibleBefore=g_activation.visible;
    if(event->type==0 && (event->device==0 || event->device==1 || event->device==2)){
        const ButtonEvent* b=(ButtonEvent*)event;
        const bool down=b->value!=0.0f;
        const bool mouse=event->device==1;
        const bool key=event->device==0;
        const bool pad=event->device==2 && g_cfg.gamepad;
        // LE physical scancodes 0x3C..0x3F are F2/F3/F4/F5. Pass through
        // unchanged; no hidden edit toggle, undo or save interception.
        if(key && b->scan>=0x3C && b->scan<=0x3F)return 0;
        if((mouse && b->scan==(DWord)g_cfg.openMouse) || (key && g_cfg.keyboardOpen && b->scan==(DWord)g_cfg.keyboardOpen) || (pad && b->scan==(DWord)g_cfg.padOpen)){
            activate(down);return 1; // never trigger the game's middle-button action
        }
        // No F2/F5 editor modes. Edit only when Inventory/Magic menu is actually open.
        if(g_activation.visible && down && b->duration==0.0f){
            if(mouse && b->scan==(DWord)g_cfg.primaryMouse){g_actionFlash=1;if(inventory_context())edit_click(false);else log_action("LEFT");}
            if(mouse && b->scan==(DWord)g_cfg.secondaryMouse){g_actionFlash=2;if(inventory_context())edit_click(true);else log_action("RIGHT");}
            if(pad && b->scan==(DWord)g_cfg.padPrimary){g_actionFlash=1;if(inventory_context())edit_click(false);else log_action("LEFT");}
            if(pad && b->scan==(DWord)g_cfg.padSecondary){g_actionFlash=2;if(inventory_context())edit_click(true);else log_action("RIGHT");}
            if(pad && ((g_cfg.padItemPrevious && b->scan==(DWord)g_cfg.padItemPrevious) || (g_cfg.padItemNext && b->scan==(DWord)g_cfg.padItemNext))){
                cycle_entry(b->scan==(DWord)g_cfg.padItemPrevious?-1:1);
            }
            if(mouse && (b->scan==(DWord)g_cfg.wheelUp || b->scan==(DWord)g_cfg.wheelDown)){
                int direction=b->scan==(DWord)g_cfg.wheelUp?-1:1;
                cycle_entry(direction);
            }
            if((mouse && b->scan==(DWord)g_cfg.prevMouse) || (key && b->scan==(DWord)layout_scancode('Q',g_cfg.previousKey)) || (pad && g_cfg.padPrev && b->scan==(DWord)g_cfg.padPrev)){InterlockedExchange(&g_activeWheel,rw02::step((int)g_activeWheel,-1,3));select_current();}
            if((mouse && b->scan==(DWord)g_cfg.nextMouse) || (key && b->scan==(DWord)layout_scancode('E',g_cfg.nextKey)) || (pad && g_cfg.padNext && b->scan==(DWord)g_cfg.padNext)){InterlockedExchange(&g_activeWheel,rw02::step((int)g_activeWheel,+1,3));select_current();}
        }
        if(key && b->scan==(DWord)g_cfg.closeKey && down && g_activation.visible){close_wheel();return 1;}
        if(g_cfg.suppress && (visibleBefore||g_activation.visible)) {
           if(mouse && (b->scan==(DWord)g_cfg.primaryMouse||b->scan==(DWord)g_cfg.secondaryMouse||b->scan==(DWord)g_cfg.wheelUp||b->scan==(DWord)g_cfg.wheelDown||b->scan==(DWord)g_cfg.prevMouse||b->scan==(DWord)g_cfg.nextMouse))return 1;
           if(key && (b->scan==(DWord)layout_scancode('Q',g_cfg.previousKey)||b->scan==(DWord)layout_scancode('E',g_cfg.nextKey)))return 1;
           if(pad && ((g_cfg.padPrev && b->scan==(DWord)g_cfg.padPrev)||(g_cfg.padNext && b->scan==(DWord)g_cfg.padNext)||(g_cfg.padItemPrevious && b->scan==(DWord)g_cfg.padItemPrevious)||(g_cfg.padItemNext && b->scan==(DWord)g_cfg.padItemNext)||b->scan==(DWord)g_cfg.padPrimary||b->scan==(DWord)g_cfg.padSecondary))return 1;
        }
    }
    if(event->device==1 && event->type==1 && g_activation.visible){
        const MouseMove* m=(MouseMove*)event;
        move_pointer(m->dx,m->dy);
        return g_cfg.suppress;
    }
    if(event->device==2 && event->type==3 && g_activation.visible && g_cfg.gamepad){
        const StickEvent* t=(StickEvent*)event;
        if(t->mask==(DWord)g_cfg.padStick){
            int x=(int)(t->x*200), y=(int)(-t->y*200);
            int selected=rw02::sector(x,y,g_cfg.padDeadzone);
            if(selected>=0){InterlockedExchange(&g_select,selected);select_current();}
            return g_cfg.suppress;
        }
    }
    return 0;
}

void dispatch_input(void* dispatcher,InputEvent** head){
    if(InterlockedExchange(&g_resetInput,0))reset_input();
    apply_pending_state();
    queue_ui_sample();
    sync_time();
    if(!head||!g_previousInput){return;}
    if(!g_cfg.suppress){
        InputEvent* cur=*head;
        for(int n=0;cur&&n<128;++n){if(!readable(cur,sizeof(ButtonEvent),0))break;process_event(cur);cur=cur->next;}
        g_previousInput(dispatcher,head);queue_ui_sample();sync_time();return;
    }
    // Copy/relink, and always restore the original chains after downstream consumers.
    InputEvent* events[128];InputEvent* successors[128]; unsigned count=0;
    for(InputEvent* e=*head;e;e=e->next){
        if(count==128 || !readable(e,sizeof(ButtonEvent),0)){
            reset_input();g_previousInput(dispatcher,head);return;
        }
        events[count]=e;successors[count]=e->next;++count;
    }
    InputEvent* first=nullptr;InputEvent* last=nullptr;
    // Ensure relinked nodes are restored even if a downstream input sink faults.
    __try {
        for(unsigned i=0;i<count;++i){
            if(process_event(events[i]))continue;
            if(last)last->next=events[i];else first=events[i];
            last=events[i];
        }
        if(last)last->next=nullptr;
        InputEvent* empty=nullptr;
        queue_ui_sample();
        sync_time();
        g_previousInput(dispatcher,first?&first:&empty);
    } __finally {
        for(unsigned i=0;i<count;++i)events[i]->next=successors[i];
    }
}
int input_fault(){reset_input();file_log("RunicWheelLE: input exception; reset and bypass\r\n");return 1;}
void __fastcall input_hook(void* dispatcher,void*,InputEvent** head){
    if(!g_previousInput)return;
    __try {dispatch_input(dispatcher,head);}
    __except(input_fault()){} // previous sink must NEVER be called twice after a fault
}
int install_input_hook(){
    if(g_inputInstalled)return 1;
    unsigned char* site=(unsigned char*)v02sig::kInputCallsite;
    if(!readable(site,5,1)||site[0]!=0xE8)return 0; // chained E8 allowed, other prologue rejected
    Long oldDisp=*(Long*)(site+1);
    Raw destination=(Raw)((DWord)site+5+oldDisp);
    if(!readable(destination,1,1)||destination==(Raw)&input_hook)return 0;
    // Accept an already hooked E8 and chain through its current target (F7).
    bool original=true;
    for(unsigned i=0;i<5;++i)if(site[i]!=v02sig::kVanillaInputCall[i])original=false;
    if(!original)file_log("RunicWheelLE: preserving earlier input hook\r\n");
    // Publish our continuation before updating live executable bytes.
    g_previousInput=(SendEvents)destination;
    DWord old=0;
    if(!VirtualProtect(site,5,0x40,&old)){g_previousInput=nullptr;return 0;}
    // call site has one byte opcode E8 already. A live concurrent patch is not
    // atomic; install only via early SKSE InputLoaded lifecycle callback.
    DWord rel=(DWord)&input_hook-((DWord)site+5);
    *(DWord*)(site+1)=rel;
    DWord unused=0;
    VirtualProtect(site,5,old,&unused);
    FlushInstructionCache(GetCurrentProcess(),site,5);
    InterlockedExchange(&g_inputInstalled,1);
    file_log("RunicWheelLE: input dispatcher chained (LE callsite)\r\n");
    return 1;
}
constexpr float edgeX[9]={-.38268343f,.38268343f,.92387953f,.92387953f,.38268343f,-.38268343f,-.92387953f,-.92387953f,-.38268343f};
constexpr float edgeY[9]={-.92387953f,-.92387953f,-.38268343f,.38268343f,.92387953f,.92387953f,.38268343f,-.38268343f,-.92387953f};
DWord tinted(DWord col){
    // Opacity 70 is the design baseline. Increase/decrease alpha while keeping colors stable.
    const unsigned alpha=(col>>24)&255u;
    unsigned adjusted=(alpha*(unsigned)g_cfg.wheelOpacity)/70u;
    if(adjusted>255u)adjusted=255u;
    return (col&0x00FFFFFFu)|(adjusted<<24);
}
void vertex(Vertex& v,float x,float y,DWord color){v.x=x-0.5f;v.y=y-0.5f;v.z=0;v.rhw=1;v.color=color;}
void make_mesh(float w,float h){
    float d=w<h?w:h;float outer=d*0.31f*(float)g_cfg.wheelScale/100.f; if(outer>440.0f)outer=440.0f;
    float inner=outer*0.36f,cx=w*.5f,cy=h*.5f;
    int pos=0;
    for(int s=0;s<8;s++){
        int active=s==(int)g_select;
        const unsigned elapsed=(unsigned)GetTickCount()-(unsigned)InterlockedCompareExchange(&g_feedbackTick,0,0);
        const bool pulse=(elapsed<(unsigned)g_cfg.feedbackDuration && InterlockedCompareExchange(&g_feedback,0,0)!=0);
        DWord outerCol=active?(pulse?0xc0d8b568u:0xaab88d53u):(g_catalog.slots[rw02::clamp((int)g_activeWheel,0,2)][s].count?0x79354250u:0x6a18212bu);
        DWord innerCol=(inventory_context() && active)?0x9a339ca0u:(active?0xb08c744au:0x752c323du);
        // Original narrow gaps between the sectors, independent from Wheeler artwork.
        constexpr float margin=0.035f;
        const float x0=edgeX[s]*(1-margin)+edgeX[s+1]*margin;
        const float y0=edgeY[s]*(1-margin)+edgeY[s+1]*margin;
        const float x1=edgeX[s+1]*(1-margin)+edgeX[s]*margin;
        const float y1=edgeY[s+1]*(1-margin)+edgeY[s]*margin;
        Vertex a,b,c,d0;
        vertex(a,cx+x0*inner,cy+y0*inner,tinted(innerCol));
        vertex(b,cx+x0*outer,cy+y0*outer,tinted(outerCol));
        vertex(c,cx+x1*outer,cy+y1*outer,tinted(outerCol));
        vertex(d0,cx+x1*inner,cy+y1*inner,tinted(innerCol));
        g_geometry[pos++]=a;g_geometry[pos++]=b;g_geometry[pos++]=c;
        g_geometry[pos++]=a;g_geometry[pos++]=c;g_geometry[pos++]=d0;
    }
}
__declspec(noinline) int make_pips(float width,float height){
    int n=0;
    const int wheel=rw02::clamp((int)g_activeWheel,0,2);
    float diameter=width<height?width:height;
    float outer=diameter*0.31f*(float)g_cfg.wheelScale/100.f;if(outer>440.0f)outer=440.0f;
    float cx=width*.5f,cy=height*.5f;
    for(int sector=0;sector<8;++sector){
      int count=g_catalog.slots[wheel][sector].count;
      float ux=(edgeX[sector]+edgeX[sector+1])*.5411961f,uy=(edgeY[sector]+edgeY[sector+1])*.5411961f;
      for(int e=0;e<count;++e){
        float factor=.77f+.055f*e;
        float x=cx+ux*outer*factor, y=cy+uy*outer*factor;
        DWord color=e==(int)g_cursors[wheel][sector]?0xfff4df9cu:0xaa879aaa;
        vertex(g_pips[n++],x-3.6f,y+3.6f,color);
        vertex(g_pips[n++],x+3.6f,y+3.6f,color);
        vertex(g_pips[n++],x,y-3.6f,color);
      }
    }
    return n/3;
}

// 5x7 high contrast glyphs: clear type icons, slot count and current selection.
// Masks read most-significant column first (no font dependency or DX9 textures).
struct Glyph {char c; unsigned char row[7];};
static const Glyph kGlyphs[]={
 {'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}},{'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}},{'5',{31,16,30,1,1,17,14}},
 {'6',{6,8,16,30,17,17,14}},{'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}},{'9',{14,17,17,15,1,2,12}},
 {'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},
 {'C',{15,16,16,16,16,16,15}},{'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}},{'F',{31,16,16,30,16,16,16}},
 {'G',{15,16,16,23,17,17,15}},{'H',{17,17,17,31,17,17,17}},
 {'I',{14,4,4,4,4,4,14}},{'J',{1,1,1,1,17,17,14}},
 {'K',{17,18,20,24,20,18,17}},{'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}},{'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}},{'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}},{'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}},{'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}},{'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}},{'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}},{'Z',{31,1,2,4,8,16,31}},
 {'-',{0,0,0,31,0,0,0}},{':',{0,4,4,0,4,4,0}},
 {'?',{14,17,1,2,4,0,4}},{' ',{0,0,0,0,0,0,0}}
};
static int g_labelVert=0;
void mark_quad(float x,float y,float width,float height,DWord col){
    if(g_labelVert+6>30000)return;
    Vertex& a=g_labels[g_labelVert++];vertex(a,x,y,col);
    Vertex& b=g_labels[g_labelVert++];vertex(b,x+width,y,col);
    Vertex& c=g_labels[g_labelVert++];vertex(c,x+width,y+height,col);
    Vertex& d=g_labels[g_labelVert++];vertex(d,x,y,col);
    Vertex& e=g_labels[g_labelVert++];vertex(e,x+width,y+height,col);
    Vertex& f=g_labels[g_labelVert++];vertex(f,x,y+height,col);
}
void draw_text(float x,float y,float unit,const char* word,DWord col){
    if(!word)return;
    for(unsigned p=0;word[p] && p<22;++p){
        const char symbol=word[p];
        const Glyph* glyph=nullptr;
        for(const Glyph& g:kGlyphs)if(g.c==symbol){glyph=&g;break;}
        if(!glyph){x+=6.f*unit;continue;}
        for(int r=0;r<7;++r)for(int c=0;c<5;++c)
            if(glyph->row[r]&(1u<<(4-c)))mark_quad(x+c*unit,y+r*unit,unit*.84f,unit*.84f,col);
        x+=6.f*unit;
    }
}
// Original pixel pictograms rendered as geometry (no textures or third-party assets).
// Each symbol is a fixed 9x9 mask; these are categories, not individual item art.
static const char* const kWeaponMask[9]={
    ".......XX","......XX.",".....XX..","....XX...","...XX....",
    "..XX.....",".XXX.....",".X..X....","X....X..."};
static const char* const kArmorMask[9]={
    "..XXXXX..",".XXXXXXX.","XXX.X.XXX","XXXXXXXXX","XXXXXXXXX",
    ".XXXXXXX.",".XXXXXXX.","..XXXXX..","..XXXXX.."};
static const char* const kSpellMask[9]={
    "....X....","...XXX...","..XX.XX..",".XX.X.XX.",".X.XXX.X.",
    ".XX.X.XX.","..XXXXX..","...XXX...","....X...."};
static const char* const kPotionMask[9]={
    "...XXX...","...XXX...","....X....","..XXXXX..",".XXXXXXX.",
    ".XXXXXXX.",".XXXXXXX.","..XXXXX..","...XXX..."};
static const char* const kAmmoMask[9]={
    "....XX...","....XXX..","....X.XX.","....X..XX","....X....",
    "..XXXXX..","...XXX...","....X....","...X.X..."};
static const char* const kShoutMask[9]={
    "...XXX...","..X...X..",".X.....X.","XX..X..XX","XX.XXX.XX",
    "XX..X..XX",".X.....X.","..X...X..","...XXX..."};
void draw_category_icon(float centerX,float centerY,rw05::Kind kind,DWord color){
    const char* const* pattern=kind==rw05::Weapon?kWeaponMask:
        kind==rw05::Armor?kArmorMask:kind==rw05::Spell?kSpellMask:
        kind==rw05::Consumable?kPotionMask:kind==rw05::Ammo?kAmmoMask:
        kind==rw05::Shout?kShoutMask:nullptr;
    if(!pattern){draw_text(centerX-8.f,centerY-12.f,3.4f,"?",color);return;}
    constexpr float pixel=4.8f;
    for(unsigned r=0;r<9;++r)for(unsigned c=0;c<9;++c){
        if(pattern[r][c]=='X')mark_quad(centerX-21.6f+(float)c*pixel,
            centerY-21.6f+(float)r*pixel,pixel*.90f,pixel*.90f,color);
    }
}
void draw_labels(float width,float height){
    g_labelVert=0;
    const float d=width<height?width:height;
    float outer=d*.31f*(float)g_cfg.wheelScale/100.f;
    if(outer>440.f)outer=440.f;
    const float cx=width*.5f,cy=height*.5f;
    const int wheel=rw02::clamp((int)g_activeWheel,0,2), selected=rw02::clamp((int)g_select,0,7);
    for(int sector=0;sector<8;++sector){
        const float ux=(edgeX[sector]+edgeX[sector+1])*.5411961f;
        const float uy=(edgeY[sector]+edgeY[sector+1])*.5411961f;
        const rw05::Slot& slot=g_model.slot[wheel][sector];
        if(!slot.count || slot.count>rw05::Capacity)continue;
        const unsigned index=g_model.cursor[wheel][sector]<slot.count?g_model.cursor[wheel][sector]:0;
        const rw05::Binding& b=slot.item[index];
        const float x=cx+ux*outer*.66f,y=cy+uy*outer*.66f;
        const bool active=selected==sector;
        const DWord accent=b.kind==rw05::Weapon?0xffffd69au:
          b.kind==rw05::Armor?0xffc0e0ffu:b.kind==rw05::Spell?0xffc9b8ffu:
          b.kind==rw05::Consumable?0xff98edc7u:b.kind==rw05::Ammo?0xffffe4a4u:0xffe2cee8u;
        // 13x13 emblem with dark drop shadow and a fine engraved edge.
        mark_quad(x-35.f,y-34.f,70.f,67.f,tinted(active?0xc07f664au:0x96404e61u));
        mark_quad(x-32.f,y-31.f,64.f,61.f,tinted(0x9b142333u));
        mark_quad(x-32.f,y+27.f,64.f,3.f,tinted(accent));
        draw_category_icon(x+1.f,y-5.f,(rw05::Kind)b.kind,0x9c020b18u);
        draw_category_icon(x,y-6.f,(rw05::Kind)b.kind,accent);
        if(slot.count>1){char qty[2]={char('0'+slot.count),0};
          mark_quad(x+23.f,y-31.f,15.f,19.f,tinted(0xdf203044u));
          draw_text(x+25.f,y-29.f,1.85f,qty,0xfff9e5aeu);}
        // Read only a *copy* created by the game-thread cache, never TESV objects.
        const char* label=g_displayNames[wheel][sector][index];
        if(g_showNames && label[0]){
            const unsigned letters=rw12ui::length_bounded(label,10);
            char shortName[11]{};
            for(unsigned k=0;k<letters;++k)shortName[k]=label[k];
            const float unit=1.82f;
            const float left=x-(float)letters*3.f*unit;
            mark_quad(left-5.f,y+34.f,(float)letters*6.f*unit+9.f,17.f,tinted(0xc0101e2au));
            draw_text(left,y+36.f,unit,shortName,0xfffff5e4u);
        }
    }
    const rw05::Binding selection=rw05::current(g_model,(unsigned)wheel,(unsigned)selected);
    if(selection.formId){
        draw_text(cx-52.f,cy-35.f,3.1f,"SELECTED",0xffebdec2u);
        const rw05::Slot& picked=g_model.slot[wheel][selected];
        unsigned cursor=g_model.cursor[wheel][selected];
        const char* label=cursor<picked.count?g_displayNames[wheel][selected][cursor]:"";
        if(g_showNames && label[0]){
            // Wide translucent caption below the ring keeps the centre uncluttered.
            const float y=cy+outer*.78f+14.f;
            mark_quad(cx-226.f,y-8.f,452.f,47.f,tinted(0xb0192535u));
            unsigned letters=rw12ui::length_bounded(label,21);
            float unit=2.75f;
            draw_text(cx-(float)letters*3.f*unit,y+4.f,unit,label,0xfffcf0d9u);
        }else{
            char hex[]="00000000";const char* digits="0123456789ABCDEF";
            for(int i=0;i<8;++i)hex[i]=digits[(selection.formId>>(28-4*i))&15u];
            draw_text(cx-51.f,cy+3.f,2.3f,hex,0xffd4ebf8u);
        }
    }else draw_text(cx-42.f,cy-9.f,3.3f,"EMPTY",0xffa6b8c8u);
    const unsigned elapsed=(unsigned)GetTickCount()-(unsigned)InterlockedCompareExchange(&g_feedbackTick,0,0);
    if(elapsed<(unsigned)g_cfg.feedbackDuration){
        const int code=(int)InterlockedCompareExchange(&g_feedback,0,0);
        const char* msg=code==1?"ADDED":code==2?"REMOVED":code==3?"EQUIP?":code==4?"REFUSED":code==5?"USE?":code==6?"UNEQUIP?":code==7?"QUEUED":code==8?"EAT N/A":nullptr;
        if(msg){
            mark_quad(cx-101.f,cy+outer*.40f,202.f,46.f,tinted(0xd0173027u));
            draw_text(cx-89.f,cy+outer*.40f+7.f,3.65f,msg,code==4?0xffffa493u:0xffaef4c3u);
        }
    }
}

int draw(Raw device){
    if(!device||!g_visible)return 0;
    Viewport vp{};
    if(!succeeded(reinterpret_cast<GetViewport>(get(device,kViewport))(device,&vp))||vp.w<320||vp.h<240)return 0;
    Raw block=nullptr, back=nullptr, oldTarget=nullptr, oldDepth=nullptr;
    int ok=0,scene=0,depthWasPresent=0,triangles=0;
    if(!succeeded(reinterpret_cast<CreateBlock>(get(device,kCreateBlock))(device,1,&block))||!block)return 0;
    if(!succeeded(reinterpret_cast<BlockAction>(get(block,4))(block)))goto cleanup;
    if(!succeeded(reinterpret_cast<GetBack>(get(device,kGetBackBuffer))(device,0,0,0,&back))||!back)goto cleanup;
    if(!succeeded(reinterpret_cast<GetSurface>(get(device,kGetTarget))(device,0,&oldTarget))||!oldTarget)goto cleanup;
    if(succeeded(reinterpret_cast<GetDepth>(get(device,kGetDepth))(device,&oldDepth)))depthWasPresent=1;
    if(!succeeded(reinterpret_cast<SetSurface>(get(device,kSetTarget))(device,0,back)))goto cleanup;
    if(!succeeded(reinterpret_cast<SetDepth>(get(device,kSetDepth))(device,nullptr)))goto cleanup;
    if(!succeeded(reinterpret_cast<Scene>(get(device,kBeginScene))(device)))goto cleanup;
    scene=1;
    // Fixed-function state, deterministic and restored below.
    reinterpret_cast<SetObj>(get(device,kVertexShader))(device,nullptr);
    reinterpret_cast<SetObj>(get(device,kPixelShader))(device,nullptr);
    reinterpret_cast<SetFVF>(get(device,kFVF))(device,0x44); // XYZRHW + DIFFUSE
    reinterpret_cast<SetTexture>(get(device,kSetTexture))(device,0,nullptr);
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,7,0); // ZENABLE
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,14,0); // ZWRITE
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,22,1); // CULL_NONE
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,27,1); // ALPHABLEND
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,19,5); // SRCALPHA
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,20,6); // INVSRCALPHA
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,137,0); // LIGHTING
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,28,0); // FOG
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,15,0); // ALPHATEST
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,52,0); // STENCIL
    reinterpret_cast<SetVal>(get(device,kRenderState))(device,174,0); // SCISSOR
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,1,2); // COLOROP SELECTARG1
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,2,0); // COLORARG1 DIFFUSE
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,4,2); // ALPHAOP SELECTARG1
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,5,0); // ALPHAARG1 DIFFUSE
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,1,1,1); // COLOROP DISABLE
    AcquireSRWLockShared(&g_catalogLock);
    __try {
        make_mesh((float)vp.w,(float)vp.h);
        triangles=make_pips((float)vp.w,(float)vp.h);
        draw_labels((float)vp.w,(float)vp.h);
    } __finally {ReleaseSRWLockShared(&g_catalogLock);}
    ok=succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))(device,4,16,g_geometry,sizeof(Vertex)));
    if(ok && triangles>0)ok=succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))(device,4,(UInit)triangles,g_pips,sizeof(Vertex)));
    if(ok && g_labelVert>0)ok=succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))(device,4,(UInit)(g_labelVert/3),g_labels,sizeof(Vertex)));
cleanup:
    if(scene)reinterpret_cast<Scene>(get(device,kEndScene))(device);
    if(oldTarget)reinterpret_cast<SetSurface>(get(device,kSetTarget))(device,0,oldTarget);
    if(depthWasPresent)reinterpret_cast<SetDepth>(get(device,kSetDepth))(device,oldDepth);
    else if(back)reinterpret_cast<SetDepth>(get(device,kSetDepth))(device,nullptr);
    if(block)reinterpret_cast<BlockAction>(get(block,5))(block);
    drop(oldDepth);drop(oldTarget);drop(back);drop(block);
    return ok;
}
int on_fault(){
    if(InterlockedExchange(&g_faults,3)!=3)file_log("RunicWheelLE: SEH exception in drawing; overlay disabled for this session\r\n");
    InterlockedExchange(&g_visible,0);
    return 1;
}
void guarded_frame(Raw device){
    if(g_faults>=3)return;
    if(InterlockedCompareExchange(&g_drawBusy,1,0)!=0)return;
    __try {
        if(InterlockedExchange(&g_pendingReset,0)) {
            InterlockedExchange(&g_visible,0);
            InterlockedExchange(&g_resetInput,1);
            file_log("RunicWheelLE: reset on save/load\r\n");
        }
        if(!g_firstFrame){
            InterlockedExchange(&g_firstFrame,1);
            format_thread_log("RunicWheelLE: first Present thread ");
        }
        if(g_window && GetForegroundWindow()!=g_window && g_visible) {
            InterlockedExchange(&g_visible,0);
            InterlockedExchange(&g_resetInput,1);
        }
        if(g_visible) {
            const int drawn=draw(device);
            if(InterlockedCompareExchange(&g_renderLogged,1,0)==0)
                file_log(drawn ? "RunicWheelLE: first D3D9 wheel draw SUCCEEDED\r\n" : "RunicWheelLE: first D3D9 wheel draw FAILED\r\n");
        }
        // Feedback uses wall-clock milliseconds rather than one single rendered frame.
    }__except(on_fault()){}
    InterlockedExchange(&g_drawBusy,0);
}
HResult W on_present(Raw device,const Rectangle* src,const Rectangle* dst,Handle wnd,const void* dirty){
    guarded_frame(device);
    schedule_type_refresh();
    schedule_time_reconcile();
    Present next=g_previous;
    return next?next(device,src,dst,wnd,dirty):(HResult)0x8876086c;
}
int install_body(){
    if(InterlockedCompareExchange(&g_installed,0,0))return 1;
    // TESV.exe LE 1.9.32.0.8 device publisher. Exact bytes verified in workstation.
    const unsigned char* pub=(const unsigned char*)v02sig::kPublishFunction;
    if(!readable(pub,15,1))return 0;
    for(int i=0;i<15;++i)if(pub[i]!=v02sig::kPublishBytes[i])return 0;
    const void* ptr=(const void*)v02sig::kDevicePointer;
    if(!readable(ptr,4,0))return 0;
    Raw device=*(Raw const*)ptr;
    if(!readable(device,4,0))return 0;
    Raw* vt=*(Raw**)device;
    if(!readable(vt,18*4,0)||!readable(vt[17],1,1))return 0;
    if(vt[17]==(Raw)&on_present)return 0;
    DeviceParameters params{};
    if(!succeeded(reinterpret_cast<GetParams>(get(device,kCreateParameters))(device,&params))||!IsWindow(params.window))return 0;
    // Set up previous before CAS publication to avoid a callback observing NULL.
    g_device=device;g_window=params.window;
    g_previous=reinterpret_cast<Present>(vt[17]);
    DWord old=0;
    if(!VirtualProtect(&vt[17],4,0x04,&old)){g_previous=nullptr;return 0;}
    Raw displaced=(Raw)InterlockedCompareExchange((volatile Long*)&vt[17],(Long)&on_present,(Long)g_previous);
    DWord unused=0;
    VirtualProtect(&vt[17],4,old,&unused);
    FlushInstructionCache(GetCurrentProcess(),&vt[17],4);
    if(displaced!=(Raw)g_previous){g_previous=nullptr;return 0;}
    InterlockedExchange(&g_installed,1);
    format_thread_log("RunicWheelLE: Present hook installed thread ");
    return 1;
}
int install_fault(){file_log("RunicWheelLE: SEH during hook installation; hook attempt aborted\r\n");return 1;}
void try_install(){
    if(InterlockedCompareExchange(&g_installed,0,0) || InterlockedCompareExchange(&g_installBusy,1,0))return;
    int installed=0;
    __try {installed=install_body();} __except(install_fault()){installed=0;}
    if(!installed)file_log("RunicWheelLE: no device/hook available yet; installation skipped\r\n");
    InterlockedExchange(&g_installBusy,0);
}
void messages_body(SKSEMessagingInterface::Message* msg){
    if(!msg)return;
    if(msg->type==SKSEMessagingInterface::kMessage_NewGame || msg->type==SKSEMessagingInterface::kMessage_PreLoadGame){
        InterlockedExchange(&g_pendingReset,1);
        InterlockedExchange(&g_actionGeneration,InterlockedCompareExchange(&g_actionGeneration,0,0)+1);
        InterlockedExchange(&g_visible,0);
        schedule_time_reconcile();
        InterlockedExchange(&g_menuFlags,0);
        InterlockedExchange(&g_resetInput,1);
        if(msg->type==SKSEMessagingInterface::kMessage_NewGame)queue_state(g_defaultState);
    }
    if(msg->type==SKSEMessagingInterface::kMessage_DataLoaded || msg->type==SKSEMessagingInterface::kMessage_PostLoadGame || msg->type==SKSEMessagingInterface::kMessage_InputLoaded){
        format_thread_log("RunicWheelLE: SKSE message callback thread ");
        try_install();
        if(!g_inputInstalled)install_input_hook();
        install_menu_lifecycle();
        queue_ui_sample();
    }
}
int message_fault() {file_log("RunicWheelLE: SKSE messaging callback fault; action skipped\r\n");return 1;}
void messages(SKSEMessagingInterface::Message* msg){
    __try {messages_body(msg);} __except(message_fault()){}
}
} // namespace
extern "C" __declspec(dllexport) bool __cdecl SKSEPlugin_Query(const SKSEInterface* skse,PluginInfo* info){
    if(!skse||!info)return false;
    info->infoVersion=PluginInfo::kInfoVersion;info->name="Runic Wheel LE";info->version=111;
    if(skse->isEditor||skse->runtimeVersion!=RUNTIME_VERSION_1_9_32_0||!skse->GetPluginHandle)return false;
    begin_log(); file_log("RunicWheelLE: v0.11.2 Query entered\r\n");
    g_handle=skse->GetPluginHandle();return g_handle!=(PluginHandle)kPluginHandle_Invalid;
}
extern "C" __declspec(dllexport) bool __cdecl SKSEPlugin_Load(const SKSEInterface* skse){
    if(!skse||skse->runtimeVersion!=RUNTIME_VERSION_1_9_32_0||skse->isEditor||g_handle==(PluginHandle)kPluginHandle_Invalid||!skse->QueryInterface)return false;
    auto* messaging=(SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);
    if(!messaging||messaging->interfaceVersion<1||!messaging->RegisterListener)return false;
    begin_log();
    g_tasks=(SKSETaskInterface*)skse->QueryInterface(kInterface_Task);
    if(!g_tasks || g_tasks->interfaceVersion<2 || !g_tasks->AddUITask || !g_tasks->AddTask){
        g_tasks=nullptr;
        file_log("RunicWheelLE: SKSE UI task API absent; inventory capture disabled\r\n");
    }
    load_config();file_log("RunicWheelLE: v0.12.1 readable emblems + safe base inventory counting; MSVC x86\r\n");
    auto* serial=(SKSESerializationInterface*)skse->QueryInterface(kInterface_Serialization);
    if(!serial||serial->version<SKSESerializationInterface::kVersion||!serial->SetUniqueID||
        !serial->SetSaveCallback||!serial->SetLoadCallback||!serial->SetRevertCallback)return false;
    serial->SetUniqueID(g_handle,kSaveUnique);
    serial->SetSaveCallback(g_handle,on_save);
    serial->SetLoadCallback(g_handle,on_load);
    serial->SetRevertCallback(g_handle,on_revert);
    if(!messaging->RegisterListener(g_handle,"SKSE",messages)){file_log("RunicWheelLE: SKSE RegisterListener failed\r\n");return false;}
    file_log("RunicWheelLE: SKSE lifecycle registration succeeded\r\n");
    return true;
}
