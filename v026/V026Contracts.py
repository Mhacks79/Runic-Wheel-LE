#!/usr/bin/env python3
"""v0.26 static contracts. Reads the real TESV.exe for every engine site the new code
calls, proves the co-save code untouched against v0.25.1, and checks the render-state
isolation and the add filter are wired into both draw paths. Not a game test."""
import os, re, zlib
from pathlib import Path

here = Path(__file__).resolve().parent
src = (here / 'RunicWheelV011.cpp').read_text(encoding='utf-8')
inl = (here / 'RunicKhaosMenu.inl').read_text(encoding='utf-8')
ini = (here / 'RunicWheelLE.ini').read_text(encoding='utf-8')
ref = (here / 'REFERENCE' / 'v0251-RunicWheelV011.cpp').read_text(encoding='utf-8')
exe = Path(os.environ.get('TESV_EXE', str(here.parent / 'inputs' / 'TESV.exe')))  # never published
checks = {}

# 1. Co-save: record format and callbacks byte-for-byte those of v0.25.1.
def segment(s, start, end):
    i = s.index(start); return s[i:s.index(end, i)]
for start, end in [('void save_impl(SKSESerializationInterface* serial){', 'int serialization_fault(){'),
                   ('static bool resolve_saved_form(', 'void save_impl(')]:
    checks[f'co-save unchanged: {start[:30]}'] = segment(src, start, end) == segment(ref, start, end)
for header in ['SaveV2.h', 'BindingModel.h', 'CatalogSave.h', 'BindSyntax.h']:
    checks[f'{header} unchanged'] = (here / header).read_bytes() == (here / 'REFERENCE' / f'v0251-{header}').read_bytes()

# 2. Every engine site the plugin checks is what TESV.exe 1.9.32.0.8 holds there.
data = exe.read_bytes()
import struct
pe = struct.unpack_from('<I', data, 0x3C)[0]
nsec = struct.unpack_from('<H', data, pe + 6)[0]; opt = struct.unpack_from('<H', data, pe + 20)[0]
base = struct.unpack_from('<I', data, pe + 24 + 28)[0]
secs = [struct.unpack_from('<8sIIII', data, pe + 24 + opt + 40 * i) for i in range(nsec)]
def at(va, n):
    rva = va - base
    for name, vsize, vaddr, rsize, raw in secs:
        if vaddr <= rva < vaddr + max(vsize, rsize):
            return data[raw + rva - vaddr: raw + rva - vaddr + n]
    raise KeyError(hex(va))
def array(name):
    m = re.search(r'static const unsigned char ' + name + r'\[\]=\{([^}]*)\}', src)
    return bytes(int(x, 16) for x in m.group(1).split(','))
sites = {'name': 0x00475AA0, 'count': 0x0040C190, 'shout': 0x006F0110, 'busy': 0x006E8AA0,
         'equip': 0x006EF3E0, 'unequip': 0x006EE560, 'equipSpell': 0x006EFAC0}
for name, va in sites.items():
    want = array(name)
    checks[f'TESV bytes at {va:#010x} ({name})'] = at(va, len(want)) == want
checks['site addresses used in code'] = all(f'0x{va:08X}' in src for va in sites.values())

# 3. SKSE LE HashUtil table == zlib's CRC32, so the bitwise rw26::crc32 equals SKSE itemId.
sdk = Path(os.environ.get('RUNIC_LAB_ROOT', str(here.parent / 'lab'))) / 'sdk/skse-le-sdk/skse/skse/HashUtil.cpp'
table = [int(x, 16) for x in re.findall(r'0x([0-9A-Fa-f]{8})', sdk.read_text())][:256]
def zrow(i):
    c = i
    for _ in range(8): c = (c >> 1) ^ (0xEDB88320 if c & 1 else 0)
    return c
checks['SKSE CRC table is the zlib table'] = len(table) == 256 and all(table[i] == zrow(i) for i in range(256))
checks['zlib check value'] = zlib.crc32(b'123456789', 0) == 0xCBF43926

