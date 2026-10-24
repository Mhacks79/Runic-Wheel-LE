// Runic Wheel LE v0.23.0; large inventory-only editor guide, contextual HUD and stack badges.
// Original v0.17 gameplay paths preserved unless explicitly audited.
// Copyright (c) 2026 Mhacks79. No third-party game/mod assets or Wheeler source.
// Uses the official SKSE 1.7.3 header for ABI; only Win32 C and D3D9 COM interfaces cross DLL boundaries.
// Native equipment actions gated by ownership/variant checks and executed on SKSE game tasks.
#define RUNIC_VERSION "0.24.0-khaos-customization-private"
#include "InputLogic.h"
#include "LayoutGeometry.h"
#include "KhaosAPI.h"
#include "KhaosThemeAPI.h"
#include "SlotCatalog.h"
#include "CatalogSave.h"
#include "CatalogEditor.h"
#include "BindingModel.h"
#include "SaveV2.h"
#include "ActionPolicy.h"
#include "NativeActionPolicy.h"
#include "SpellMembership.h"
#include "DisplayName.h"
#include "IconPolicy.h"
#include "StackNavigation.h"
#include "ScrollPulsePolicy.h"
#include "AdvancedEditor.h"
#include "GuidePlacement.h"
#include "KhaosWindowPolicy.h"
#include "ProfessionalFontMap.h"
#include "PresentationTimeline.h"
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
// Texture coordinate values are float: art is independent of game thread state.
struct TextureVertex {float x,y,z,rhw; DWord color; float u,v;};
struct LockedRectangle {Long pitch; void* bits;};
IMPORT void W OutputDebugStringA(const char*);
IMPORT DWord W GetCurrentThreadId();
IMPORT DWord W GetTickCount();
IMPORT UInit W GetTempPathA(UInit,char*);
IMPORT UInit W GetModuleFileNameA(Handle,char*,UInit);
IMPORT Long W GetModuleHandleExA(DWord,const char*,Handle*);
IMPORT Handle W CreateFileA(const char*,DWord,DWord,void*,DWord,DWord,Handle);
IMPORT Long W WriteFile(Handle,const void*,DWord,DWord*,void*);
IMPORT Long W ReadFile(Handle,void*,DWord,DWord*,void*);
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
IMPORT Long W WritePrivateProfileStringA(const char*,const char*,const char*,const char*);
IMPORT void* W GetProcAddress(Handle,const char*);
static_assert(sizeof(void*) == 4, "Win32 required");
static_assert(sizeof(MBI) == 28, "Win32 VirtualQuery layout");
static_assert(sizeof(DeviceParameters) == 16, "D3D9 creation params layout");
static_assert(sizeof(Vertex) == 20, "D3D9 XYZRHW+DIFFUSE stride");
static_assert(sizeof(TextureVertex) == 28, "D3D9 XYZRHW+DIFFUSE+TEX1 stride");
extern "C" int _fltused=0; // Required by MSVC-target x86 code using floating point, no CRT entry point.

namespace {
using Raw = void*;
using Present = HResult (W*)(Raw,const Rectangle*,const Rectangle*,Handle,const void*);
// COM slot ordinals from IDirect3DDevice9 / IDirect3DStateBlock9 in the D3D9 SDK (not TESV virtual tables).
constexpr UInit kCreateParameters=9,kGetBackBuffer=18,kSetTarget=37,kGetTarget=38,kSetDepth=39,kGetDepth=40;
constexpr UInit kBeginScene=41,kEndScene=42,kViewport=48,kRenderState=57,kCreateBlock=59;
constexpr UInit kSetTexture=65,kTextureStage=67,kSamplerState=69,kDrawUP=83,kFVF=89,kVertexShader=92,kPixelShader=107;
constexpr UInit kCreateTexture=23,kLockTexture=19,kUnlockTexture=20;
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
using SetSampler = HResult (W*)(Raw,DWord,DWord,DWord);
using CreateTexture = HResult (W*)(Raw,UInit,UInit,UInit,DWord,DWord,DWord,Raw*,void*);
using TextureLock = HResult (W*)(Raw,UInit,LockedRectangle*,const void*,DWord);
using TextureUnlock = HResult (W*)(Raw,UInit);
using DrawUp = HResult (W*)(Raw,UInit,UInit,const void*,UInit);
static PluginHandle g_handle = kPluginHandle_Invalid;
static Present g_previous = nullptr;
static Raw g_device = nullptr;
static Handle g_window = nullptr;
static volatile Long g_installed=0, g_drawBusy=0, g_faults=0, g_visible=0;
static volatile Long g_firstFrame=0, g_select=0, g_activeWheel=0, g_item=0, g_actionFlash=0;
static volatile Long g_installBusy=0, g_pendingReset=0, g_resetInput=0, g_renderLogged=0, g_inputInstalled=0;
static Vertex g_geometry[48];
static Vertex g_pips[3*(8*4+48)]; // Up to 2 focus arcs during sector crossfade
static int g_pipsUsed=0;
static Vertex g_labels[30000];
static TextureVertex g_artQuads[8*6]; // one primary medallion per sector
static TextureVertex g_miniArtQuads[8*6]; // dedicated late D3D9 pass for all stacked previews
static TextureVertex g_frameQuads[6];
static TextureVertex g_plaqueQuads[6];
static TextureVertex g_fontQuads[6*1400];
static int g_fontVertexCount=0;
static int g_artVertexCount=0;
static int g_miniArtVert=0;
static int g_miniCount=0;
static volatile Long g_guideEditorLogged=0, g_previewLogged=0;
static Raw g_iconAtlas=nullptr;
static Raw g_frameAtlas=nullptr;
static Raw g_fontAtlas=nullptr;
static Raw g_plaqueAtlas=nullptr;
static Raw g_atlasOwner=nullptr;
static int g_iconAtlasChecked=0;
static int g_firstThemeEnabled=1;
static volatile Long g_feedback=0, g_feedbackTick=0;
// Only these timestamps/flags cross input and Present threads. No TESV calls during rendering.
static volatile Long g_openTick=0, g_closeTick=0, g_exitArmed=0;
static int g_motionEnabled=1, g_reduceMotion=0, g_openDuration=160, g_closeDuration=110, g_sectorDuration=125;
static DWord g_presentationNow=0;
static rw16::Frame g_presentationFrame{1.f,1.f,true};
static int g_focusFrom=0, g_focusTo=0, g_focusWheel=-1;
static DWord g_focusStart=0, g_focusOpenStamp=0;
static float g_focusBlend=1.f;
// Presentation-only cache: game-thread copies fixed-size names, renderer reads under catalog lock.
static char g_displayNames[rw05::Wheels][rw05::Sectors][rw05::Capacity][rw12ui::NameBytes]{};
static unsigned char g_iconIndices[rw05::Wheels][rw05::Sectors][rw05::Capacity]{};
static volatile Long g_namesEnabled=1, g_namesReported=0;
static volatile Long g_namesResolved=0;
static int g_showNames=1;
static int g_sectorLabels=0; // opt-in: primary name is in the large lower plaque
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
static volatile Long g_editorResetRequired=0; // menu thread -> input thread
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
    // The prefix must be copied too; earlier builds accidentally left an empty INI path.
    for(UInit i=0;i<start;++i)g_configPath[i]=g_logPath[i];
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
        if(!e->opening)InterlockedExchange(&g_editorResetRequired,1);
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
      case 0x1F:return rw05::Torch; // TESObjectLIGH in LE; never invent an Armor subtype
      default:return rw05::Unspecified;}
}
struct SpellVisualInfo {unsigned type=0xFFFFFFFFu; unsigned school=0xFFFFFFFFu;};
SpellVisualInfo spell_visual_info(Raw form,unsigned formType){
    SpellVisualInfo info{};
    if(formType==0x77){info.type=7;return info;}
    if(formType!=0x16 || !readable(form,0x78,0))return info;
    info.type=*(const DWord*)((const unsigned char*)form+0x74); // SpellItem::Data::type at 0x6C+0x08
    // SpellItem::Data::type is definitive for powers/lesser powers/abilities.
    // For ordinary spells, sample up to 32 effects and choose the first VALID
    // school; some modded spells use an unschooled initial helper effect.
    if(info.type==2 || info.type==3 || info.type==4 || info.type==6 || info.type==7)return info;
    const unsigned effectCount=*(const DWord*)((const unsigned char*)form+0x3C);
    Raw entries=*(Raw const*)((const unsigned char*)form+0x34);
    if(!effectCount || effectCount>32 || !readable(entries,effectCount*sizeof(Raw),0))return info;
    for(unsigned i=0;i<effectCount;++i){
        Raw effect=((Raw const*)entries)[i];
        if(!readable(effect,0x10,0))continue;
        Raw mgef=*(Raw const*)((const unsigned char*)effect+0x0C);
        if(!readable(mgef,0x48,0))continue;
        const unsigned school=*(const DWord*)((const unsigned char*)mgef+0x44); // EffectSetting::properties.school
        if((school>=18 && school<=22) || school<=4){info.school=school;break;}
    }
    return info;
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
    int leftClickRightHand,inputDiagnostics;
};
// An explicit constant initializer is mandatory with /NOENTRY (no C++ constructors).
static Settings g_cfg={2,0,16,18,2, 3,4,260,100,22, 1,1,9,0,10,12, 0,1,8,9,1, 512,256,45,4,8,130,1,70,900,1,0};
static int g_reverseItemScroll=0,g_itemPreviousKey=0,g_itemNextKey=0;
static int g_skipPreviousKey=0,g_skipNextKey=0,g_showNextPreview=1,g_magicSchoolAccents=1;
static rw20::ScrollPulse g_scrollEdges[2]{};
// Advanced editor is only active when Inventory/Magic is open AND the wheel is visible.
// Input-thread-only state; only g_editorDisplay is read across threads.
static rw21::Modifiers g_modifiers{};
static rw21::Pending g_advancedPending{};
static int g_enableAdvancedEditor=1,g_showEditorHelp=1,g_showWheelControlsGuide=1,g_showWheelOccupancy=1,g_quickSectorKeys=0;
static int g_showAllStackPreviews=1,g_showStackPositionDots=1;
static volatile Long g_editorDisplay=0; // code for visible status, never a TESV pointer
static int g_clearArmedWheel=-1,g_clearArmedSector=-1;
static DWord g_clearArmedTick=0;
static int g_wheelClearArmed=-1;
static DWord g_wheelClearTick=0;

