// KHA-OS v0.5L plugin-owned D3D9 configuration UI, no ImGui ABI/shared context.
// Include inside RunicWheelV011.cpp anonymous namespace, after render helpers.
// Only the existing RunicWheelLE.ini [Controls] is authoritative; co-save untouched.

static volatile Long g_khaosHosted=0,g_khaosReady=0;
static int g_khaosTab=0,g_khaosScroll=0,g_khaosFocus=-1,g_khaosDirty=0;
static int g_khaosTitleEdit=-1,g_khaosStatus=0;
static int g_khaosWidth=1280,g_khaosHeight=720;
// Persistent move offset during a play session, plus explicit drag lifetime.
static float g_khaosOffsetX=0.f,g_khaosOffsetY=0.f;
static float g_khaosGrabX=0.f,g_khaosGrabY=0.f;
static int g_khaosDragging=0;
static char g_khaosTitles[3][24]{};
static int g_khaosThemeGeneration=-1;
static float g_khaosUIScale=1.f;
static int g_khaosProfileNotice=0,g_khaosProfileAction=0;
static DWord g_khaosThemePanel=0xf1121c27u;
// v0.26 pointer fallback. Another overlay's ImGui backend (Aether Reloaded) can hide the
// Windows pointer through WM_SETCURSOR while this panel is open; the panel then draws
// its own. 0=never, 1=only while the Windows pointer is hidden, 2=always.
static int g_khaosCursorMode=1;
static float g_khaosMouseX=-1.f,g_khaosMouseY=-1.f;
static int g_khaosSeedPointer=0;
static Handle g_khaosHwnd=nullptr;
static Vertex g_khaosPointer[2*3*5];
// Window client coordinates -> back-buffer pixels, the space the panel is drawn in.
void khaos_point(intptr_t lp,float& x,float& y){
    x=(float)(short)((unsigned long)lp&0xffffu);
    y=(float)(short)(((unsigned long)lp>>16)&0xffffu);
    Rectangle client{};
    Handle wnd=g_khaosHwnd?g_khaosHwnd:g_window;
    if(wnd && GetClientRect(wnd,&client)){
        const rw26ui::Mapping m=rw26ui::client_to_buffer((float)(client.r-client.l),(float)(client.b-client.t),
                                                         (float)g_khaosWidth,(float)g_khaosHeight);
        x*=m.sx;y*=m.sy;
    }
}
void khaos_seed_pointer(){
    Long p[2]={0,0};
    Handle wnd=g_khaosHwnd?g_khaosHwnd:g_window;
    if(!wnd || !GetCursorPos(p) || !ScreenToClient(wnd,p))return;
    const intptr_t lp=(intptr_t)(((unsigned long)(p[1]&0xffff)<<16)|((unsigned long)p[0]&0xffffu));
    khaos_point(lp,g_khaosMouseX,g_khaosMouseY);
}
bool khaos_pointer_wanted(){
    if(g_khaosCursorMode<=0 || g_khaosMouseX<0.f || g_khaosMouseY<0.f)return false;
    if(g_khaosCursorMode>=2)return true;
    CursorInfo info{};info.size=sizeof(info);
    if(!GetCursorInfo(&info))return true; // unknown: a second pointer beats none
    return !rw26ui::os_pointer_visible(info.flags,info.cursor!=nullptr);
}
int khaos_build_pointer(float x,float y,float scale){
    // Original arrow, drawn twice: a dark outline then a light fill (5 triangles each).
    static const float shape[5][6]={
        {0,0, 0,17, 4.5f,13}, {0,0, 4.5f,13, 6.5f,12.5f}, {0,0, 6.5f,12.5f, 12,12},
        {4.5f,13, 7.5f,20, 9.8f,19}, {4.5f,13, 9.8f,19, 6.5f,12.5f}};
    int n=0;
    for(int layer=0;layer<2;++layer){
        const float grow=layer==0?1.28f:1.f;
        const float shift=layer==0?-1.3f*scale:0.f;
        const DWord color=layer==0?0xf0101010u:0xfff4ead2u;
        for(int t=0;t<5;++t)for(int k=0;k<3;++k)
            vertex(g_khaosPointer[n++],x+shift+shape[t][k*2]*scale*grow,y+shift+shape[t][k*2+1]*scale*grow,color);
    }
    return n;
}
void khaos_update_theme(){
    Handle dll=nullptr;
    if(!GetModuleHandleExA(2,"KHAOS.dll",&dll)||!dll)return;
    auto fn=(KHAOS_UI_GetThemeSnapshotFn)GetProcAddress(dll,"KHAOS_UI_GetThemeSnapshot");
    auto gen=(KHAOS_UI_GetThemeGenerationFn)GetProcAddress(dll,"KHAOS_UI_GetThemeGeneration");
    if(!fn||!gen)return;
    int current=(int)gen();if(current==g_khaosThemeGeneration)return;
    KHAOS_UI_ThemeSnapshot snap{};snap.apiVersion=KHAOS_UI_THEME_API_VERSION;
    snap.structSize=sizeof(snap);
    if(!fn(&snap)||snap.apiVersion!=KHAOS_UI_THEME_API_VERSION||
       snap.colorCount<=KHAOS_UI_COLOR_WINDOW_BG)return;
    const float* c=snap.colors[KHAOS_UI_COLOR_WINDOW_BG];
    int rgb[3]={};for(int i=0;i<3;++i){float v=c[i];
       if(v<0.f)v=0.f;if(v>1.f)v=1.f;rgb[i]=(int)(v*255.f);}
    g_khaosThemePanel=0xf0000000u|((DWord)rgb[0]<<16)|((DWord)rgb[1]<<8)|(DWord)rgb[2];
    g_khaosUIScale=snap.uiScale<0.8f?0.8f:snap.uiScale>1.25f?1.25f:snap.uiScale;
    g_khaosThemeGeneration=current;
}