# 4. Render isolation in BOTH the wheel and the KHA-OS panel.
state_fn = segment(src, 'void overlay_state(Raw device,DWord width,DWord height){', 'void detach_extra_targets(')
for state, label in [(194, 'SRGBWRITEENABLE'), (168, 'COLORWRITEENABLE'), (171, 'BLENDOP'), (206, 'SEPARATEALPHA')]:
    checks[f'overlay sets {label}'] = re.search(r'\{' + str(state) + r',', state_fn) is not None
checks['overlay sets viewport and sRGB sampling'] = 'kSetViewport' in state_fn and '(device,0,11,0)' in state_fn
draw = segment(src, 'int draw(Raw device){', 'int on_fault(){')
panel = segment(inl, 'int khaos_draw_frame(Raw device,int width,int height){', 'return ok;')
for label, body in [('wheel', draw), ('panel', panel)]:
    checks[f'{label}: back-buffer size'] = 'kSurfaceDesc' in body
    checks[f'{label}: overlay_state'] = 'overlay_state(device,' in body
    checks[f'{label}: MRT detached and restored'] = 'detach_extra_targets(device,extraTargets)' in body and 'restore_extra_targets(device,extraTargets)' in body
    checks[f'{label}: no stale per-path state list'] = '(device,174,0)' not in body
checks['panel pointer drawn last'] = panel.index('khaos_pointer_wanted') > panel.index('draw_textured_pass(device,g_fontAtlas')
checks['INI documents KhaosCursorMode'] = 'KhaosCursorMode=1' in ini

# 5. Gameplay wiring.
act = segment(src, 'void perform_action(const ActionRequest& request){', 'int native_action_fault(){')
edit = segment(src, 'void edit_click(bool remove){', 'static bool resolve_saved_form(')
checks['add filter'] = 'rw26::addable(pick.formType,pick.spellType)' in edit
checks['add keeps copy only for weapons/armor'] = 'if(!remove && rw26::keeps_variant(pick.formType))b.variantId=pick.variant;' in edit
checks['add reads one consistent pick'] = 'read_ui_pick()' in edit and 'g_uiKind' not in edit
checks['no blanket variant refusal'] = 'variant hash needs exact instance' not in act
checks['exact copy passed to EquipItem'] = '(manager,actor,form,target,1,slot,true,false,false,nullptr)' in act
checks['power gets no slot'] = 'rw26::spell_takes_hand(spellType)?slot:nullptr' in act
checks['abilities refused'] = 'spellType!=0 && spellType!=2 && spellType!=3' in act
checks['shout route verified + learned + busy'] = all(x in act for x in ['shout_sites_verified()', 'learned_shout(actor,form)', '0x006E8AA0)(actor)', '0x006F0110)(manager,actor,form)'])
checks['scroll routed by formType'] = 'const bool scroll=formType==0x17;' in act and 'kind==rw05::Weapon || scroll' in act
checks['ingredient no longer refused'] = 'ingredient eat native route not verified' not in act
checks['ownership still required'] = 'if(!owned_inventory(actor,form,owned))' in act
checks['spell still through EquipSpell only'] = act.count('0x006EFAC0') == 1 and 'GameEquip>(0x006EF3E0)(manager,actor,form,spellSlot' not in act
checks['still on the game task'] = 'g_tasks->AddTask((TaskDelegate*)&g_nativeTask)' in src
checks['partial copy list never acted on'] = 'if(out.lists>=kMaxCopies)return false;' in src
checks['no function-local dynamic statics added'] = not re.search(r'\n\s+static (?!const)[A-Za-z_:<>]+ [a-z_]+\s*[=({]', act)

failed = [k for k, v in checks.items() if not v]
for k, v in checks.items():
    print(('PASS ' if v else 'FAIL ') + k)
if failed:
    raise SystemExit('V026 contracts failed: ' + ', '.join(failed))
print(f'PASS V026: {len(checks)} contracts (TESV sites, co-save frozen to v0.25.1, CRC == SKSE, render isolation, routes)')