static volatile Long g_khaosOpen=0;
static int g_gamepadDeadzonePercent=22;
static int g_pageCount=3,g_sectorCount=8; // UI capacity, hidden entries kept in RWL2
static int g_offsetX=0,g_offsetY=0,g_iconScalePercent=100,g_textScalePercent=100;
static int g_khaosFontOverride=0;
static int g_visualAccentR=219,g_visualAccentG=186,g_visualAccentB=125;
static int g_backgroundStrength=88,g_showDecorativeFrame=1;
static int g_showWheelTitles=1,g_showStackHints=1;
static char g_wheelTitles[3][24]={{'W','H','E','E','L',' ','I',0},{'W','H','E','E','L',' ','I','I',0},{'W','H','E','E','L',' ','I','I','I',0}};
void load_wheel_titles(){
    const char* const fallbacks[3]={"WHEEL I","WHEEL II","WHEEL III"};
    for(int i=0;i<3;++i){
        char key[]="Wheel1Title";key[5]=char('1'+i);
        char raw[24]{};
        GetPrivateProfileStringA("Controls",key,fallbacks[i],raw,sizeof raw,g_configPath);
        unsigned k=0;
        for(unsigned j=0;raw[j] && k<sizeof(g_wheelTitles[0])-1;++j){
            const unsigned char c=(unsigned char)raw[j];
            if(c>=32 && c<=126)g_wheelTitles[i][k++]=(char)c;
        }
        g_wheelTitles[i][k]=0;
    }
}

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
    const unsigned wheel=(unsigned)rw02::clamp((int)g_activeWheel,0,g_pageCount-1);
    const unsigned sector=(unsigned)rw02::clamp((int)g_select,0,g_sectorCount-1);
    bool changed=false;
    unsigned before=0,after=0,count=0;
    DWord actual=0;
    AcquireSRWLockExclusive(&g_catalogLock);
    __try {
        count=g_model.slot[wheel][sector].count;
        before=g_model.cursor[wheel][sector];
        changed=rw19::cycle(g_model,wheel,sector,direction);
        after=g_model.cursor[wheel][sector];
        if(changed){
            InterlockedExchange(&g_cursors[wheel][sector],(Long)after);
            actual=g_model.slot[wheel][sector].item[after].formId;
        }
    } __finally {ReleaseSRWLockExclusive(&g_catalogLock);}
    if(changed){
        visual_feedback(9);
        // Always log a real transition during this private build so runtime QA
        // can distinguish an input pulse from a changed FormID and cursor.
        char event[160]="RunicWheelLE: STACK W0 S0 0/0 -> 0/0 FORM=00000000\r\n";
        event[21]=(char)('1'+wheel);event[24]=(char)('1'+sector);
        event[26]=(char)('1'+before);event[28]=(char)('0'+count);
        event[33]=(char)('1'+after);event[35]=(char)('0'+count);
        const char* hex="0123456789ABCDEF";
        for(unsigned j=0;j<8;++j)event[42+j]=hex[(actual>>(28-j*4))&15u];
        file_log(event);
    }else if(g_cfg.inputDiagnostics)file_log("RunicWheelLE: stack has fewer than two distinct entries at current sector\r\n");
    select_current();
}
void select_next_occupied(int direction){
    const unsigned wheel=(unsigned)rw02::clamp((int)g_activeWheel,0,g_pageCount-1);
    const unsigned sector=(unsigned)rw02::clamp((int)g_select,0,g_sectorCount-1);
    unsigned char counts[8]{};
    AcquireSRWLockShared(&g_catalogLock);
    __try {for(unsigned i=0;i<(unsigned)g_sectorCount;++i)counts[i]=g_model.slot[wheel][i].count;}
    __finally {ReleaseSRWLockShared(&g_catalogLock);}
    const unsigned next=rw20::next_filled(counts,sector,direction);
    if(next!=sector){InterlockedExchange(&g_select,(Long)next);select_current();}
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
    if(kind!=rw05::Weapon && kind!=rw05::Armor && kind!=rw05::Consumable && kind!=rw05::Ammo && kind!=rw05::Torch){
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
    if(kind==rw05::Torch){
        // TESObjectLIGH (0x1F) equips in the left hand in Skyrim LE. Use the
        // already signature-verified EquipManager entrypoint on the GAME thread.
        // Only inventory-owned, non-ambiguous base forms reach this branch.
        Raw worn=owned.wornLeft?owned.wornLeft:owned.wornRight;
        if(worn){
          reinterpret_cast<GameUnequip>(0x006EE560)(manager,actor,form,worn,1,nullptr,true,false,true,false,nullptr);
          action_diagnostic("RunicWheelLE: TORCH unequip request submitted\r\n",6);
        }else{
          reinterpret_cast<GameEquip>(0x006EF3E0)(manager,actor,form,nullptr,1,nullptr,true,false,false,nullptr);
          action_diagnostic("RunicWheelLE: TORCH equip request submitted (verify in game)\r\n",3);
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
                g_iconIndices[w][sector][i]=rw14::Misc;
                if(i>=slot.count)continue;
                rw05::Binding& b=slot.item[i];
                if(!b.formId)continue;
                Raw form=reinterpret_cast<Raw (__cdecl*)(DWord)>(0x00451A30)(b.formId);
                if(!readable(form,0x14,0)||*(DWord*)((unsigned char*)form+0x0C)!=b.formId)continue;
                const unsigned formType=*((const unsigned char*)form+0x12);
                if(!b.kind)b.kind=(unsigned char)form_kind(formType);
                // SKSE LE GameObjects.h: WEAP.gameData.type at 0xF1;
                // ARMO.bipedObject.data.parts at 0xE4; ALCH.itemData.flags at 0xA8;
                // AMMO.settings.flags at 0x90. Only on game task, after bounds checks.
                unsigned subtype=0,armorBits=0,alchemyFlags=0,ammoFlags=0;
                const SpellVisualInfo spellInfo=spell_visual_info(form,formType);
                if(formType==0x29 && readable(form,0xF2,0))subtype=*((const unsigned char*)form+0xF1);
                if(formType==0x1A && readable(form,0xE8,0))armorBits=*(const DWord*)((const unsigned char*)form+0xE4);
                if(formType==0x2E && readable(form,0xAC,0))alchemyFlags=*(const DWord*)((const unsigned char*)form+0xA8);
                if(formType==0x2A && readable(form,0x94,0))ammoFlags=*(const DWord*)((const unsigned char*)form+0x90);
                g_iconIndices[w][sector][i]=rw14::icon_for(formType,subtype,armorBits,alchemyFlags,ammoFlags,spellInfo.type,spellInfo.school);
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
           binding=rw05::current(snapshot,snapshot.activeWheel,(unsigned)rw02::clamp((int)g_select,0,g_sectorCount-1));}
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
    // Auto follows key POSITIONS: scan 16 = Q on QWERTY / A on AZERTY;
    // scan 18 = E on both. Explicit legacy logical text Q/E now mode 3.
    g_cfg.keyboardMode=value("KeyboardLayoutMode",2,0,3);
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
    g_reverseItemScroll=value("ReverseItemScroll",0,0,1);
    g_itemPreviousKey=value("PreviousItemScanCode",0,0,255);
    g_itemNextKey=value("NextItemScanCode",0,0,255);
    g_skipPreviousKey=value("PreviousOccupiedSectorScanCode",0,0,255);
    g_skipNextKey=value("NextOccupiedSectorScanCode",0,0,255);
    g_showNextPreview=value("ShowNextItemPreview",1,0,1);
    g_showAllStackPreviews=value("ShowAllStackPreviews",1,0,1);
    g_showStackPositionDots=value("ShowStackPositionDots",1,0,1);
    g_magicSchoolAccents=value("ShowMagicSchoolAccents",1,0,1);
    // Never steal Skyrim's F2-F5 keys, even when a legacy INI requests it.
    if(g_itemPreviousKey>=0x3C && g_itemPreviousKey<=0x3F)g_itemPreviousKey=0;
    if(g_itemNextKey>=0x3C && g_itemNextKey<=0x3F)g_itemNextKey=0;
    if(g_itemPreviousKey==g_itemNextKey)g_itemPreviousKey=g_itemNextKey=0;
    if(g_skipPreviousKey>=0x3C && g_skipPreviousKey<=0x3F)g_skipPreviousKey=0;
    if(g_skipNextKey>=0x3C && g_skipNextKey<=0x3F)g_skipNextKey=0;
    if(g_skipPreviousKey==g_skipNextKey)g_skipPreviousKey=g_skipNextKey=0;
    g_showWheelTitles=value("ShowWheelTitles",1,0,1);
    g_showStackHints=value("ShowStackHints",1,0,1);
    g_enableAdvancedEditor=value("EnableAdvancedEditorGestures",1,0,1);
    g_showEditorHelp=value("ShowAdvancedEditorGuide",1,0,1);
    g_showWheelControlsGuide=value("ShowWheelControlsPanel",1,0,1);
    g_showWheelOccupancy=value("ShowWheelOccupancy",1,0,1);
    g_quickSectorKeys=value("EnableQuickSectorKeys",0,0,1);
    load_wheel_titles();
    g_cfg.leftClickRightHand=value("LeftClickEquipsRightHand",1,0,1);
    g_cfg.inputDiagnostics=value("EnableInputDiagnostics",0,0,1);
    g_cfg.closeKey=value("CloseKeyboardScanCode",1,0,255);
    if(g_cfg.closeKey>=0x3C && g_cfg.closeKey<=0x3F){
        g_cfg.closeKey=1;
        file_log("RunicWheelLE: reserved function key CLOSE binding ignored (F2-F5)\r\n");
    }
    // An optional item shortcut may not shadow wheel pages, open or close.
    if(g_itemPreviousKey==g_cfg.previousKey || g_itemPreviousKey==g_cfg.nextKey ||
       g_itemPreviousKey==g_cfg.closeKey || g_itemPreviousKey==g_cfg.keyboardOpen)g_itemPreviousKey=0;
    if(g_itemNextKey==g_cfg.previousKey || g_itemNextKey==g_cfg.nextKey ||
       g_itemNextKey==g_cfg.closeKey || g_itemNextKey==g_cfg.keyboardOpen)g_itemNextKey=0;
    // No optional hotkey may steal an existing wheel, item, close or open key.
    const int reserved[5]={g_cfg.previousKey,g_cfg.nextKey,g_cfg.closeKey,g_cfg.keyboardOpen,g_itemPreviousKey};
    for(unsigned i=0;i<5;++i){
        if(g_skipPreviousKey && g_skipPreviousKey==reserved[i])g_skipPreviousKey=0;
        if(g_skipNextKey && g_skipNextKey==reserved[i])g_skipNextKey=0;
    }
    if(g_skipPreviousKey && g_skipPreviousKey==g_itemNextKey)g_skipPreviousKey=0;
    if(g_skipNextKey && g_skipNextKey==g_itemNextKey)g_skipNextKey=0;
    g_cfg.padPrimary=value("GamepadPrimaryMask",512,0,65535);
    g_cfg.padSecondary=value("GamepadSecondaryMask",256,0,65535);
    g_gamepadDeadzonePercent=value("GamepadDeadzonePercent",22,0,90);
    g_cfg.padDeadzone=g_gamepadDeadzonePercent*2;
    g_cfg.padItemPrevious=value("GamepadPreviousItemMask",4,0,65535);
    g_cfg.padItemNext=value("GamepadNextItemMask",8,0,65535);
    g_cfg.wheelScale=value("WheelScalePercent",130,75,170);
    g_pageCount=value("VisiblePageCount",3,1,3);
    g_sectorCount=value("VisibleSectorCount",8,4,8);
    g_offsetX=value("WheelOffsetXPercent",0,-35,35);
    g_offsetY=value("WheelOffsetYPercent",0,-30,30);
    g_iconScalePercent=value("IconSizePercent",100,65,145);
    g_textScalePercent=value("TextScalePercent",100,65,150);
    g_visualAccentR=value("AccentRed",219,0,255);
    g_visualAccentG=value("AccentGreen",186,0,255);
    g_visualAccentB=value("AccentBlue",125,0,255);
    g_backgroundStrength=value("BackgroundStrengthPercent",88,30,100);
    g_showDecorativeFrame=value("ShowDecorativeFrame",1,0,1);
    g_cfg.nativeActions=value("EnableNativeActions",1,0,1);
    g_enableSpellActions=value("EnableSpellActions",1,0,1);
    g_cfg.wheelOpacity=value("WheelOpacityPercent",70,25,100);
    g_firstThemeEnabled=value("ShowConceptIcons",1,0,1);
    g_showNames=value("ShowItemNames",1,0,1);
    g_sectorLabels=value("ShowSectorLabels",0,0,1);
    g_cfg.feedbackDuration=value("ClickFeedbackMs",900,250,2000);
    g_motionEnabled=value("EnablePresentationAnimations",1,0,1);
    g_reduceMotion=value("ReduceMotion",0,0,1);
    g_openDuration=value("OpenAnimationMs",160,0,400);
    g_closeDuration=value("CloseAnimationMs",110,0,300);
    g_sectorDuration=value("SectorTransitionMs",125,0,250);
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
    if(g_model.activeWheel>=g_pageCount)g_model.activeWheel=0;
    publish_state_locked(g_model);
    file_log("RunicWheelLE: deprecated editor hotkeys removed; only automatic menu context\r\n");
    g_defaultState=g_model;
    select_current();
}
// The canonical V2 state is independent from the D3D9 legacy read-only projection.
void snapshot_state_locked(rw05::State& state){
    state=g_model;
    state.activeWheel=(unsigned char)rw02::clamp((int)g_activeWheel,0,g_pageCount-1);
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
    InterlockedExchange(&g_activeWheel,rw02::clamp((int)state.activeWheel,0,g_pageCount-1));
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
    rw21::cancel(g_advancedPending);g_clearArmedWheel=g_clearArmedSector=-1;g_wheelClearArmed=-1;
    InterlockedExchange(&g_editorDisplay,0);
    select_current();
    InterlockedExchange(&g_refreshNeeded,1);
}
// In-game editor intentionally fails closed until hovered-object identity is verified.

// No native action can be triggered from these commands. They edit typed RWL2
// catalog data under the existing exclusive lock and preserve the co-save ABI.
void advanced_wheel_operation(int direction,unsigned gesture){
    if(!g_enableAdvancedEditor || !inventory_context() ||
       (gesture!=(rw21::Control|rw21::Alt) && gesture!=rw21::ShiftAlt))return;
    const unsigned wheel=(unsigned)rw02::clamp((int)g_activeWheel,0,g_pageCount-1);
    const unsigned target=(wheel + (direction<0?(unsigned)(g_pageCount-1):1u))%(unsigned)g_pageCount;
    bool changed=false;
    AcquireSRWLockExclusive(&g_catalogLock);
    __try {
        rw05::State state{};snapshot_state_locked(state);
        if(gesture==rw21::ShiftAlt)changed=rw21::swap_wheels(g_editor,state,wheel,target);
        else changed=rw21::clone_wheel(g_editor,state,wheel,target);
        if(changed)publish_state_locked(state);
    } __finally {ReleaseSRWLockExclusive(&g_catalogLock);}
    if(changed)select_current();
    const int code=changed?(gesture==rw21::ShiftAlt?25:24):4;
    visual_feedback(code);InterlockedExchange(&g_editorDisplay,code);
    file_log(changed?"RunicWheelLE: ADVANCED WHEEL copy/swap committed\r\n":
       "RunicWheelLE: ADVANCED WHEEL refused (destination occupied, empty source or invalid)\r\n");
}
void advanced_reorder(int direction){
    if(!g_enableAdvancedEditor || !inventory_context())return;
    const unsigned wheel=(unsigned)rw02::clamp((int)g_activeWheel,0,g_pageCount-1);
    const unsigned sector=(unsigned)rw02::clamp((int)g_select,0,g_sectorCount-1);
    bool changed=false;
    AcquireSRWLockExclusive(&g_catalogLock);
    __try {
        rw05::State state{};snapshot_state_locked(state);
        changed=rw21::rotate_entry(g_editor,state,wheel,sector,direction);
        if(changed)publish_state_locked(state);
    } __finally {ReleaseSRWLockExclusive(&g_catalogLock);}
    if(changed)select_current();
    const int code=changed?16:4;
    visual_feedback(code);InterlockedExchange(&g_editorDisplay,code);
}
void advanced_action(unsigned gesture,bool primary){
    if(!g_enableAdvancedEditor || !inventory_context())return;
    const unsigned wheel=(unsigned)rw02::clamp((int)g_activeWheel,0,g_pageCount-1);
    const unsigned sector=(unsigned)rw02::clamp((int)g_select,0,g_sectorCount-1);
    const DWord now=GetTickCount();
    bool changed=false;
    int status=4;
    AcquireSRWLockExclusive(&g_catalogLock);
    __try {
        rw05::State state{};snapshot_state_locked(state);
        if(primary && gesture==rw21::Alt){
            changed=rw21::rotate_entry(g_editor,state,wheel,sector,+1);
            status=changed?16:4;
        }else if(primary){
            rw21::Mode mode=rw21::None;
            if(gesture==rw21::Shift)mode=rw21::MoveItem;
            else if(gesture==rw21::Control)mode=rw21::CopyItem;
            else if(gesture==rw21::ShiftAlt)mode=rw21::SwapSector;
            else if(gesture==rw21::ShiftControl)mode=rw21::CloneSector;
            if(mode!=rw21::None){
                rw21::Result result=rw21::Invalid;
                if(g_advancedPending.mode==mode && (g_advancedPending.wheel!=wheel || g_advancedPending.sector!=sector)){
                    result=rw21::paste(g_advancedPending,g_editor,state,wheel,sector);
                    if(result==rw21::Applied)changed=true;
                }else result=rw21::arm(g_advancedPending,state,mode,wheel,sector);
                status=result==rw21::Applied? (mode==rw21::MoveItem?12:mode==rw21::CopyItem?13:mode==rw21::SwapSector?21:22):
                    result==rw21::Armed? (mode==rw21::MoveItem?10:mode==rw21::CopyItem?11:mode==rw21::SwapSector?20:23):
                    result==rw21::Cancelled?19:4;
            }
        }else if(gesture==rw21::Shift){
            changed=rw05::history(g_editor,state,true);status=changed?14:4;
        }else if(gesture==rw21::Control){
            changed=rw05::history(g_editor,state,false);status=changed?15:4;
        }else if(gesture==rw21::ShiftAlt){
            if(g_wheelClearArmed==(int)wheel && (unsigned)(now-g_wheelClearTick)<=1750u){
                changed=rw21::clear_wheel(g_editor,state,wheel);
                status=changed?27:4;
                g_wheelClearArmed=-1;
            }else{
                g_wheelClearArmed=(int)wheel;g_wheelClearTick=now;status=26;
            }
        }else if(gesture==rw21::Alt){
            if(g_clearArmedWheel==(int)wheel && g_clearArmedSector==(int)sector &&
               (unsigned)(now-g_clearArmedTick)<=1750u){
                changed=rw21::clear_stack(g_editor,state,wheel,sector);
                status=changed?18:4;
                g_clearArmedWheel=g_clearArmedSector=-1;
            }else{
                g_clearArmedWheel=(int)wheel;g_clearArmedSector=(int)sector;
                g_clearArmedTick=now;status=17;
            }
        }
        if(changed){state.activeWheel=(unsigned char)wheel;publish_state_locked(state);}
    } __finally {ReleaseSRWLockExclusive(&g_catalogLock);}
    if(changed){select_current();InterlockedExchange(&g_refreshNeeded,1);}
    visual_feedback(status);
    InterlockedExchange(&g_editorDisplay,status);
    if(g_cfg.inputDiagnostics)file_log(changed?
       "RunicWheelLE: ADVANCED EDIT committed, undoable typed wheel transaction\r\n":
       "RunicWheelLE: ADVANCED EDIT armed/cancelled/refused (no TESV mutation)\r\n");
}

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
    const unsigned wheel=(unsigned)rw02::clamp((int)g_activeWheel,0,g_pageCount-1);
    const unsigned sector=(unsigned)rw02::clamp((int)g_select,0,g_sectorCount-1);
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
void reset_input(){
    rw02::reset(g_activation);g_mouseX=g_mouseY=0;
    g_modifiers.reset();rw21::cancel(g_advancedPending);
    g_clearArmedWheel=g_clearArmedSector=-1;g_wheelClearArmed=-1;
    InterlockedExchange(&g_editorDisplay,0);
    InterlockedExchange(&g_visible,0);InterlockedExchange(&g_exitArmed,0);
    select_current();
}
void change_visible(bool visible){
    const bool was=InterlockedCompareExchange(&g_visible,0,0)!=0;
    if(visible && !was){
        InterlockedExchange(&g_exitArmed,0);
        InterlockedExchange(&g_openTick,(Long)GetTickCount());
    }else if(!visible && was){
        rw21::cancel(g_advancedPending);
        g_clearArmedWheel=g_clearArmedSector=-1;g_wheelClearArmed=-1;
        InterlockedExchange(&g_editorDisplay,0);
        InterlockedExchange(&g_closeTick,(Long)GetTickCount());
        InterlockedExchange(&g_exitArmed,(g_motionEnabled && g_closeDuration)?1:0);
    }
    if((InterlockedExchange(&g_visible,visible?1:0)!=0)!=visible){
        file_log(visible?"RunicWheelLE: input -> wheel OPEN\r\n":"RunicWheelLE: input -> wheel CLOSED\r\n");
    }
}
void activate(bool down){
    bool was=g_activation.visible;
    bool now=rw02::button(g_activation,down,GetTickCount(),(unsigned)g_cfg.holdMs);
    if(!was && now){g_mouseX=g_mouseY=0;InterlockedExchange(&g_renderLogged,0);rw20::reset(g_scrollEdges);}
    if(was!=now)change_visible(now);
    if(!was && now)InterlockedExchange(&g_refreshNeeded,1);
}
void close_wheel(){
    const bool was=g_activation.visible;
    reset_input();
    // Escape closes input immediately. The optional fade is visual only.
    if(was && g_motionEnabled && g_closeDuration){
        InterlockedExchange(&g_closeTick,(Long)GetTickCount());
        InterlockedExchange(&g_exitArmed,1);
    }
    file_log("RunicWheelLE: input -> wheel CANCELLED\r\n");
}
void move_pointer(Long dx,Long dy){
    dx=rw02::clamp((int)dx,-200,200);dy=rw02::clamp((int)dy,-200,200);
    g_mouseX=rw02::clamp((int)(g_mouseX+dx*g_cfg.sensitivity/100),-240,240);
    g_mouseY=rw02::clamp((int)(g_mouseY+dy*g_cfg.sensitivity/100),-240,240);
    int sector=rwl24::select(g_sectorCount,(int)g_mouseX,(int)g_mouseY,g_cfg.deadzone);
    if(sector>=0){InterlockedExchange(&g_select,sector);select_current();}
}
int layout_scancode(char c,int fallback){
    // DirectInput scan codes are physical positions, independent of Windows
    // layout. AZERTY therefore gets A/E automatically without chasing a
    // translated logical Q that sits in the wrong position.
    if(g_cfg.keyboardMode!=3)return fallback;
    Handle layout=GetKeyboardLayout(0);
    short info=VkKeyScanExA(c,layout);
    if(info==-1)return fallback;
    unsigned sc=MapVirtualKeyExA((unsigned)(info&255),0,layout);
    return sc&&sc<=255?(int)sc:fallback;
}
// Return true when an event must not reach vanilla gameplay.
int process_event(InputEvent* event){
    if(!event)return 0;
    // Foreground ownership is checked BEFORE modal configuration capture.
    // KHA-OS cannot starve Skyrim/Windows if another window gains focus.
    if(g_window && GetForegroundWindow()!=g_window){
        if(g_khaosOpen){
            InterlockedExchange(&g_khaosOpen,0);
            file_log("RunicWheelLE: KHAOS released on foreground loss\r\n");
        }
        if(g_activation.visible)reset_input();
        return 0;
    }
    if(g_khaosOpen){
        // Skyrim Tab opens TweenMenu. Never intercept it: dismiss configuration
        // and forward the same key to vanilla. Escape closes configuration only.
        if(event->type==0 && event->device==0){
            const ButtonEvent* b=(const ButtonEvent*)event;
            if(rw25::close_on_directinput((unsigned)event->device,(unsigned)b->scan,b->value!=0.0f)){
                InterlockedExchange(&g_khaosOpen,0);
                file_log(b->scan==15 ? "RunicWheelLE: KHAOS released Skyrim TAB\r\n" :
                                      "RunicWheelLE: KHAOS closed on ESC\r\n");
                return b->scan==15 ? 0:1;
            }
        }
        return 1; // Modal config captures remaining gameplay actions only.
    }
    const bool visibleBefore=g_activation.visible;
    if(event->type==0 && (event->device==0 || event->device==1 || event->device==2)){
        const ButtonEvent* b=(ButtonEvent*)event;
        const bool down=b->value!=0.0f;
        const bool mouse=event->device==1;
        const bool key=event->device==0;
        const bool pad=event->device==2 && g_cfg.gamepad;
        // Track physical modifiers even when the wheel opens while they are held.
        // Modifier keys themselves always pass through unchanged.
        if(key && g_modifiers.update((unsigned)b->scan,down))return 0;
        // LE physical scancodes 0x3C..0x3F are F2/F3/F4/F5. Pass through
        // unchanged; no hidden edit toggle, undo or save interception.
        if(key && b->scan>=0x3C && b->scan<=0x3F)return 0;
        if((mouse && b->scan==(DWord)g_cfg.openMouse) || (key && g_cfg.keyboardOpen && b->scan==(DWord)g_cfg.keyboardOpen) || (pad && b->scan==(DWord)g_cfg.padOpen)){
            activate(down);return 1; // never trigger the game's middle-button action
        }
        // Wheel pulses are not ordinary held mouse buttons in the LE dispatcher.
        // Some input paths report a nonzero timer or a zero analogue value;
        // the old (down && duration==0) guard incorrectly discarded them.
        if(g_activation.visible && mouse &&
           (b->scan==(DWord)g_cfg.wheelUp || b->scan==(DWord)g_cfg.wheelDown)){
            const unsigned wheelIndex=b->scan==(DWord)g_cfg.wheelUp?0u:1u;
            const bool detent=rw20::accept(g_scrollEdges[wheelIndex],b->value,b->duration,GetTickCount());
            if(detent){
                const int direction=(wheelIndex==0u?-1:1)*(g_reverseItemScroll?-1:1);
                if(g_enableAdvancedEditor && inventory_context() && g_modifiers.gesture()==rw21::Alt)
                    advanced_reorder(direction);
                else cycle_entry(direction);
                if(g_cfg.inputDiagnostics)file_log("RunicWheelLE: accepted single mouse wheel detent\r\n");
            }else if(g_cfg.inputDiagnostics)file_log("RunicWheelLE: ignored wheel release/duplicate detent\r\n");
            return 1;
        }
        // No F2/F5 editor modes. Edit only when Inventory/Magic menu is actually open.
        if(g_activation.visible && down && b->duration==0.0f){
            if(mouse && b->scan==(DWord)g_cfg.primaryMouse){
                g_actionFlash=1;
                if(inventory_context()){
                    if(g_enableAdvancedEditor && g_modifiers.gesture()!=rw21::Plain)advanced_action(g_modifiers.gesture(),true);
                    else edit_click(false);
                }else log_action(g_cfg.leftClickRightHand?"RIGHT":"LEFT");
            }
            if(mouse && b->scan==(DWord)g_cfg.secondaryMouse){
                g_actionFlash=2;
                if(inventory_context()){
                    if(g_enableAdvancedEditor && g_modifiers.gesture()!=rw21::Plain)advanced_action(g_modifiers.gesture(),false);
                    else edit_click(true);
                }else log_action(g_cfg.leftClickRightHand?"LEFT":"RIGHT");
            }
            if(pad && b->scan==(DWord)g_cfg.padPrimary){g_actionFlash=1;if(inventory_context())edit_click(false);else log_action("RIGHT");}
            if(pad && b->scan==(DWord)g_cfg.padSecondary){g_actionFlash=2;if(inventory_context())edit_click(true);else log_action("LEFT");}
            if(pad && ((g_cfg.padItemPrevious && b->scan==(DWord)g_cfg.padItemPrevious) || (g_cfg.padItemNext && b->scan==(DWord)g_cfg.padItemNext))){
                cycle_entry(b->scan==(DWord)g_cfg.padItemPrevious?-1:1);
            }
            if(key && ((g_itemPreviousKey && b->scan==(DWord)g_itemPreviousKey) ||
                       (g_itemNextKey && b->scan==(DWord)g_itemNextKey))){
                cycle_entry(b->scan==(DWord)g_itemPreviousKey?-1:1);
            }
            if(key && ((g_skipPreviousKey && b->scan==(DWord)g_skipPreviousKey) ||
                       (g_skipNextKey && b->scan==(DWord)g_skipNextKey))){
                select_next_occupied(b->scan==(DWord)g_skipPreviousKey?-1:1);
            }
            if(key && g_quickSectorKeys && b->scan>=2u && b->scan<(DWord)(2+g_sectorCount)){
                // Physical upper-row positions (same key position on AZERTY/QWERTY).
                InterlockedExchange(&g_select,(Long)(b->scan-2u));select_current();
            }
            if((mouse && b->scan==(DWord)g_cfg.prevMouse) || (key && b->scan==(DWord)layout_scancode('Q',g_cfg.previousKey)) || (pad && g_cfg.padPrev && b->scan==(DWord)g_cfg.padPrev)){
                const unsigned modifier=g_modifiers.gesture();
                if(key && g_enableAdvancedEditor && inventory_context() &&
                   (modifier==rw21::ShiftAlt || modifier==(rw21::Control|rw21::Alt)))
                    advanced_wheel_operation(-1,modifier);
                else {InterlockedExchange(&g_activeWheel,rw02::step((int)g_activeWheel,-1,g_pageCount));select_current();}
            }
            if((mouse && b->scan==(DWord)g_cfg.nextMouse) || (key && b->scan==(DWord)layout_scancode('E',g_cfg.nextKey)) || (pad && g_cfg.padNext && b->scan==(DWord)g_cfg.padNext)){
                const unsigned modifier=g_modifiers.gesture();
                if(key && g_enableAdvancedEditor && inventory_context() &&
                   (modifier==rw21::ShiftAlt || modifier==(rw21::Control|rw21::Alt)))
                    advanced_wheel_operation(+1,modifier);
                else {InterlockedExchange(&g_activeWheel,rw02::step((int)g_activeWheel,+1,g_pageCount));select_current();}
            }
        }
        if(key && b->scan==(DWord)g_cfg.closeKey && down && g_activation.visible){close_wheel();return 1;}
        if(g_cfg.suppress && (visibleBefore||g_activation.visible)) {
           if(mouse && (b->scan==(DWord)g_cfg.primaryMouse||b->scan==(DWord)g_cfg.secondaryMouse||b->scan==(DWord)g_cfg.wheelUp||b->scan==(DWord)g_cfg.wheelDown||b->scan==(DWord)g_cfg.prevMouse||b->scan==(DWord)g_cfg.nextMouse))return 1;
           if(key && ((g_quickSectorKeys && b->scan>=2u && b->scan<=9u) ||
                     b->scan==(DWord)layout_scancode('Q',g_cfg.previousKey)||b->scan==(DWord)layout_scancode('E',g_cfg.nextKey)||
              (g_itemPreviousKey && b->scan==(DWord)g_itemPreviousKey)||(g_itemNextKey && b->scan==(DWord)g_itemNextKey)||
              (g_skipPreviousKey && b->scan==(DWord)g_skipPreviousKey)||(g_skipNextKey && b->scan==(DWord)g_skipNextKey)))return 1;
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
            int selected=rwl24::select(g_sectorCount,x,y,g_cfg.padDeadzone);
            if(selected>=0){InterlockedExchange(&g_select,selected);select_current();}
            return g_cfg.suppress;
        }
    }
    return 0;
}

void dispatch_input(void* dispatcher,InputEvent** head){
    if(InterlockedExchange(&g_resetInput,0))reset_input();
    if(InterlockedExchange(&g_editorResetRequired,0)){
        rw21::cancel(g_advancedPending);
        g_clearArmedWheel=g_clearArmedSector=-1;g_wheelClearArmed=-1;
        InterlockedExchange(&g_editorDisplay,0);
    }
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
DWord tinted(DWord col){
    // Opacity 70 is the design baseline; presentation alpha is applied to *all* layers later.
    const unsigned alpha=(col>>24)&255u;
    unsigned adjusted=(alpha*(unsigned)g_cfg.wheelOpacity*(unsigned)g_backgroundStrength)/(70u*88u);
    if(adjusted>255u)adjusted=255u;
    return (col&0x00FFFFFFu)|(adjusted<<24);
}
float sector_weight(int sector){
    if(sector==g_focusTo)return g_focusFrom==g_focusTo?1.f:g_focusBlend;
    return sector==g_focusFrom?1.f-g_focusBlend:0.f;
}
void update_visual_focus(DWord now,int wheel,int selected,DWord opened){
    if(g_focusWheel!=wheel || g_focusOpenStamp!=opened){
        g_focusWheel=wheel;g_focusOpenStamp=opened;
        g_focusFrom=g_focusTo=selected;g_focusStart=now;g_focusBlend=1.f;
    }else if(selected!=g_focusTo){
        g_focusFrom=g_focusTo;g_focusTo=selected;g_focusStart=now;
    }
    g_focusBlend=rw16::selected_blend(now,g_focusStart,(unsigned)g_sectorDuration,g_motionEnabled,g_reduceMotion);
}
void vertex(Vertex& v,float x,float y,DWord color){v.x=x-0.5f;v.y=y-0.5f;v.z=0;v.rhw=1;v.color=color;}
float wheel_cx(float width){return width*(.5f+(float)g_offsetX/100.f);}
float wheel_cy(float height){const float lift=height*.045f<40.f?height*.045f:40.f;
    return height*(.5f+(float)g_offsetY/100.f)-lift;}
DWord wheel_accent(){return 0xff000000u|((DWord)g_visualAccentR<<16)|((DWord)g_visualAccentG<<8)|(DWord)g_visualAccentB;}
void make_mesh(float w,float h){
    float d=w<h?w:h;float outer=d*0.31f*(float)g_cfg.wheelScale/100.f; if(outer>440.0f)outer=440.0f; if(outer>h*.44f)outer=h*.44f;
    const float lift=h*.045f<40.f?h*.045f:40.f;
    float inner=outer*0.36f,cx=wheel_cx(w),cy=wheel_cy(h);(void)lift;
    int pos=0;
    for(int s=0;s<g_sectorCount;s++){
        const float pulse=rw16::feedback_alpha(g_presentationNow,(DWord)InterlockedCompareExchange(&g_feedbackTick,0,0),(unsigned)g_cfg.feedbackDuration);
        const bool flashing=pulse>0.f && InterlockedCompareExchange(&g_feedback,0,0)!=0;
        const DWord baseOuter=g_catalog.slots[rw02::clamp((int)g_activeWheel,0,2)][s].count?0x78383433u:0x66232429u;
        const DWord selectedOuter=flashing?rw16::lerp_color(wheel_accent(),0xc0ffffffu,pulse):wheel_accent();
        DWord outerCol=rw16::lerp_color(baseOuter,selectedOuter,sector_weight(s));
        const DWord selectedInner=inventory_context()?0x9a668e82u:0xb06d5936u;
        DWord innerCol=rw16::lerp_color(0x75232a30u,selectedInner,sector_weight(s));
        // Original narrow gaps between the sectors, independent from Wheeler artwork.
        constexpr float margin=0.035f;
        const rwl24::Point e0=rwl24::edge(g_sectorCount,s),e1=rwl24::edge(g_sectorCount,s+1);
        const float x0=e0.x*(1-margin)+e1.x*margin;
        const float y0=e0.y*(1-margin)+e1.y*margin;
        const float x1=e1.x*(1-margin)+e0.x*margin;
        const float y1=e1.y*(1-margin)+e0.y*margin;
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
    float outer=diameter*0.31f*(float)g_cfg.wheelScale/100.f;if(outer>440.0f)outer=440.0f;if(outer>height*.44f)outer=height*.44f;
    const float lift=height*.045f<40.f?height*.045f:40.f;
    float cx=wheel_cx(width),cy=wheel_cy(height);(void)lift;
    for(int sector=0;sector<g_sectorCount;++sector){
      int count=g_catalog.slots[wheel][sector].count;
      if(count>(int)rw05::Capacity)count=(int)rw05::Capacity; // Render-only guard: 240 vertices maximum.
      rwl24::Point dir=rwl24::center(g_sectorCount,sector);
      float ux=dir.x,uy=dir.y;
      for(int e=0;e<count;++e){
        float factor=.77f+.055f*e;
        float x=cx+ux*outer*factor, y=cy+uy*outer*factor;
        DWord color=e==(int)g_cursors[wheel][sector]?0xfff4df9cu:0xaa879aaa;
        vertex(g_pips[n++],x-3.6f,y+3.6f,color);
        vertex(g_pips[n++],x+3.6f,y+3.6f,color);
        vertex(g_pips[n++],x,y-3.6f,color);
      }
    }
    // Two independent radial sweeps crossfade. Indices remain exact for game actions;
    // sector 7->0 never rotates across the other six sectors.
    const float weights[2]={g_focusFrom==g_focusTo?0.f:1.f-g_focusBlend, g_focusFrom==g_focusTo?1.f:g_focusBlend};
    const int sectors[2]={g_focusFrom,g_focusTo};
    const float feedback=InterlockedCompareExchange(&g_feedback,0,0)!=0?
        rw16::feedback_alpha(g_presentationNow,(DWord)InterlockedCompareExchange(&g_feedbackTick,0,0),(unsigned)g_cfg.feedbackDuration):0.f;
    for(int arc=0;arc<2;++arc){
        if(weights[arc]<=0.f)continue;
        const int focus=rw02::clamp(sectors[arc],0,7);
        DWord gold=rw16::with_alpha(0x9debd0a0u,weights[arc]);
        if(feedback>0.f)gold=rw16::lerp_color(gold,rw16::with_alpha(0xeaffd991u,weights[arc]),feedback);
        for(int k=0;k<12;++k){
            const float t0=(float)k/12.f,t1=(float)(k+1)/12.f;
            const rwl24::Point ea=rwl24::edge(g_sectorCount,focus),eb=rwl24::edge(g_sectorCount,focus+1);
            const float ax=ea.x*(1.f-t0)+eb.x*t0;
            const float ay=ea.y*(1.f-t0)+eb.y*t0;
            const float bx=ea.x*(1.f-t1)+eb.x*t1;
            const float by=ea.y*(1.f-t1)+eb.y*t1;
            vertex(g_pips[n++],cx+ax*outer*.866f,cy+ay*outer*.866f,gold);
            vertex(g_pips[n++],cx+ax*outer*.878f,cy+ay*outer*.878f,gold);
            vertex(g_pips[n++],cx+bx*outer*.878f,cy+by*outer*.878f,gold);
            vertex(g_pips[n++],cx+ax*outer*.866f,cy+ay*outer*.866f,gold);
            vertex(g_pips[n++],cx+bx*outer*.878f,cy+by*outer*.878f,gold);
            vertex(g_pips[n++],cx+bx*outer*.866f,cy+by*outer*.866f,gold);
        }
    }
    g_pipsUsed=n;
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
void professional_text(float midX,float y,float width,float fontPx,const char* raw,DWord col);
void draw_text(float x,float y,float unit,const char* word,DWord col);
void mark_quad(float x,float y,float width,float height,DWord col){
    if(g_labelVert+6>30000)return;
    Vertex& a=g_labels[g_labelVert++];vertex(a,x,y,col);
    Vertex& b=g_labels[g_labelVert++];vertex(b,x+width,y,col);
    Vertex& c=g_labels[g_labelVert++];vertex(c,x+width,y+height,col);
    Vertex& d=g_labels[g_labelVert++];vertex(d,x,y,col);
    Vertex& e=g_labels[g_labelVert++];vertex(e,x+width,y+height,col);
    Vertex& f=g_labels[g_labelVert++];vertex(f,x,y+height,col);
}
void draw_wheel_pager(float cx,float cy,float outer,int wheel){
    // Enlarged bottom-right page block: readable at 1440p and still legible at 1080p.
    const float x=cx+outer*.84f,y=cy+outer*.78f;
    const float panelW=96.f,panelH=g_showWheelOccupancy?62.f:42.f;
    mark_quad(x-panelW*.5f,y-panelH*.5f,panelW,panelH,tinted(0xd10b1520u));
    mark_quad(x-panelW*.5f,y-panelH*.5f,panelW,2.f,tinted(0xffcfa66eu));
    mark_quad(x-panelW*.5f,y+panelH*.5f-2.f,panelW,2.f,tinted(0x959a7d53u));
    char pager[]="0/0";pager[0]=(char)('1'+wheel);pager[2]=(char)('0'+g_pageCount);
    if(g_fontAtlas)professional_text(x,y-panelH*.5f+2.f,panelW-12.f,17.5f,pager,0xfff8e7c7u);
    else draw_text(x-13.f,y-panelH*.5f+11.f,1.85f,pager,0xfff8e7c7u);
    for(int i=0;i<g_pageCount;++i){
        const float mx=x-18.f+(float)i*18.f;
        const bool active=i==wheel;
        DWord fill=tinted(active?0xffdfb76cu:0x9061604fu);
        mark_quad(mx-6.f,y-3.f,12.f,12.f,tinted(active?0xd5775638u:0x8c2f3944u));
        mark_quad(mx-3.8f,y-.8f,7.6f,7.6f,fill);
    }
    if(g_showWheelOccupancy){
        const unsigned used=rw21::count_occupied(g_model,(unsigned)wheel);
        const float bx=x-31.f,by=y+16.f;
        for(unsigned j=0;j<8u;++j){
            const float xx=bx+(float)j*7.4f;
            mark_quad(xx,by,5.2f,5.0f,tinted(j<used?0xffd9bb79u:0x55485044u));
        }
    }
}
void draw_controls_panel(float width,float height,float cx,float cy,float outer){
    const float scale=width>=2200.f && height>=1200.f?1.18f:width>=1600.f && height>=900.f?1.05f:.95f;
    const float w=324.f*scale,h=272.f*scale;
    const float margin=14.f;
    float x=cx-outer-26.f-w;
    if(x<margin)x=margin;
    float y=cy-outer*.44f;
    if(y<margin)y=margin;
    if(y+h>height-margin)y=height-margin-h;
    mark_quad(x,y,w,h,tinted(0xf20b1520u));
    mark_quad(x,y,w,3.f*scale,tinted(0xffdbba7du));
    mark_quad(x,y+h-2.f*scale,w,2.f*scale,tinted(0xff997c53u));
    mark_quad(x+14.f*scale,y+48.f*scale,w-28.f*scale,1.5f*scale,tinted(0xa7ae8c55u));
    if(g_fontAtlas){
        professional_text(x+w*.5f,y+5.f*scale,w-24.f*scale,24.f*scale,"WHEEL CONTROLS",0xfff6deb0u);
        professional_text(x+w*.5f,y+31.f*scale,w-24.f*scale,12.5f*scale,"INVENTORY / MAGIC ONLY",0xffc8ae86u);
    }else draw_text(x+20.f*scale,y+16.f*scale,1.7f*scale,"CONTROLS",0xfff6deb0u);
    static const char* const lines[]={
        "LMB  - RIGHT HAND",
        "RMB  - LEFT HAND",
        "WHEEL - NEXT STACK ITEM",
        "Q/E OR A/E - CHANGE PAGE",
        "SIDE MOUSE - CHANGE PAGE",
        "MIDDLE - CLOSE WHEEL",
        "HOLD MIDDLE - OPEN WHEEL"};
    for(unsigned i=0;i<7u;++i){
        const float ty=y+60.f*scale+(float)i*26.f*scale;
        if(i%2==0)mark_quad(x+8.f*scale,ty-3.f*scale,w-16.f*scale,22.f*scale,tinted(0x48364b55u));
        if(g_fontAtlas)professional_text(x+w*.5f,ty-7.f*scale,w-22.f*scale,16.5f*scale,lines[i],0xfff0e2cfu);
        else draw_text(x+16.f*scale,ty+2.f*scale,1.3f*scale,lines[i],0xfff0e2cfu);
    }
    char footer[]="0 PAGES / 0 SECTORS";
    footer[0]=(char)('0'+g_pageCount);footer[11]=(char)('0'+g_sectorCount);
    mark_quad(x+16.f*scale,y+h-43.f*scale,w-32.f*scale,27.f*scale,tinted(0xa51b2834u));
    if(g_fontAtlas)professional_text(x+w*.5f,y+h-46.f*scale,w-38.f*scale,15.5f*scale,footer,0xffe4c48bu);
    else draw_text(x+28.f*scale,y+h-36.f*scale,1.5f*scale,footer,0xffe4c48bu);
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
// V0.14: standalone, hand-authored icon assets, never cropped from a collage.
// Every file starts RWAT + uint32 width + uint32 height + little-endian BGRA pixels.
void release_theme(){
    drop(g_iconAtlas);g_iconAtlas=nullptr;
    drop(g_frameAtlas);g_frameAtlas=nullptr;
    drop(g_fontAtlas);g_fontAtlas=nullptr;
    drop(g_plaqueAtlas);g_plaqueAtlas=nullptr;
    g_atlasOwner=nullptr;g_iconAtlasChecked=0;
}
int texture_path(const char* filename,char* out,UInit cap){
    if(!g_logReady)return 0;
    UInit i=0,last=0;
    for(;g_logPath[i]&&i<230;++i){out[i]=g_logPath[i];if(out[i]=='\\'||out[i]=='/')last=i+1;}
    UInit n=0;while(filename[n]&&last+n+1<cap){out[last+n]=filename[n];++n;}
    if(filename[n])return 0;
    out[last+n]=0;return 1;
}
Raw read_texture(Raw device,const char* basename,DWord width,DWord height){
    char filename[280]{};
    if(!texture_path(basename,filename,sizeof(filename)))return nullptr;
    Handle f=CreateFileA(filename,0x80000000u,1,nullptr,3,0x80,nullptr);
    if(f==(Handle)(-1))return nullptr;
    unsigned char header[12];DWord bytes=0;
    Raw tex=nullptr;
    if(ReadFile(f,header,12,&bytes,nullptr)&&bytes==12 &&
       header[0]=='R'&&header[1]=='W'&&header[2]=='A'&&header[3]=='T' &&
       *(const DWord*)(header+4)==width&&*(const DWord*)(header+8)==height &&
       succeeded(reinterpret_cast<CreateTexture>(get(device,kCreateTexture))
         (device,width,height,1,0,21,1,&tex,nullptr)) && tex){
        LockedRectangle rect{};
        if(succeeded(reinterpret_cast<TextureLock>(get(tex,kLockTexture))(tex,0,&rect,nullptr,0)) &&
           rect.bits && rect.pitch>=(Long)(width*4u)){
            int complete=1;
            for(UInit row=0;row<height;row++){
                unsigned char* dest=(unsigned char*)rect.bits+row*(UInit)rect.pitch;
                if(!ReadFile(f,dest,width*4,&bytes,nullptr)||bytes!=width*4){complete=0;break;}
            }
            reinterpret_cast<TextureUnlock>(get(tex,kUnlockTexture))(tex,0);
            if(!complete){drop(tex);tex=nullptr;}
        }else{drop(tex);tex=nullptr;}
    }
    CloseHandle(f);
    return tex;
}
int load_theme(Raw device){
    if(!g_firstThemeEnabled||!device)return 0;
    if(g_iconAtlas&&g_frameAtlas&&g_fontAtlas&&g_atlasOwner==device)return 1;
    if(g_atlasOwner && g_atlasOwner!=device)release_theme();
    if(g_iconAtlasChecked)return g_iconAtlas&&g_frameAtlas&&g_fontAtlas;
    g_iconAtlasChecked=1;
    g_atlasOwner=device;
    g_iconAtlas=read_texture(device,"RunicWheelIcons.rwtx",2048,1024);
    g_frameAtlas=read_texture(device,"RunicWheelFrame.rwtx",1024,1024);
    g_fontAtlas=read_texture(device,"RunicWheelFont.rwtx",1024,1024);
    g_plaqueAtlas=read_texture(device,"RunicWheelPlaque.rwtx",1024,256);
    if(g_iconAtlas&&g_frameAtlas&&g_fontAtlas){
       file_log("RunicWheelLE: V018 32 independent medallions, arcana schools, powers and Unicode font loaded\r\n");
       if(g_plaqueAtlas)file_log("RunicWheelLE: V015 gold-trimmed nameplate atlas loaded\r\n");
       return 1;
    }
    file_log("RunicWheelLE: V014 art asset missing; per-layer safe fallback enabled\r\n");
    return 0;
}
void artwork_vertex(TextureVertex& v,float x,float y,DWord color,float u,float vv){
    v.x=x-.5f;v.y=y-.5f;v.z=0;v.rhw=1;v.color=color;v.u=u;v.v=vv;
}
void quad_tex(TextureVertex* q,int& n,float x,float y,float w,float h,
              DWord col,float u0,float v0,float u1,float v1){
    artwork_vertex(q[n++],x,y,col,u0,v0);
    artwork_vertex(q[n++],x+w,y,col,u1,v0);
    artwork_vertex(q[n++],x+w,y+h,col,u1,v1);
    artwork_vertex(q[n++],x,y,col,u0,v0);
    artwork_vertex(q[n++],x+w,y+h,col,u1,v1);
    artwork_vertex(q[n++],x,y+h,col,u0,v1);
}
void add_frame(float cx,float cy,float outer){
    int n=0;quad_tex(g_frameQuads,n,cx-outer,cy-outer,outer*2,outer*2,
                    0xffffffffu,0,0,1,1);
}
void add_art_icon_scaled(float x,float y,unsigned id,float selected,float outer,float relativeSize){
    if(!g_iconAtlas||g_artVertexCount+6>8*6)return;
    if(id>=32)id=rw14::Misc;
    const int col=id%8,row=id/8;
    // Inset texels within independent 256px cells; linear filtering cannot bleed neighbors.
    constexpr float pad=2.f;
    const float u0=(col*256.f+pad)/2048.f,v0=(row*256.f+pad)/1024.f;
    const float u1=((col+1)*256.f-pad)/2048.f,v1=((row+1)*256.f-pad)/1024.f;
    float size=outer*.398f; if(size>181.f)size=181.f;
    size*=relativeSize*(float)g_iconScalePercent/100.f*(1.f+.055f*selected);
    quad_tex(g_artQuads,g_artVertexCount,x-size*.5f,y-size*.5f,size,size,
              0xffffffffu,u0,v0,u1,v1);
}
void add_art_icon(float x,float y,unsigned id,float selected,float outer){
    add_art_icon_scaled(x,y,id,selected,outer,1.f);
}
// The early main-medallion pass MUST NOT carry a miniature: background quads are
// drawn later and used to cover it. The miniatures live in their own late pass.
void add_mini_art_icon(float x,float y,unsigned id,float outer,bool selected){
    if(!g_iconAtlas || g_miniArtVert+6>8*6)return;
    if(id>=rw14::Count)id=rw14::Misc;
    const int col=(int)id%8,row=(int)id/8;
    constexpr float pad=2.f;
    const float u0=(col*256.f+pad)/2048.f,v0=(row*256.f+pad)/1024.f;
    const float u1=((col+1)*256.f-pad)/2048.f,v1=((row+1)*256.f-pad)/1024.f;
    float size=outer*(selected?.177f:.156f);
    if(size>78.f)size=78.f;
    size*=((float)g_iconScalePercent/100.f);
    quad_tex(g_miniArtQuads,g_miniArtVert,x-size*.5f,y-size*.5f,size,size,
             0xffffffffu,u0,v0,u1,v1);
    ++g_miniCount;
}
// Unicode glyph atlas with 46px font, 64px slots, all printable ASCII, Latin-1
// accents and common Western typography. Codes mapped to exact cell indexes.
unsigned font_index(unsigned code){
    for(unsigned i=0;i<kProGlyphCount;i++)if(kProCodepoints[i]==code)return i;
    for(unsigned i=0;i<kProGlyphCount;i++)if(kProCodepoints[i]==0x3F)return i;
    return 0;
}
void font_character(float x,float y,unsigned index,float scale,DWord color){
    if(index>=kProGlyphCount||g_fontVertexCount+12>6*1400)return;
    const float u0=(float)(index%16u)/16.f,v0=(float)(index/16u)/16.f;
    const float u1=u0+1.f/16.f,v1=v0+1.f/16.f;
    float size=64.f*scale;
    quad_tex(g_fontQuads,g_fontVertexCount,x+1.4f,y+1.6f,size,size,0xb2000000u,u0,v0,u1,v1);
    quad_tex(g_fontQuads,g_fontVertexCount,x,y,size,size,color,u0,v0,u1,v1);
}
// Pixel-width-aware fitting: never split a multibyte UTF-8 codepoint.
void professional_text(float midX,float y,float width,float fontPx,const char* raw,DWord col){
    if(!raw||!raw[0]||!g_fontAtlas)return;
    unsigned symbols[72];unsigned count=0;
    const unsigned char* p=(const unsigned char*)raw;
    unsigned bytes=0;
    while(p[bytes] && bytes<rw12ui::NameBytes-1 && count<65){
        unsigned consumed=1,cp=rw12ui::utf8_decode(p+bytes,rw12ui::NameBytes-1-bytes,consumed);
        if(!cp)break;
        symbols[count++]=font_index(cp);bytes+=consumed;
    }
    const float scale=fontPx*(float)(g_khaosFontOverride?100:g_textScalePercent)/(46.f*100.f);
    float full=0;for(unsigned i=0;i<count;i++)full+=(float)kProAdvances[symbols[i]]*scale;
    if(full>width){
      while(count>2&&full+(float)kProAdvances[font_index(0x2026)]*scale>width){
        full-=(float)kProAdvances[symbols[--count]]*scale;
      }
      symbols[count++]=font_index(0x2026);full+=(float)kProAdvances[symbols[count-1]]*scale;
    }
    float x=midX-full*.5f;
    for(unsigned i=0;i<count;i++){
      font_character(x,y,symbols[i],scale,col);
      x+=(float)kProAdvances[symbols[i]]*scale;
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
    g_labelVert=0;g_artVertexCount=0;g_miniArtVert=0;g_miniCount=0;g_fontVertexCount=0;
    const float d=width<height?width:height;
    float outer=d*.31f*(float)g_cfg.wheelScale/100.f;
    if(outer>440.f)outer=440.f; if(outer>height*.44f)outer=height*.44f;
    const float lift=height*.045f<40.f?height*.045f:40.f;
    const float cx=wheel_cx(width),cy=wheel_cy(height);(void)lift;
    add_frame(cx,cy,outer);
    {
        const float y=cy+outer+10.f;
        const float plateW=outer*1.18f;
        const float plateH=80.f;
        int n=0;quad_tex(g_plaqueQuads,n,cx-plateW*.5f,y-18.f,plateW,plateH,
            0xffffffffu,0.f,0.f,1.f,1.f);
    }
    const int wheel=rw02::clamp((int)g_activeWheel,0,g_pageCount-1), selected=rw02::clamp((int)g_select,0,g_sectorCount-1);
    draw_wheel_pager(cx,cy,outer,wheel);
    if(g_showWheelTitles && g_wheelTitles[wheel][0]){
        const float desiredY=cy-outer-58.f;
        const float y=desiredY<14.f?14.f:desiredY; // stronger top plaque with room for subtitle
        const float plateW=outer*.78f<288.f?288.f:outer*.78f;
        const float plateH=g_showWheelOccupancy?46.f:34.f;
        mark_quad(cx-plateW*.5f,y,plateW,plateH,tinted(0xee0c171fu));
        mark_quad(cx-plateW*.5f,y,plateW,2.f,tinted(0xffd4ad73u));
        mark_quad(cx-plateW*.5f,y+plateH-2.f,plateW,2.f,tinted(0xa6a48763u));
        if(g_fontAtlas)professional_text(cx,y-4.f,plateW-24.f,25.f,g_wheelTitles[wheel],0xfffae9c8u);
        else draw_text(cx-60.f,y+7.f,1.75f,g_wheelTitles[wheel],0xfffae9c8u);
        if(g_showWheelOccupancy && g_fontAtlas){
            const unsigned entries=rw21::count_entries(g_model,(unsigned)wheel);
            const unsigned used=rw21::count_occupied(g_model,(unsigned)wheel);
            char stat[]="00 ITEMS  /  0 SLOTS";
            stat[0]=(char)('0'+((entries/10)%10));stat[1]=(char)('0'+entries%10);
            stat[13]=(char)('0'+used);
            professional_text(cx,y+21.f,plateW-18.f,13.4f,stat,0xffd0c2a5u);
        }
    }
    // Both helper panels are now Inventory/Magic-only and separately hideable.
    const bool editContext=(InterlockedCompareExchange(&g_menuFlags,0,0)&3)!=0;
    if(editContext && g_showWheelControlsGuide)draw_controls_panel(width,height,cx,cy,outer);
    const bool guideVisible=editContext && g_showEditorHelp && g_enableAdvancedEditor;
    if(guideVisible){
        const rw22::GuideBox guide=rw22::guide_box(width,height,cx,cy,outer,true);
        if(guide.visible){
            const float px=guide.x,yy=guide.y,pw=guide.w,ph=guide.h;
            const float k=guide.scale;
            const float inset=14.f*k;
            const float rowTop=63.f*k;
            const float rowStep=31.f*k;
            const float keyW=pw*.46f;
            const float keyCenter=px+keyW*.5f;
            const float actionCenter=px+keyW+(pw-keyW)*.5f;
            mark_quad(px,yy,pw,ph,tinted(0xf20b1520u));
            mark_quad(px,yy,pw,3.f*k,tinted(0xffdbba7du));
            mark_quad(px,yy+ph-2.f*k,pw,2.f*k,tinted(0xff997c53u));
            mark_quad(px+inset,yy+46.f*k,pw-2.f*inset,1.5f*k,tinted(0xa7ae8c55u));
            mark_quad(px+keyW,yy+rowTop-6.f*k,1.f*k,13.f*rowStep+4.f*k,tinted(0x638d7651u));
            if(g_fontAtlas){
                professional_text(px+pw*.5f,yy+6.f*k,pw-24.f*k,25.f*k,"ADVANCED EDITOR",0xfff6deb0u);
                professional_text(px+pw*.5f,yy+35.f*k,pw-22.f*k,12.8f*k,"INVENTORY / MAGIC ONLY",0xffc8ae86u);
            }else draw_text(px+inset,yy+16.f*k,2.f*k,"EDITOR",0xfff6deb0u);
            static const char* const keys[]={
                "LMB / RMB", "SHIFT + LMB", "CTRL + LMB", "SHIFT+ALT+LMB",
                "SHIFT+CTRL+LMB", "ALT + LMB", "SHIFT + RMB", "CTRL + RMB",
                "ALT + RMB X2", "ALT + WHEEL", "CTRL+ALT+Q/E",
                "SHIFT+ALT+Q/E", "S+ALT+RMB X2"};
            static const char* const keysCompact[]={
                "LMB/RMB", "SHIFT LMB", "CTRL LMB", "S+ALT LMB", "S+CTRL LMB",
                "ALT LMB", "SHIFT RMB", "CTRL RMB", "ALT RMB X2", "ALT WHEEL",
                "C+ALT Q/E", "S+ALT Q/E", "S+ALT RMB X2"};
            static const char* const actions[]={
                "ADD / REMOVE", "MOVE OBJECT", "COPY OBJECT", "SWAP SECTORS",
                "CLONE SECTOR", "REORDER OBJECT", "UNDO", "REDO",
                "CLEAR SECTOR", "REORDER STACK", "CLONE PAGE", "SWAP PAGES",
                "CLEAR PAGE"};
            for(unsigned i=0;i<13u;++i){
                const float top=yy+rowTop+(float)i*rowStep;
                if(i%2==0)mark_quad(px+4.f*k,top-5.f*k,pw-8.f*k,29.f*k,tinted(0x48364b55u));
                const char* key=guide.compact?keysCompact[i]:keys[i];
                if(g_fontAtlas){
                    professional_text(keyCenter,top-9.f*k,keyW-18.f*k,17.1f*k,key,0xfff2c988u);
                    professional_text(actionCenter,top-9.f*k,pw-keyW-14.f*k,18.5f*k,actions[i],0xfff8ecdbu);
                }else{
                    draw_text(px+8.f*k,top+3.f*k,1.35f*k,key,0xfff2c988u);
                }
            }
            const int mode=(int)InterlockedCompareExchange(&g_editorDisplay,0,0);
            const char* status=mode==10?"MOVE: SELECT TARGET":mode==11?"COPY: SELECT TARGET":
                mode==20?"SWAP: SELECT TARGET":mode==23?"CLONE: EMPTY TARGET":
                mode==17?"CLEAR: REPEAT ALT RMB":mode==26?"CLEAR PAGE: REPEAT":nullptr;
            if(status){
                mark_quad(px+inset,yy+ph-40.f*k,pw-2.f*inset,29.f*k,tinted(0xc274482du));
                if(g_fontAtlas)professional_text(px+pw*.5f,yy+ph-44.f*k,pw-28.f*k,16.2f*k,status,0xffffe2a5u);
            }else if(g_fontAtlas){
                professional_text(px+pw*.5f,yy+ph-42.f*k,pw-24.f*k,13.4f*k,
                    guide.compact?"S=SHIFT  C=CTRL":"MIDDLE CLICK CLOSES WHEEL",0xffb4c8c6u);
            }
            if(InterlockedCompareExchange(&g_guideEditorLogged,1,0)==0)
                file_log("RunicWheelLE: v0.23 LARGE INVENTORY/MAGIC editor guide RENDERED\r\n");
        }
    }
    for(int sector=0;sector<g_sectorCount;++sector){
        const rwl24::Point dir=rwl24::center(g_sectorCount,sector);
        const float ux=dir.x;
        const float uy=dir.y;
        const rw05::Slot& slot=g_model.slot[wheel][sector];
        if(!slot.count || slot.count>rw05::Capacity)continue;
        const unsigned index=g_model.cursor[wheel][sector]<slot.count?g_model.cursor[wheel][sector]:0;
        const rw05::Binding& b=slot.item[index];
        const float x=cx+ux*outer*.66f,y=cy+uy*outer*.66f;
        const bool active=selected==sector;
        const DWord accent=b.kind==rw05::Weapon?0xffffd69au:
          b.kind==rw05::Armor?0xffc0e0ffu:b.kind==rw05::Spell?0xffc9b8ffu:
          b.kind==rw05::Consumable?0xff98edc7u:b.kind==rw05::Ammo?0xffffe4a4u:0xffe2cee8u;
        if(g_magicSchoolAccents && b.kind==rw05::Spell){
            const unsigned id=g_iconIndices[wheel][sector][index];
            const DWord tint=id==rw14::Alteration?0xff89d4ffu:
                id==rw14::Conjuration?0xffc7a4f3u:id==rw14::Destruction?0xffec9352u:
                id==rw14::Illusion?0xff96e5d9u:id==rw14::Restoration?0xffe8ca79u:
                id==rw14::Power?0xffe09a97u:0xffd2bbc6u;
            mark_quad(x-27.f,y+outer*.173f,54.f,2.6f,tinted(tint));
        }
        if(g_iconAtlas && g_firstThemeEnabled){
            add_art_icon(x,y,g_iconIndices[wheel][sector][index],sector_weight(sector),outer);
            if(g_showNextPreview && slot.count>1 && (g_showAllStackPreviews || active)){
                const unsigned next=rw20::preview_index(index,slot.count);
                // Rounded-metal substrate in geometric pass; actual miniature is
                // rendered *after* backgrounds, not underneath them.
                const float off=outer*.15f;
                const float bx=x+off,by=y+off*.80f;
                const float r=outer*(active?.100f:.090f);
                (void)r;
                add_mini_art_icon(bx,by,g_iconIndices[wheel][sector][next],outer,active);
                if(g_showStackPositionDots){
                    for(unsigned dot=0;dot<slot.count;++dot){
                        const float dx=bx-((float)slot.count-1.f)*5.0f+(float)dot*10.f;
                        mark_quad(dx-3.f,by+r+5.f,6.f,3.f,tinted(dot==index?0xffebc47cu:0xff616b75u));
                    }
                }
            }
        }else{
            mark_quad(x-35.f,y-34.f,70.f,67.f,tinted(active?0xc07f664au:0x96404e61u));
            mark_quad(x-32.f,y-31.f,64.f,61.f,tinted(0x9b142333u));
            mark_quad(x-32.f,y+27.f,64.f,3.f,tinted(accent));
            draw_category_icon(x,y-6.f,(rw05::Kind)b.kind,accent);
        }
        // Count is not an additional artwork tile; always retain wheel stack semantics.
        if(slot.count>1){
            // Show cursor/total, not just total; distinct visual proof of scroll.
            char qty[4]={char('1'+index),'/',char('0'+slot.count),0};
            mark_quad(x+30.f,y-57.f,54.f,28.f,tinted(0xe0192837u));
            mark_quad(x+30.f,y-57.f,54.f,2.f,tinted(0xb9b38a52u));
            if(g_fontAtlas)professional_text(x+56.f,y-56.f,52.f,20.f,qty,0xfff9e5aeu);
            else draw_text(x+36.f,y-51.f,2.1f,qty,0xfff9e5aeu);
        }
        const char* label=g_displayNames[wheel][sector][index];
        if(g_showStackHints && active && slot.count>1){
            const float hintY=y-outer*.21f;
            mark_quad(x-29.f,hintY,58.f,2.f,tinted(0x848b9ca9u));
            mark_quad(x-29.f,hintY,58.f*((float)(index+1)/(float)slot.count),2.f,tinted(0xffe7c182u));
        }
        if(g_showNames && g_sectorLabels && label[0]){
            // Small captions are optically centred and fitted by proportional glyph width.
            float yy=y+outer*.210f;
            mark_quad(x-82.f,yy-4.f,164.f,27.f,tinted(0xda101a23u));
            mark_quad(x-69.f,yy-4.f,138.f,1.0f,tinted(0x989e8357u));
            if(g_fontAtlas)professional_text(x,yy-6.f,154.f,17.f,label,0xfff8ebcdu);
            else draw_text(x-39.f,yy,1.45f,label,0xfff6efdfu);
        }
    }
    const rw05::Binding selection=rw05::current(g_model,(unsigned)wheel,(unsigned)selected);
    if(selection.formId){
        const rw05::Slot& picked=g_model.slot[wheel][selected];
        unsigned cursor=g_model.cursor[wheel][selected];
        const char* label=cursor<picked.count?g_displayNames[wheel][selected][cursor]:"";
        if(g_showNames && label[0]){
            // Larger plaque and cleaner line spacing for readability.
            const float y=cy+outer+12.f;
            const float captionWidth=outer*1.22f;
            if(!g_plaqueAtlas){
                mark_quad(cx-captionWidth*.5f,y-9.f,captionWidth,74.f,tinted(0xea111924u));
                mark_quad(cx-captionWidth*.5f,y-9.f,captionWidth,2.f,0xffc5a474u);
            }
            if(g_fontAtlas){
                professional_text(cx,y-8.f,captionWidth-44.f,31.f,label,0xfffef3dfu);
                const unsigned iconId=g_iconIndices[wheel][selected][cursor];
                const char* category=selection.kind==rw05::Weapon?"WEAPON":
                  selection.kind==rw05::Armor?"ARMOR":selection.kind==rw05::Spell?rw14::spell_caption(iconId):
                  selection.kind==rw05::Consumable?"CONSUMABLE":selection.kind==rw05::Ammo?"AMMUNITION":
                  selection.kind==rw05::Shout?"DRAGON SHOUT":selection.kind==rw05::Torch?"TORCH":"ITEM";
                char subtitle[48]{};
                rw19::caption(subtitle,sizeof(subtitle),category,g_model.cursor[wheel][selected],picked.count);
                professional_text(cx,y+24.f,captionWidth-74.f,16.4f,subtitle,0xffc0dbe0u);
                if(g_showNextPreview && picked.count>1 && cursor<picked.count){
                    const unsigned next=rw20::preview_index(cursor,picked.count);
                    const char* nextLabel=g_displayNames[wheel][selected][next];
                    if(nextLabel[0]){
                        char text[80]={'N','E','X','T',':',' ',0};
                        unsigned n=6;
                        for(unsigned j=0;nextLabel[j] && j<rw12ui::NameBytes-1 && n+1<sizeof text;++j)
                            text[n++]=nextLabel[j];
                        text[n]=0;
                        professional_text(cx,y+48.f,captionWidth-70.f,13.5f,text,0xffc7b198u);
                    }
                }
            }else draw_text(cx-92.f,y+1.f,2.9f,label,0xfffff5e4u);
        }else{
            char hex[]="00000000";const char* digits="0123456789ABCDEF";
            for(int i=0;i<8;++i)hex[i]=digits[(selection.formId>>(28-4*i))&15u];
            if(g_fontAtlas)professional_text(cx,cy-18.f,160.f,20.f,hex,0xffd4ebf8u);
            else draw_text(cx-51.f,cy+3.f,2.3f,hex,0xffd4ebf8u);
        }
    }
    if(!selection.formId && g_fontAtlas){
        const float y=cy+outer+12.f;
        professional_text(cx,y-5.f,outer*.96f,22.f,"EMPTY SECTOR",0xffc8b6a1u);
        professional_text(cx,y+21.f,outer*.96f,12.8f,
             (InterlockedCompareExchange(&g_menuFlags,0,0)&3)?"HOVER ITEM AND LEFT CLICK":"CHOOSE AN OCCUPIED SECTOR",0xff8fa7b6u);
    }
    const float feedbackFade=rw16::feedback_alpha(g_presentationNow,(DWord)InterlockedCompareExchange(&g_feedbackTick,0,0),(unsigned)g_cfg.feedbackDuration);
    if(feedbackFade>0.f){
        const int code=(int)InterlockedCompareExchange(&g_feedback,0,0);
        const char* msg=code==1?"ADDED":code==2?"REMOVED":code==3?"EQUIP?":code==4?"REFUSED":code==5?"USE?":code==6?"UNEQUIP?":code==7?"QUEUED":code==8?"EAT N/A":code==9?"NEXT ITEM":
             code==10?"MOVE: TARGET":code==11?"COPY: TARGET":code==12?"MOVED":code==13?"COPIED":
             code==14?"UNDO":code==15?"REDO":code==16?"REORDERED":code==17?"CLEAR? AGAIN":
             code==18?"STACK CLEARED":code==19?"CANCELLED":code==20?"SWAP: TARGET":
             code==21?"SWAPPED":code==22?"SLOT CLONED":code==23?"CLONE: TARGET":
             code==24?"WHEEL CLONED":code==25?"WHEELS SWAPPED":
             code==26?"CLEAR WHEEL? AGAIN":code==27?"WHEEL CLEARED":nullptr;
        char stackMsg[32]{};
        if(code==9){
            const rw05::Slot& picked=g_model.slot[wheel][selected];
            rw19::caption(stackMsg,sizeof(stackMsg),"ITEM",g_model.cursor[wheel][selected],picked.count);
            msg=stackMsg;
        }
        if(msg){
            const float rise=g_reduceMotion?0.f:4.f*(1.f-feedbackFade);
            const float fy=cy+outer*.38f-rise;
            mark_quad(cx-112.f,fy,224.f,50.f,tinted(rw16::with_alpha(0xe0182830u,feedbackFade)));
            mark_quad(cx-87.f,fy+43.f,174.f*feedbackFade,1.8f,tinted(rw16::with_alpha(code==4?0xffba7567u:0xffdfbd78u,feedbackFade)));
            if(g_fontAtlas)professional_text(cx,fy-10.f,204.f,22.f,msg,rw16::with_alpha(code==4?0xffffa493u:0xffaef4c3u,feedbackFade));
            else draw_text(cx-89.f,fy+7.f,3.65f,msg,rw16::with_alpha(0xffaef4c3u,feedbackFade));
        }
    }
}

void apply_to_vertex(Vertex& v,float cx,float cy,float scale,float opacity){
    v.x=cx+(v.x-cx)*scale;
    v.y=cy+(v.y-cy)*scale;
    v.color=rw16::with_alpha(v.color,opacity);
}
void apply_to_vertex(TextureVertex& v,float cx,float cy,float scale,float opacity){
    v.x=cx+(v.x-cx)*scale;
    v.y=cy+(v.y-cy)*scale;
    v.color=rw16::with_alpha(v.color,opacity);
}
void apply_presentation(float width,float height){
    const float cx=wheel_cx(width),cy=wheel_cy(height);
    const float scale=g_presentationFrame.scale,opacity=g_presentationFrame.opacity;
    if(scale==1.f && opacity==1.f)return;
    for(int i=0;i<48;++i)apply_to_vertex(g_geometry[i],cx,cy,scale,opacity);
    // Selection sweep is now capable of drawing two arcs; the bounds match g_pips.
    for(int i=0;i<g_pipsUsed;++i)apply_to_vertex(g_pips[i],cx,cy,scale,opacity);
    for(int i=0;i<g_labelVert;++i)apply_to_vertex(g_labels[i],cx,cy,scale,opacity);
    for(int i=0;i<g_artVertexCount;++i)apply_to_vertex(g_artQuads[i],cx,cy,scale,opacity);
    for(int i=0;i<6;++i){apply_to_vertex(g_frameQuads[i],cx,cy,scale,opacity);
                          apply_to_vertex(g_plaqueQuads[i],cx,cy,scale,opacity);}
    for(int i=0;i<g_fontVertexCount;++i)apply_to_vertex(g_fontQuads[i],cx,cy,scale,opacity);
}
int draw_textured_pass(Raw device,Raw texture,const TextureVertex* quads,UInit triCount){
    if(!texture||!triCount)return 1;
    reinterpret_cast<SetTexture>(get(device,kSetTexture))(device,0,texture);
    reinterpret_cast<SetFVF>(get(device,kFVF))(device,0x144);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,1,4);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,2,2);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,3,0);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,4,4);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,5,2);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,6,0);
    reinterpret_cast<SetSampler>(get(device,kSamplerState))(device,0,5,2);
    reinterpret_cast<SetSampler>(get(device,kSamplerState))(device,0,6,2);
    reinterpret_cast<SetSampler>(get(device,kSamplerState))(device,0,1,3);
    reinterpret_cast<SetSampler>(get(device,kSamplerState))(device,0,2,3);
    return succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))
      (device,4,triCount,quads,sizeof(TextureVertex)));
}
void reset_texture_pass(Raw device){
    reinterpret_cast<SetTexture>(get(device,kSetTexture))(device,0,nullptr);
    reinterpret_cast<SetFVF>(get(device,kFVF))(device,0x44);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,1,2);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,2,0);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,4,2);
    reinterpret_cast<SetTextureStage>(get(device,kTextureStage))(device,0,5,0);
}
int draw(Raw device){
    if(!device)return 0;
    g_presentationNow=GetTickCount();
    const bool visible=InterlockedCompareExchange(&g_visible,0,0)!=0;
    const bool closing=InterlockedCompareExchange(&g_exitArmed,0,0)!=0;
    g_presentationFrame=rw16::frame(visible,closing,g_motionEnabled!=0,g_reduceMotion!=0,
        g_presentationNow,(DWord)InterlockedCompareExchange(&g_openTick,0,0),
        (DWord)InterlockedCompareExchange(&g_closeTick,0,0),
        (unsigned)g_openDuration,(unsigned)g_closeDuration);
    if(!g_presentationFrame.draw){
        if(!visible)InterlockedExchange(&g_exitArmed,0);
        return 0;
    }
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
    (void)load_theme(device); // Fail-safe: missing assets retain V0.12.1 pixel symbols.
    AcquireSRWLockShared(&g_catalogLock);
    __try {
        update_visual_focus(g_presentationNow,rw02::clamp((int)g_activeWheel,0,g_pageCount-1),
                            rw02::clamp((int)g_select,0,g_sectorCount-1),
                            (DWord)InterlockedCompareExchange(&g_openTick,0,0));
        make_mesh((float)vp.w,(float)vp.h);
        triangles=make_pips((float)vp.w,(float)vp.h);
        draw_labels((float)vp.w,(float)vp.h);
        apply_presentation((float)vp.w,(float)vp.h);
    } __finally {ReleaseSRWLockShared(&g_catalogLock);}
    ok=succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))(device,4,(UInit)(g_sectorCount*2),g_geometry,sizeof(Vertex)));
    if(ok && triangles>0)ok=succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))(device,4,(UInit)triangles,g_pips,sizeof(Vertex)));
    // Draw order: translucent sectors -> ornamental frame -> independent icons
    // -> geometrical caption backgrounds -> Unicode text. All state restored.
    if(ok && g_frameAtlas && g_showDecorativeFrame)ok=draw_textured_pass(device,g_frameAtlas,g_frameQuads,2);
    if(ok && g_firstThemeEnabled && g_iconAtlas && g_artVertexCount)ok=draw_textured_pass(device,g_iconAtlas,g_artQuads,(UInit)(g_artVertexCount/3));
    if(ok && g_plaqueAtlas)ok=draw_textured_pass(device,g_plaqueAtlas,g_plaqueQuads,2);
    reset_texture_pass(device);
    if(ok && g_labelVert>0)ok=succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))(device,4,(UInit)(g_labelVert/3),g_labels,sizeof(Vertex)));
    // Mini badges must overlay their near-opaque background, not be painted under it.
    if(ok && g_firstThemeEnabled && g_iconAtlas && g_miniArtVert){
        ok=draw_textured_pass(device,g_iconAtlas,g_miniArtQuads,(UInit)(g_miniArtVert/3));
        if(ok && InterlockedCompareExchange(&g_previewLogged,1,0)==0)
            file_log("RunicWheelLE: v0.22 stack miniature LATE PASS rendered\r\n");
    }
    if(ok && g_fontAtlas && g_fontVertexCount)ok=draw_textured_pass(device,g_fontAtlas,g_fontQuads,(UInit)(g_fontVertexCount/3));
    reset_texture_pass(device);

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
    InterlockedExchange(&g_exitArmed,0);
    InterlockedExchange(&g_resetInput,1);
    return 1;
}
void guarded_frame(Raw device){
    if(g_faults>=3)return;
    if(InterlockedCompareExchange(&g_drawBusy,1,0)!=0)return;
    __try {
        if(InterlockedExchange(&g_pendingReset,0)) {
            InterlockedExchange(&g_visible,0);
            InterlockedExchange(&g_exitArmed,0);
            InterlockedExchange(&g_resetInput,1);
            file_log("RunicWheelLE: reset on save/load\r\n");
        }
        if(!g_firstFrame){
            InterlockedExchange(&g_firstFrame,1);
            format_thread_log("RunicWheelLE: first Present thread ");
        }
        if(g_window && GetForegroundWindow()!=g_window && (g_visible || g_exitArmed)) {
            InterlockedExchange(&g_visible,0);
            InterlockedExchange(&g_exitArmed,0);
            InterlockedExchange(&g_resetInput,1);
        }
        if(g_visible || g_exitArmed) {
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
#include "RunicKhaosMenu.inl"
} // namespace

// KHA-OS v0.5L API v2, optional host: plugin works normally without KHAOS.dll.
KHAOS_ABI bool KHAOS_Query(KHAOS_PluginInfo* info){
    if(!info || info->structSize<sizeof(KHAOS_PluginInfo) || info->apiVersion<KHAOS_API_VERSION_MIN)return false;
    info->apiVersion=KHAOS_API_VERSION;info->structSize=sizeof(KHAOS_PluginInfo);info->flags=0;
    info->pluginId="runicwheel.le";info->displayName="Runic Wheel LE";
    info->category="Interface";info->dllName="RunicWheelLE.dll";
    info->originalHotkey="Middle mouse button";
    info->description="Highly customizable Nordic equipment wheel, pages, input and appearance";
    return true;
}
KHAOS_ABI void KHAOS_SetHosted(bool hosted){
    InterlockedExchange(&g_khaosHosted,hosted?1:0);
    if(!hosted)InterlockedExchange(&g_khaosOpen,0);
    file_log(hosted?"RunicWheelLE: KHAOS host attached\r\n":"RunicWheelLE: KHAOS host detached\r\n");
}
KHAOS_ABI void KHAOS_ToggleMenu(){
    if(!g_khaosReady)return;
    if(g_khaosOpen){InterlockedExchange(&g_khaosOpen,0);file_log("RunicWheelLE: KHAOS menu closed\r\n");return;}
    khaos_begin_session();
    InterlockedExchange(&g_khaosOpen,1);
    InterlockedExchange(&g_visible,0);InterlockedExchange(&g_exitArmed,0);
    InterlockedExchange(&g_resetInput,1);schedule_time_reconcile();
    file_log("RunicWheelLE: KHAOS menu opened\r\n");
}
KHAOS_ABI bool KHAOS_IsMenuOpen(){return g_khaosOpen!=0;}
KHAOS_ABI void KHAOS_RenderFrame(void* hwnd,void* device,int width,int height){
    if(!g_khaosOpen || !g_khaosReady || !device || !hwnd || width<640 || height<480)return;
    if(g_drawBusy || InterlockedCompareExchange(&g_drawBusy,1,0)!=0)return;
    g_khaosWidth=width;g_khaosHeight=height;
    __try{(void)khaos_draw_frame(device,width,height);}
    __except(1){
       InterlockedExchange(&g_khaosOpen,0);file_log("RunicWheelLE: KHAOS UI drawing fault; menu disabled\r\n");
    }
    InterlockedExchange(&g_drawBusy,0);
}
KHAOS_ABI bool KHAOS_ProcessWndProc(void* hwnd,unsigned int msg,uintptr_t wp,intptr_t lp){
    if(!g_khaosOpen || !g_khaosReady)return false;
    if(rw25::lost_focus(msg,(unsigned)wp)){
        InterlockedExchange(&g_khaosOpen,0);
        file_log("RunicWheelLE: KHAOS UI yielded on focus loss\r\n");
        return false;
    }
    return hwnd && khaos_ui_event(msg,wp,lp);
}

extern "C" __declspec(dllexport) bool __cdecl SKSEPlugin_Query(const SKSEInterface* skse,PluginInfo* info){
    if(!skse||!info)return false;
    info->infoVersion=PluginInfo::kInfoVersion;info->name="Runic Wheel LE";info->version=125;
    if(skse->isEditor||skse->runtimeVersion!=RUNTIME_VERSION_1_9_32_0||!skse->GetPluginHandle)return false;
    begin_log(); file_log("RunicWheelLE: v0.25.0 Query entered\r\n");
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
    load_config();file_log("RunicWheelLE: v0.25 KHA-OS focus-safe movable configuration and adaptive layout; MSVC x86\r\n");
    auto* serial=(SKSESerializationInterface*)skse->QueryInterface(kInterface_Serialization);
    if(!serial||serial->version<SKSESerializationInterface::kVersion||!serial->SetUniqueID||
        !serial->SetSaveCallback||!serial->SetLoadCallback||!serial->SetRevertCallback)return false;
    serial->SetUniqueID(g_handle,kSaveUnique);
    serial->SetSaveCallback(g_handle,on_save);
    serial->SetLoadCallback(g_handle,on_load);
    serial->SetRevertCallback(g_handle,on_revert);
    if(!messaging->RegisterListener(g_handle,"SKSE",messages)){file_log("RunicWheelLE: SKSE RegisterListener failed\r\n");return false;}
    file_log("RunicWheelLE: SKSE lifecycle registration succeeded\r\n");
    InterlockedExchange(&g_khaosReady,1);
    return true;
}