struct KhaosField {const char* key;const char* label;const char* help; int tab,min,max,step,initial; int* target;};
static KhaosField g_khaosFields[]={
 // LAYOUT
 {"VisiblePageCount","VISIBLE PAGES","1-3 visible pages. Hidden page items are preserved.",0,1,3,1,3,&g_pageCount},
 {"VisibleSectorCount","SECTORS PER PAGE","4-8 radial sectors. Hidden sectors retain their saved contents.",0,4,8,1,8,&g_sectorCount},
 {"WheelScalePercent","WHEEL SIZE","Overall radial wheel diameter.",0,75,170,5,130,&g_cfg.wheelScale},
 {"WheelOffsetXPercent","HORIZONTAL POSITION","Move the wheel left or right in screen percent.",0,-35,35,2,0,&g_offsetX},
 {"WheelOffsetYPercent","VERTICAL POSITION","Move the wheel up or down in screen percent.",0,-30,30,2,0,&g_offsetY},
 {"MouseDeadzone","INNER DEADZONE","Mouse distance required before selecting a sector.",0,0,120,2,22,&g_cfg.deadzone},
 {"MouseSensitivityPercent","CURSOR SENSITIVITY","Adjust the wheel mouse motion response.",0,20,350,10,100,&g_cfg.sensitivity},
 // ART
 {"ShowConceptIcons","NORDIC ARTWORK","Use the embedded Heraldry medallions and ornate frame.",1,0,1,1,1,&g_firstThemeEnabled},
 {"ShowDecorativeFrame","ENGRAVED FRAME","Optional ornate rim. For alternate sector counts, try OFF.",1,0,1,1,1,&g_showDecorativeFrame},
 {"IconSizePercent","MEDALLION SIZE","Scale individual primary and mini icons.",1,65,145,5,100,&g_iconScalePercent},
 {"WheelOpacityPercent","WHEEL OPACITY","Control the transparency of the wheel.",1,25,100,5,70,&g_cfg.wheelOpacity},
 {"BackgroundStrengthPercent","BACKGROUND STRENGTH","Opacity of dark backing panels.",1,30,100,5,88,&g_backgroundStrength},
 {"AccentRed","ACCENT RED","Custom highlight colour: red channel.",1,0,255,5,219,&g_visualAccentR},
 {"AccentGreen","ACCENT GREEN","Custom highlight colour: green channel.",1,0,255,5,186,&g_visualAccentG},
 {"AccentBlue","ACCENT BLUE","Custom highlight colour: blue channel.",1,0,255,5,125,&g_visualAccentB},
 // LABELS
 {"ShowItemNames","ITEM NAMES","Display the selected item's real cached name.",2,0,1,1,1,&g_showNames},
 {"ShowSectorLabels","SECTOR CAPTIONS","Show labels near each sector medallion.",2,0,1,1,0,&g_sectorLabels},
 {"TextScalePercent","TEXT SIZE","Scale font for the radial wheel and editor guide.",2,65,150,5,100,&g_textScalePercent},
 {"ShowWheelTitles","PAGE TITLES","Display each page name above the wheel.",2,0,1,1,1,&g_showWheelTitles},
 {"ShowWheelOccupancy","PAGE CAPACITY","Show number of occupied slots per page.",2,0,1,1,1,&g_showWheelOccupancy},
 {"ShowStackHints","STACK PROGRESS","Display progress indicator on the selected stack.",2,0,1,1,1,&g_showStackHints},
 // STACKS
 {"ShowNextItemPreview","NEXT ITEM MEDALLION","Draw the medallion of the next stack item.",3,0,1,1,1,&g_showNextPreview},
 {"ShowAllStackPreviews","ALL STACK PREVIEWS","Show mini-previews in every stacked sector.",3,0,1,1,1,&g_showAllStackPreviews},
 {"ShowStackPositionDots","STACK POSITION DOTS","Show individual gold stack progress markers.",3,0,1,1,1,&g_showStackPositionDots},
 {"ReverseItemScroll","REVERSE MOUSE WHEEL","Change scroll wheel direction for a stack.",3,0,1,1,0,&g_reverseItemScroll},
 {"CycleEntryUpMouseCode","SCROLL UP INPUT","DirectInput mouse button code for wheel up.",3,5,12,1,8,&g_cfg.wheelUp},
 {"CycleEntryDownMouseCode","SCROLL DOWN INPUT","DirectInput mouse button code for wheel down.",3,5,12,1,9,&g_cfg.wheelDown},
 // MOTION
 {"EnablePresentationAnimations","ENABLE MOTION","Master switch for animations.",4,0,1,1,1,&g_motionEnabled},
 {"ReduceMotion","REDUCED MOTION","Accessibility: remove animated scaling/sweep.",4,0,1,1,0,&g_reduceMotion},
 {"OpenAnimationMs","OPENING DURATION","Open transition in milliseconds.",4,0,400,10,160,&g_openDuration},
 {"CloseAnimationMs","CLOSING DURATION","Close transition in milliseconds.",4,0,300,10,110,&g_closeDuration},
 {"SectorTransitionMs","SECTOR CROSSFADE","Selection highlight animation duration.",4,0,250,10,125,&g_sectorDuration},
 {"ClickFeedbackMs","ACTION FEEDBACK TIME","Milliseconds that action confirmation remains visible.",4,250,2000,50,900,&g_cfg.feedbackDuration},
 // INPUT
 {"OpenMouseButton","OPEN MOUSE BUTTON","DirectInput: middle mouse = 2.",5,0,4,1,2,&g_cfg.openMouse},
 {"PreviousWheelMouseButton","PREVIOUS PAGE MOUSE","Mouse side button 3 is the default.",5,0,4,1,3,&g_cfg.prevMouse},
 {"NextWheelMouseButton","NEXT PAGE MOUSE","Mouse side button 4 is the default.",5,0,4,1,4,&g_cfg.nextMouse},
 {"OpenKeyboardScanCode","OPEN KEY SCANCODE","0 disables; physical DirectInput keyboard scan code.",5,0,255,1,0,&g_cfg.keyboardOpen},
 {"PreviousWheelScanCode","PREVIOUS PAGE KEY","Physical scan 16 = Q on QWERTY / A on AZERTY.",5,0,255,1,16,&g_cfg.previousKey},
 {"NextWheelScanCode","NEXT PAGE KEY","Physical scan 18 = E, independent of keyboard layout.",5,0,255,1,18,&g_cfg.nextKey},
 {"CloseKeyboardScanCode","CLOSE KEY SCANCODE","Physical scan 1 = Escape.",5,0,255,1,1,&g_cfg.closeKey},
 {"PreviousItemScanCode","PREVIOUS STACK KEY","Optional physical key. 0=disabled.",5,0,255,1,0,&g_itemPreviousKey},
 {"NextItemScanCode","NEXT STACK KEY","Optional physical key. 0=disabled.",5,0,255,1,0,&g_itemNextKey},
 {"PreviousOccupiedSectorScanCode","PREVIOUS OCCUPIED KEY","Skip empty sectors. 0=disabled.",5,0,255,1,0,&g_skipPreviousKey},
 {"NextOccupiedSectorScanCode","NEXT OCCUPIED KEY","Skip empty sectors. 0=disabled.",5,0,255,1,0,&g_skipNextKey},
 {"KeyboardLayoutMode","KEYBOARD LAYOUT","2=physical AZERTY/QWERTY; 3=legacy translated Q/E.",5,0,3,1,2,&g_cfg.keyboardMode},
 {"LeftClickEquipsRightHand","LEFT CLICK: RIGHT HAND","Left/right in-game equip mapping.",5,0,1,1,1,&g_cfg.leftClickRightHand},
 {"HoldThresholdMs","HOLD THRESHOLD","Tap/hold delay for wheel activation.",5,80,1200,20,260,&g_cfg.holdMs},
 {"ConsumeInputWhileOpen","BLOCK GAME INPUT","Prevent click-through while radial wheel is active.",5,0,1,1,1,&g_cfg.suppress},
 {"EnableQuickSectorKeys","QUICK SECTOR KEYS","Optional physical number keys 1-8.",5,0,1,1,0,&g_quickSectorKeys},
 {"EnableGamepad","GAMEPAD ENABLED","Allow controller input on the radial wheel.",5,0,1,1,1,&g_cfg.gamepad},
 {"GamepadDeadzonePercent","GAMEPAD DEADZONE","Percent deadzone for stick aiming.",5,0,90,5,22,&g_gamepadDeadzonePercent},
 // ENGINE
 {"SlowTimePercent","TIME SPEED","World time percentage while the wheel is open.",6,1,100,5,10,&g_slowPercent},
 {"EnableNativeActions","NATIVE EQUIPMENT","Master gate for native equip/unequip/consumables.",6,0,1,1,1,&g_cfg.nativeActions},
 {"EnableSpellActions","NATIVE SPELLS","Allow verified EquipSpell game-thread dispatch.",6,0,1,1,1,&g_enableSpellActions},
 {"EnableSafeInventoryUITask","INVENTORY CAPTURE","Use verified read-only SKSE UI-task selected item lookup.",6,0,1,1,1,&g_hoverCaptureEnabled},
 {"ShowMagicSchoolAccents","MAGIC SCHOOL COLOURS","Different school accents per spell medallion.",6,0,1,1,1,&g_magicSchoolAccents},
 {"EnableInputDiagnostics","INPUT DIAGNOSTICS","Verbose input and stack event logging.",6,0,1,1,0,&g_cfg.inputDiagnostics},
 // EDITOR
 {"EnableAdvancedEditorGestures","ADVANCED EDITOR","Enable move, swap, clone, clear and undo gestures.",7,0,1,1,1,&g_enableAdvancedEditor},
 {"ShowAdvancedEditorGuide","EDITOR HELP PANEL","Only show guide in Inventory/Magic contexts.",7,0,1,1,1,&g_showEditorHelp},
 {"ShowWheelControlsPanel","WHEEL CONTROLS PANEL","Show the larger controls guide in Inventory/Magic contexts.",7,0,1,1,1,&g_showWheelControlsGuide},
};
static constexpr int kKhaosFieldCount=(int)(sizeof(g_khaosFields)/sizeof(g_khaosFields[0]));
static int g_khaosDraft[kKhaosFieldCount]{};
static const DWord g_khaosPalettes[6]={0xffdbba7du,0xff7bbdd9u,0xffe18b62u,0xffb28cceu,0xff84b59eu,0xffbbc3ceu};
static const char* const g_khaosPaletteNames[6]={"NORDIC","FROST","EMBER","ARCANE","FOREST","SILVER"};
static const char* const g_khaosTabs[8]={"LAYOUT","APPEARANCE","LABELS","STACKS","ANIMATION","INPUT","GAMEPLAY","EDITOR"};

