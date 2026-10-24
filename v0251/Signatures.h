#pragma once
// TESV.exe 1.9.32.0.8 only. Matches CommonLibLE verified row BSRenderer_PublishDevice.
// LE static evidence: function 0x00CE7D20 loads [0x01B913F0], then [eax+0x38],
// stores IDirect3DDevice9* to [0x01BB0928] and returns (15 exact bytes).
// The C++ code verifies every byte before installing a COM hook.
namespace v02sig {
constexpr unsigned long kPublishFunction = 0x00CE7D20u;
constexpr unsigned long kDevicePointer = 0x01BB0928u;
constexpr unsigned long kInputCallsite = 0x00A68F86u;
constexpr unsigned char kVanillaInputCall[5] = {0xE8,0x85,0x2C,0xC8,0xFF};
constexpr unsigned char kPublishBytes[15] = {
    0xA1,0xF0,0x13,0xB9,0x01,0x8B,0x48,0x38,
    0x89,0x0D,0x28,0x09,0xBB,0x01,0xC3
};
}

namespace v07sig {
// Verified directly from the owner's Skyrim LE 1.9.32.0.8 TESV.exe.
constexpr unsigned long kMenuManagerSingleton=0x012E3548u; // already CommonLibLE g_menuManager
constexpr unsigned long kEventDispatcherAddEventSink=0x006E3E30u; // proven ret 4, new CommonLibLE record
constexpr unsigned long kInventoryMenuVtable=0x010E5B90u;
constexpr unsigned long kMagicMenuVtable=0x010E6594u;
constexpr unsigned long kGlobalTimeMultiplier=0x012C2410u;
constexpr unsigned long kBSTimer=0x01B4ADD0u;
constexpr unsigned long kSetGlobalTimeMultiplier=0x00A4B150u;
constexpr unsigned long kGFxGetMember=0x00923960u;
constexpr unsigned long kGFxReleaseManaged=0x00922660u;
constexpr unsigned long kLookupFormByID=0x00451A30u;
constexpr unsigned long kInventoryRender=0x00849200u;
constexpr unsigned long kMagicRender=0x00873940u;
constexpr unsigned char kSlowPrologue[5]={0x80,0x79,0x2c,0x00,0x74};
constexpr unsigned char kGetMemberPrologue[3]={0x83,0xec,0x10};
constexpr unsigned char kReleaseManagedPrologue[3]={0x8b,0x44,0x24};
}

// LE 1.9.32.0.8 MagicMenu calls this function for SpellItem formType 0x16.
// 0x00873A9A: ECX=EquipManager; pushes slot, form, actor. Function ret 0x0C.
// CommonLibLE verified symbol EquipManager_EquipSpell. Static-only, game use untested.
namespace v11sig {
constexpr unsigned long kEquipManagerEquipSpell=0x006EFAC0u;
constexpr unsigned long kMagicMenuEquipSpellCallsite=0x00873A9Au;
}