int khaos_field_tab_count(int tab){int n=0;for(int i=0;i<kKhaosFieldCount;++i)if(g_khaosFields[i].tab==tab)++n;return n;}
int khaos_field_index(int tab,int row){
    for(int i=0;i<kKhaosFieldCount;++i)if(g_khaosFields[i].tab==tab && row--==0)return i;
    return -1;
}
void khaos_copy_title(char (&dst)[24],const char* src){int n=0;while(src&&src[n]&&n<23){
    unsigned char c=(unsigned char)src[n];dst[n]=(c>=32&&c<=126)?(char)c:' ';++n;}dst[n]=0;}
void khaos_begin_session(){
    for(int i=0;i<kKhaosFieldCount;++i){g_khaosDraft[i]=*g_khaosFields[i].target;}
    for(int w=0;w<3;++w)khaos_copy_title(g_khaosTitles[w],g_wheelTitles[w]);
    g_khaosDragging=0;g_khaosScroll=0;g_khaosFocus=-1;g_khaosTitleEdit=-1;g_khaosDirty=0;g_khaosStatus=0;g_khaosProfileNotice=0;g_khaosProfileAction=0;
}
void khaos_discard(){khaos_begin_session();g_khaosStatus=2;}
void khaos_adjust(int field,int direction){
    if(field<0||field>=kKhaosFieldCount)return;
    KhaosField& f=g_khaosFields[field];int val=g_khaosDraft[field]+f.step*direction;
    if(val<f.min)val=f.min;if(val>f.max)val=f.max;
    g_khaosDraft[field]=val;g_khaosDirty=1;g_khaosFocus=field;g_khaosStatus=0;g_khaosProfileAction=0;
}
void khaos_int_string(int n,char (&buffer)[24]);
bool khaos_save_profile(int slot){
    if(slot<0||slot>=3 || !g_configPath[0])return false;
    char section[]="RunicProfileA";section[12]=(char)('A'+slot);
    // Profile snapshots live inside canonical RunicWheelLE.ini, never a second store.
    if(!WritePrivateProfileStringA(section,"Version","1",g_configPath))return false;
    for(int i=0;i<kKhaosFieldCount;++i){char text[24]{};khaos_int_string(g_khaosDraft[i],text);
        if(!WritePrivateProfileStringA(section,g_khaosFields[i].key,text,g_configPath))return false;}
    for(int w=0;w<3;++w){char key[]="Wheel1Title";key[5]=(char)('1'+w);
        if(!WritePrivateProfileStringA(section,key,g_khaosTitles[w],g_configPath))return false;}
    g_khaosProfileNotice=slot+1;g_khaosProfileAction=1;return true;
}
bool khaos_load_profile(int slot){
    if(slot<0||slot>=3 || !g_configPath[0])return false;
    char section[]="RunicProfileA";section[12]=(char)('A'+slot);
    if(GetPrivateProfileIntA(section,"Version",0,g_configPath)!=1)return false;
    for(int i=0;i<kKhaosFieldCount;++i){const KhaosField& f=g_khaosFields[i];
        int n=(int)GetPrivateProfileIntA(section,f.key,f.initial,g_configPath);
        if(n<f.min)n=f.min;if(n>f.max)n=f.max;g_khaosDraft[i]=n;}
    for(int w=0;w<3;++w){char key[]="Wheel1Title";key[5]=(char)('1'+w);
        char temp[24]{};GetPrivateProfileStringA(section,key,"WHEEL",temp,sizeof(temp),g_configPath);
        khaos_copy_title(g_khaosTitles[w],temp);}
    g_khaosDirty=1;g_khaosProfileNotice=slot+1;g_khaosProfileAction=2;g_khaosStatus=0;return true;
}
void khaos_apply_palette(int number){
    if(number<0||number>=6)return;
    const DWord c=g_khaosPalettes[number];
    const char* keys[3]={"AccentRed","AccentGreen","AccentBlue"};
    const int colors[3]={int((c>>16)&255),int((c>>8)&255),int(c&255)};
    for(int j=0;j<3;++j){for(int i=0;i<kKhaosFieldCount;++i){
        const char* a=keys[j],*b=g_khaosFields[i].key;
        while(*a && *a==*b){++a;++b;}if(!*a && !*b)g_khaosDraft[i]=colors[j];
    }}
    g_khaosDirty=1;g_khaosProfileAction=0;g_khaosStatus=0;
}
void khaos_reset_tab(){for(int i=0;i<kKhaosFieldCount;++i){if(g_khaosFields[i].tab==g_khaosTab){
    g_khaosDraft[i]=g_khaosFields[i].initial;g_khaosDirty=1;}}
}
int khaos_draft_key(const char* key){
    for(int i=0;i<kKhaosFieldCount;++i){const char* a=key;const char* b=g_khaosFields[i].key;
       while(*a&&*b&&*a==*b){++a;++b;}if(*a==0&&*b==0)return g_khaosDraft[i];}
    return 0;
}
// Conservatively reject reserved function-key or duplicate mouse-wheels, and
// warn rather than delete hidden saved content.
bool khaos_valid_draft(){
    if(khaos_draft_key("CycleEntryUpMouseCode")==khaos_draft_key("CycleEntryDownMouseCode"))return false;
    const char* keys[]={"OpenKeyboardScanCode","PreviousWheelScanCode","NextWheelScanCode",
        "CloseKeyboardScanCode","PreviousItemScanCode","NextItemScanCode",
        "PreviousOccupiedSectorScanCode","NextOccupiedSectorScanCode"};
    for(unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);++i){
        const int a=khaos_draft_key(keys[i]);
        if(a>=0x3c && a<=0x3f)return false; // Skyrim F2/F3/F4/F5 preserved
        if(a){for(unsigned j=0;j<i;++j){if(a==khaos_draft_key(keys[j]))return false;}}
    }
    return true;
}
void khaos_int_string(int n,char (&buffer)[24]){
    unsigned value=n<0?(unsigned)-n:(unsigned)n;
    char temp[14];int count=0;
    do{temp[count++]=(char)('0'+value%10);value/=10;}while(value && count<12);
    int j=0;if(n<0)buffer[j++]='-';while(count)buffer[j++]=temp[--count];buffer[j]=0;
}
bool khaos_apply(){
    if(!g_khaosReady || !g_configPath[0] || !khaos_valid_draft()){g_khaosStatus=3;return false;}
    // Preserve all existing INI sections, especially user's [BindingsV2].
    // Only changed keys are written; no independent KHA-OS settings store.
    for(int i=0;i<kKhaosFieldCount;++i){
        if(*g_khaosFields[i].target!=g_khaosDraft[i]){
            char text[24]{};khaos_int_string(g_khaosDraft[i],text);
            if(!WritePrivateProfileStringA("Controls",g_khaosFields[i].key,text,g_configPath)){
                g_khaosStatus=3;file_log("RunicWheelLE: KHAOS config INI write FAILED\r\n");return false;
            }
        }
    }
    for(int w=0;w<3;++w){
        const char* a=g_khaosTitles[w];const char* b=g_wheelTitles[w];
        while(*a && *a==*b){++a;++b;}if(*a!=*b){
            char key[]="Wheel1Title";key[5]=(char)('1'+w);
            if(!WritePrivateProfileStringA("Controls",key,g_khaosTitles[w],g_configPath)){
                g_khaosStatus=3;return false;
            }
        }
    }
    // These targets are 32-bit integer fields. On Windows x86 aligned stores
    // are atomic, but still publish via InterlockedExchange across threads.
    for(int i=0;i<kKhaosFieldCount;++i)
        InterlockedExchange((volatile Long*)g_khaosFields[i].target,g_khaosDraft[i]);
    for(int w=0;w<3;++w)khaos_copy_title(g_wheelTitles[w],g_khaosTitles[w]);
    g_cfg.padDeadzone=g_gamepadDeadzonePercent*2;
    g_enableSlowTime=(g_slowPercent<100)?1:0;
    if(g_activeWheel>=g_pageCount)InterlockedExchange(&g_activeWheel,0);
    if(g_select>=g_sectorCount)InterlockedExchange(&g_select,0);
    select_current();
    g_khaosDirty=0;g_khaosStatus=1;g_khaosProfileAction=0;
    file_log("RunicWheelLE: KHAOS settings saved to canonical RunicWheelLE.ini\r\n");
    return true;
}

// UI native scaling independent of wheel text-size user settings.
void khaos_text(float cx,float y,float width,float px,const char* content,DWord color){
    professional_text(cx,y,width,px,content,color);
}
// Align glyph ink to the actual rectangle: atlas cells contain padding above
// and below printable pixels. Never use a magic negative Y offset per row.
void khaos_centered(float cx,float boxY,float boxH,float width,float px,const char* content,DWord color){
    professional_text(cx,rw25::centered_text_y(boxY,boxH,px),width,px,content,color);
}
void khaos_rect(float x,float y,float w,float h,DWord color){if(w>0 && h>0)mark_quad(x,y,w,h,color);}
void khaos_line(float x,float y,float w,DWord color){khaos_rect(x,y,w,2.f,color);}
void khaos_field_display(int id,char (&out)[24]){
    int value=g_khaosDraft[id];if(g_khaosFields[id].min==0 && g_khaosFields[id].max==1){
        khaos_copy_title(out,value?"ON":"OFF");return;}
    khaos_int_string(value,out);
}

struct KhaosDimensions {float x,y,w,h,scale,rowBase,rowStep,leftW,previewX;int rows;};
KhaosDimensions khaos_dimensions(int width,int height){
    const float s=(height<800?0.85f:height>1300?1.17f:1.f)*g_khaosUIScale;
    float w=(float)width-40.f;if(w>1120.f*s)w=1120.f*s;
    float h=(float)height-36.f;if(h>790.f*s)h=790.f*s;
    const float baseX=((float)width-w)*.5f;
    const float baseY=((float)height-h)*.5f;
    const rw25::Offset o=rw25::restrict_offset((float)width,(float)height,baseX,baseY,w,h,g_khaosOffsetX,g_khaosOffsetY);
    const float x=baseX+o.x,y=baseY+o.y;
    const float previewX=(w>880.f*s)?x+w-315.f*s:x+w;
    const float leftW=previewX-x;
    const float rowBase=y+141.f*s;
    const float rowStep=40.f*s;
    int rows=(int)((h-259.f*s)/rowStep);if(rows<3)rows=3;if(rows>14)rows=14;
    return {x,y,w,h,s,rowBase,rowStep,leftW,previewX,rows};
}

void khaos_draw_menu(int width,int height){
    const KhaosDimensions d=khaos_dimensions(width,height);
    const float s=d.scale;
    const float accentR=(float)khaos_draft_key("AccentRed");
    const float accentG=(float)khaos_draft_key("AccentGreen");
    const float accentB=(float)khaos_draft_key("AccentBlue");
    const DWord gold=0xff000000u | ((DWord)accentR<<16)|((DWord)accentG<<8)|(DWord)accentB;
    g_labelVert=0;g_fontVertexCount=0;g_artVertexCount=0;g_miniArtVert=0;
    khaos_rect(0,0,(float)width,(float)height,0xb5080c13u);
    khaos_rect(d.x,d.y,d.w,d.h,g_khaosThemePanel);
    khaos_rect(d.x,d.y,d.w,4.f*s,gold);
    khaos_rect(d.x,d.y+56.f*s,d.w,2.f*s,0xb189704eu);
    // Drag handle and a dedicated close button, never hidden behind the title.
    const float closeX=d.x+d.w-50.f*s,closeY=d.y+10.f*s;
    khaos_rect(closeX,closeY,37.f*s,35.f*s,0xff573640u);
    khaos_rect(closeX,closeY,37.f*s,2.f*s,gold);
    khaos_centered(closeX+18.5f*s,closeY,35.f*s,31.f*s,25.f*s,"X",0xffffdfc8u);
    khaos_centered(d.x+(d.w-60.f*s)*.5f,d.y+3.f*s,35.f*s,d.w-130.f*s,29.f*s,
        "RUNIC WHEEL  |  KHA-OS CONFIGURATION",0xfff6dfb0u);
    static const char* const sub="PRIVATE LE 1.9.32  /  KHA-OS MODULE  /  LIVE PREVIEW  /  CANONICAL INI";
    khaos_text(d.x+d.w*.5f,d.y+36.f*s,d.w-32.f*s,12.f*s,sub,0xffb4bec9u);
    const float tabW=d.w/8.f;
    for(int t=0;t<8;++t){
        const float tx=d.x+(float)t*tabW;
        khaos_rect(tx,d.y+63.f*s,tabW-2.f*s,43.f*s,t==g_khaosTab?0xff344151u:0xff202b39u);
        if(t==g_khaosTab)khaos_rect(tx,d.y+103.f*s,tabW-2.f*s,3.f*s,gold);
        khaos_centered(tx+tabW*.5f,d.y+63.f*s,43.f*s,tabW-11.f*s,16.f*s,g_khaosTabs[t],t==g_khaosTab?0xffffe7bdu:0xffb6bcc5u);
    }
    khaos_rect(d.x+15.f*s,d.y+119.f*s,d.leftW-28.f*s,1.f*s,0x967e715du);
    const int count=khaos_field_tab_count(g_khaosTab);
    const int maxScroll=count>d.rows?count-d.rows:0;
    if(g_khaosScroll>maxScroll)g_khaosScroll=maxScroll;
    for(int row=0;row<d.rows;++row){
        const int actual=g_khaosScroll+row;
        if(actual>=count)break;
        const int idx=khaos_field_index(g_khaosTab,actual);
        if(idx<0)break;
        const float yy=d.rowBase+(float)row*d.rowStep;
        khaos_rect(d.x+13.f*s,yy,d.leftW-26.f*s,d.rowStep-4.f*s,(row%2?0x84232f3du:0x842b3949u));
        if(g_khaosFocus==idx)khaos_rect(d.x+13.f*s,yy,4.f*s,d.rowStep-4.f*s,gold);
        const float labelLeft=d.x+26.f*s;
        const float labelW=d.leftW-240.f*s;
        khaos_centered(labelLeft+labelW*.5f,yy,d.rowStep-4.f*s,labelW,17.4f*s,
            g_khaosFields[idx].label,0xffecdfc9u);
        char number[24]{};khaos_field_display(idx,number);
        const float bx=d.x+d.leftW-89.f*s;
        khaos_rect(bx-84.f*s,yy+2.f*s,31.f*s,28.f*s,0xff344457u);
        khaos_rect(bx+47.f*s,yy+2.f*s,31.f*s,28.f*s,0xff344457u);
        khaos_centered(bx-68.f*s,yy+2.f*s,28.f*s,23.f*s,21.f*s,"-",0xffe9c882u);
        khaos_centered(bx+62.f*s,yy+2.f*s,28.f*s,23.f*s,21.f*s,"+",0xffe9c882u);
        khaos_centered(bx,yy+2.f*s,28.f*s,84.f*s,21.f*s,number,0xfffcf3e7u);
    }
    // Narrow view still provides fully functional tab/row controls.
    const int focus=(g_khaosFocus>=0 && g_khaosFields[g_khaosFocus].tab==g_khaosTab)?g_khaosFocus:khaos_field_index(g_khaosTab,0);
    if(focus>=0){
        khaos_text(d.x+d.leftW*.5f,d.y+d.h-109.f*s,d.leftW-40.f*s,15.f*s,
            g_khaosFields[focus].help,0xffbdd4d8u);
    }
    if(d.previewX<d.x+d.w){
        const float cx=d.previewX+(d.x+d.w-d.previewX)*.5f;
        const float cy=d.y+270.f*s;
        const float r=109.f*s;
        khaos_rect(d.previewX+7.f*s,d.y+122.f*s,d.x+d.w-d.previewX-14.f*s,445.f*s,0xd11a2835u);
        khaos_centered(cx,d.y+132.f*s,34.f*s,260.f*s,21.f*s,"LIVE LAYOUT PREVIEW",gold);
        const int n=khaos_draft_key("VisibleSectorCount");
        const int pages=khaos_draft_key("VisiblePageCount");
        for(int i=0;i<n;++i){
            const rwl24::Point p=rwl24::center(n,i);
            const float x=cx+p.x*r*.76f,y=cy+p.y*r*.76f;
            khaos_rect(x-17.f*s,y-17.f*s,34.f*s,34.f*s,0xff40536au);
            khaos_rect(x-14.f*s,y-14.f*s,28.f*s,28.f*s,(i==0)?gold:0xff7f8a93u);
            char number[24]{};khaos_int_string(i+1,number);
            khaos_centered(x,y-17.f*s,34.f*s,25.f*s,18.f*s,number,i==0?0xff1c252fu:0xfff2eee6u);
        }
        khaos_rect(cx-52.f*s,cy-31.f*s,104.f*s,62.f*s,0xff111c29u);
        khaos_centered(cx,cy-31.f*s,31.f*s,91.f*s,23.f*s,"RUNIC",gold);
        khaos_centered(cx,cy+3.f*s,28.f*s,94.f*s,13.f*s,"WHEEL LE",0xffd9e0e6u);
        char np[24]{},ns[24]{};khaos_int_string(pages,np);khaos_int_string(n,ns);
        khaos_text(cx,d.y+396.f*s,265.f*s,15.f*s,"VISIBLE PAGE / SECTOR LIMITS",0xffa9c5c5u);
        khaos_centered(cx-43.f*s,d.y+426.f*s,44.f*s,70.f*s,28.f*s,np,gold);
        khaos_centered(cx+43.f*s,d.y+426.f*s,44.f*s,70.f*s,28.f*s,ns,gold);
        khaos_text(cx,d.y+461.f*s,264.f*s,12.f*s,"HIDDEN ITEMS REMAIN SAVED",0xffdbbf94u);
        if(d.h>820.f*s){
            khaos_text(cx,d.y+619.f*s,270.f*s,16.f*s,"COLOUR PRESETS",gold);
            for(int j=0;j<6;++j){
                const int c=j%3,row=j/3;
                const float px=d.previewX+20.f*s+(float)c*94.f*s;
                const float py=d.y+(647.f+(float)row*61.f)*s;
                khaos_rect(px,py,82.f*s,45.f*s,0xff1f2b3au);
                khaos_rect(px+4.f*s,py+4.f*s,74.f*s,16.f*s,g_khaosPalettes[j]);
                khaos_centered(px+41.f*s,py+20.f*s,25.f*s,76.f*s,12.f*s,g_khaosPaletteNames[j],0xfff0f1e9u);
            }
        }
        if(g_khaosTab==2){
            khaos_text(cx,d.y+498.f*s,270.f*s,13.f*s,"PAGE TITLE EDITOR: FOCUS + TYPE",0xffd3e1e3u);
            for(int w=0;w<3;++w){
                const float tx=d.y+(521.f+23.f*w)*s;
                khaos_text(cx,tx,265.f*s,14.f*s,g_khaosTitles[w],g_khaosTitleEdit==w?gold:0xffc2b0a1u);
            }
        }
    }
    const float by=d.y+d.h-68.f*s;
    khaos_line(d.x+18.f*s,by-7.f*s,d.w-36.f*s,0x9b9c825cu);
    const char* status=g_khaosProfileAction==1?"PROFILE SAVED TO INI":g_khaosProfileAction==2?"PROFILE LOADED: PRESS ENTER TO APPLY":g_khaosStatus==1?"SAVED TO RUNICWHEELLE.INI":g_khaosStatus==2?"CHANGES REVERTED":
        g_khaosStatus==3?"INVALID INPUT OR SAVE FAILED":g_khaosDirty?"UNSAVED CHANGES":"READY";
    khaos_centered(d.x+d.w*.5f,by-3.f*s,24.f*s,d.w-20.f*s,17.f*s,status,g_khaosStatus==3?0xffff9c87u:gold);
    khaos_centered(d.x+d.w*.5f,by+21.f*s,22.f*s,d.w-25.f*s,14.f*s,
        "ENTER APPLY  |  END RESET TAB  |  F6-F8 SAVE PROFILES  |  F9-F11 LOAD  |  ESC CLOSE",0xffe1e3dfu);
}

bool khaos_ui_event(unsigned msg,uintptr_t wp,intptr_t lp){
    if(!g_khaosOpen)return false;
    const KhaosDimensions d=khaos_dimensions(g_khaosWidth,g_khaosHeight);
    // Client-area mouse messages carry client coordinates (WM_MOUSEWHEEL does not).
    if(msg>=0x0200u && msg<=0x0209u)khaos_point(lp,g_khaosMouseX,g_khaosMouseY);
    if(rw25::skyrim_tab(msg,(unsigned)wp)){
        InterlockedExchange(&g_khaosOpen,0);g_khaosDragging=0;
        file_log("RunicWheelLE: KHAOS released Skyrim TAB (window message)\r\n");
        return false; // do NOT steal Skyrim's TweenMenu hotkey
    }
    if(msg==0x0104u && wp==0x1bu){
        InterlockedExchange(&g_khaosOpen,0);g_khaosDragging=0;return false;
    }
    if(msg==0x0100u){ // WM_KEYDOWN
        const int key=(int)wp;
        if(key==0x1b){InterlockedExchange(&g_khaosOpen,0);g_khaosDragging=0;return true;}
        if(key==0x0d){khaos_apply();return true;}
        if(key==0x08 && g_khaosTitleEdit<0){khaos_discard();return true;}
        // F5 belongs to Skyrim Quick Save. Never claim or bind it.
        if(key==0x23){khaos_reset_tab();return true;} // End resets this tab.
        // F6/F7/F8 save profiles A/B/C, F9/F10/F11 restore to staged settings.
        if(key>=0x75 && key<=0x77){if(!khaos_save_profile(key-0x75))g_khaosStatus=3;return true;}
        if(key>=0x78 && key<=0x7a){if(!khaos_load_profile(key-0x78))g_khaosStatus=3;return true;}
        if(key==0x21){g_khaosTab=(g_khaosTab+7)%8;g_khaosScroll=0;g_khaosFocus=-1;g_khaosTitleEdit=-1;return true;}
        if(key==0x22){g_khaosTab=(g_khaosTab+1)%8;g_khaosScroll=0;g_khaosFocus=-1;g_khaosTitleEdit=-1;return true;}
        if(g_khaosTitleEdit>=0){
            if(key==0x08){char* p=g_khaosTitles[g_khaosTitleEdit];int len=0;while(p[len])++len;
                if(len>0){p[len-1]=0;g_khaosDirty=1;}return true;}
            if(key==0x1b || key==0x0d){g_khaosTitleEdit=-1;return true;}
            return true;
        }
        const int count=khaos_field_tab_count(g_khaosTab);
        if(key==0x26 || key==0x28){
            int row=0;for(int i=0;i<count;++i)if(khaos_field_index(g_khaosTab,i)==g_khaosFocus){row=i;break;}
            row+=(key==0x26?-1:1);if(row<0)row=0;if(row>=count)row=count-1;
            g_khaosFocus=khaos_field_index(g_khaosTab,row);
            if(row<g_khaosScroll)g_khaosScroll=row;
            if(row>=g_khaosScroll+d.rows)g_khaosScroll=row-d.rows+1;
            return true;
        }
        if(key==0x25 || key==0x27){
            if(g_khaosFocus<0)g_khaosFocus=khaos_field_index(g_khaosTab,0);
            khaos_adjust(g_khaosFocus,key==0x25?-1:1);return true;
        }
        return true;
    }
    if(msg==0x0102u && g_khaosTitleEdit<0)return true;
    if(msg==0x0102u && g_khaosTitleEdit>=0){ // WM_CHAR
        const unsigned c=(unsigned)wp;
        if(c>=32 && c<=126){char* p=g_khaosTitles[g_khaosTitleEdit];int n=0;while(p[n])++n;
            if(n<23){p[n]=(char)c;p[n+1]=0;g_khaosDirty=1;}}
        return true;
    }
    if(msg==0x020Au){ // WM_MOUSEWHEEL
        const short delta=(short)((wp>>16)&0xffff);
        const int maxScroll=khaos_field_tab_count(g_khaosTab)-d.rows;
        g_khaosScroll+=delta>0?-1:1;
        if(g_khaosScroll<0)g_khaosScroll=0;
        if(g_khaosScroll>maxScroll)g_khaosScroll=maxScroll>0?maxScroll:0;
        return true;
    }
    if(msg==0x0200u){ // WM_MOUSEMOVE, drag ONLY while holding title bar
        if(g_khaosDragging){
            const float mx=g_khaosMouseX;
            const float my=g_khaosMouseY;
            const float baseX=((float)g_khaosWidth-d.w)*.5f;
            const float baseY=((float)g_khaosHeight-d.h)*.5f;
            const rw25::Offset off=rw25::restrict_offset((float)g_khaosWidth,(float)g_khaosHeight,
                baseX,baseY,d.w,d.h,mx-g_khaosGrabX-baseX,my-g_khaosGrabY-baseY);
            g_khaosOffsetX=off.x;g_khaosOffsetY=off.y;
            return true;
        }
        return rw25::contains({d.x,d.y,d.w,d.h},g_khaosMouseX,g_khaosMouseY);
    }
    if(msg==0x0215u){g_khaosDragging=0;return false;} // capture changed: end drag, keep UI
    if(msg==0x0202u){
        if(g_khaosDragging)file_log("RunicWheelLE: KHAOS window drag finished\r\n");
        g_khaosDragging=0;return true;
    } // WM_LBUTTONUP
    if(msg==0x0201u){ // WM_LBUTTONDOWN
        const float x=g_khaosMouseX;
        const float y=g_khaosMouseY;
        if(!rw25::contains({d.x,d.y,d.w,d.h},x,y)){
            g_khaosDragging=0;InterlockedExchange(&g_khaosOpen,0);
            return false; // allow host Hub/Skyrim to process outside clicks
        }
        const float bx=d.x+d.w-50.f*d.scale;
        if(rw25::contains({bx,d.y+10.f*d.scale,37.f*d.scale,35.f*d.scale},x,y)){
            g_khaosDragging=0;InterlockedExchange(&g_khaosOpen,0);
            file_log("RunicWheelLE: KHAOS close button clicked\r\n");return true;
        }
        if(y>=d.y && y<d.y+56.f*d.scale){
            g_khaosDragging=1;g_khaosGrabX=x-d.x;g_khaosGrabY=y-d.y;
            file_log("RunicWheelLE: KHAOS window drag started\r\n");
            return true;
        }
        if(y>=d.y+63.f*d.scale && y<=d.y+108.f*d.scale && x>=d.x && x<d.x+d.w){
            int t=(int)((x-d.x)/(d.w/8.f));if(t>=0&&t<8){g_khaosTab=t;g_khaosScroll=0;g_khaosFocus=-1;g_khaosTitleEdit=-1;}return true;
        }
        if(g_khaosTab==2 && d.previewX<d.x+d.w && x>=d.previewX && y>=d.y+519.f*d.scale && y<d.y+602.f*d.scale){
            int t=(int)((y-(d.y+519.f*d.scale))/(23.f*d.scale));
            if(t>=0&&t<3){g_khaosTitleEdit=t;g_khaosFocus=-1;}return true;
        }
        if(d.previewX<d.x+d.w && d.h>820.f*d.scale && x>=d.previewX){
            for(int j=0;j<6;++j){const float px=d.previewX+20.f*d.scale+(float)(j%3)*94.f*d.scale;
                const float py=d.y+(647.f+(float)(j/3)*61.f)*d.scale;
                if(x>=px&&x<px+82.f*d.scale&&y>=py&&y<py+45.f*d.scale){
                    khaos_apply_palette(j);return true;}
            }
        }
        if(x>=d.x && x<=d.x+d.leftW && y>=d.rowBase && y<d.rowBase+d.rowStep*d.rows){
            const int row=(int)((y-d.rowBase)/d.rowStep);
            const int id=khaos_field_index(g_khaosTab,row+g_khaosScroll);
            if(id>=0){
                g_khaosFocus=id;
                const float bx=d.x+d.leftW-89.f*d.scale;
                if(x>=bx-97.f*d.scale && x<=bx-43.f*d.scale)khaos_adjust(id,-1);
                else if(x>=bx+40.f*d.scale && x<=bx+89.f*d.scale)khaos_adjust(id,1);
                else if(g_khaosFields[id].max==1 && g_khaosFields[id].min==0)khaos_adjust(id,g_khaosDraft[id]? -1:1);
            }return true;
        }
        return true;
    }
    // WndProc must not starve other UI surfaces. Close on focus loss, let
    // outside click-through and unhandled messages reach KHA-OS/Skyrim.
    return msg==0x0204u || msg==0x0205u || msg==0x0101u;
}

int khaos_draw_frame(Raw device,int width,int height){
    if(!device||width<640||height<480)return 0;
    Raw block=nullptr,back=nullptr,oldTarget=nullptr,oldDepth=nullptr;
    Raw extraTargets[3]={nullptr,nullptr,nullptr};
    SurfaceDesc desc{};
    int ok=0,scene=0,depthWasPresent=0;
    const int wheelArtEnabled=g_firstThemeEnabled;
    if(!succeeded(reinterpret_cast<CreateBlock>(get(device,kCreateBlock))(device,1,&block))||!block)return 0;
    if(!succeeded(reinterpret_cast<BlockAction>(get(block,4))(block)))goto cleanup;
    if(!succeeded(reinterpret_cast<GetBack>(get(device,kGetBackBuffer))(device,0,0,0,&back))||!back)goto cleanup;
    // Lay the panel out in back-buffer pixels; mouse messages are mapped into the same space.
    if(succeeded(reinterpret_cast<GetSurfaceDesc>(get(back,kSurfaceDesc))(back,&desc)) &&
       desc.width>=640 && desc.height>=480){width=(int)desc.width;height=(int)desc.height;}
    g_khaosWidth=width;g_khaosHeight=height;
    if(g_khaosSeedPointer){g_khaosSeedPointer=0;if(g_khaosMouseX<0.f)khaos_seed_pointer();}
    if(!succeeded(reinterpret_cast<GetSurface>(get(device,kGetTarget))(device,0,&oldTarget))||!oldTarget)goto cleanup;
    if(succeeded(reinterpret_cast<GetDepth>(get(device,kGetDepth))(device,&oldDepth)))depthWasPresent=1;
    if(!succeeded(reinterpret_cast<SetSurface>(get(device,kSetTarget))(device,0,back)))goto cleanup;
    detach_extra_targets(device,extraTargets);
    if(!succeeded(reinterpret_cast<SetDepth>(get(device,kSetDepth))(device,nullptr)))goto cleanup;
    // Host KHA-OS may already be inside BeginScene. D3DERR_INVALIDCALL from
    // an active scene is not proof the device is lost; draw attempts fail safely.
    if(succeeded(reinterpret_cast<Scene>(get(device,kBeginScene))(device)))scene=1;
    overlay_state(device,(DWord)width,(DWord)height);
    g_firstThemeEnabled=1; // KHA-OS font is needed even if radial artwork is disabled.
    (void)load_theme(device);
    g_firstThemeEnabled=wheelArtEnabled;
    khaos_update_theme();
    g_khaosFontOverride=1;
    khaos_draw_menu(width,height);
    g_khaosFontOverride=0;
    ok=succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))(device,4,(UInit)(g_labelVert/3),g_labels,sizeof(Vertex)));
    if(ok && g_fontAtlas && g_fontVertexCount)ok=draw_textured_pass(device,g_fontAtlas,g_fontQuads,(UInit)(g_fontVertexCount/3));
    reset_texture_pass(device);
    // The pointer goes last, above the full-screen backdrop of the panel.
    if(ok && khaos_pointer_wanted()){
        const int n=khaos_build_pointer(g_khaosMouseX,g_khaosMouseY,(float)height/1080.f*1.35f);
        ok=succeeded(reinterpret_cast<DrawUp>(get(device,kDrawUP))(device,4,(UInit)(n/3),g_khaosPointer,sizeof(Vertex)));
    }
cleanup:
    if(scene)reinterpret_cast<Scene>(get(device,kEndScene))(device);
    if(oldTarget)reinterpret_cast<SetSurface>(get(device,kSetTarget))(device,0,oldTarget);
    restore_extra_targets(device,extraTargets);
    if(depthWasPresent)reinterpret_cast<SetDepth>(get(device,kSetDepth))(device,oldDepth);
    else if(back)reinterpret_cast<SetDepth>(get(device,kSetDepth))(device,nullptr);
    if(block)reinterpret_cast<BlockAction>(get(block,5))(block);
    drop(oldDepth);drop(oldTarget);drop(back);drop(block);
    return ok;
}
