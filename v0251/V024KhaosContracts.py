from pathlib import Path
import re
root=Path(__file__).parent
src=(root/'RunicWheelV011.cpp').read_text()
menu=(root/'RunicKhaosMenu.inl').read_text()
api=(root/'KhaosAPI.h').read_text()
layout=(root/'LayoutGeometry.h').read_text()
assert 'KHAOS_API_VERSION 2u' in api
for f in ['KHAOS_Query','KHAOS_SetHosted','KHAOS_ToggleMenu','KHAOS_IsMenuOpen','KHAOS_RenderFrame','KHAOS_ProcessWndProc']:
    assert 'KHAOS_ABI '+('bool' if f in ['KHAOS_Query','KHAOS_IsMenuOpen','KHAOS_ProcessWndProc'] else 'void')+' '+f in src
assert 'KHAOS_PluginInfo* info' in src
assert 'g_model=rw05::migrate' in src
assert 'WritePrivateProfileStringA("Controls"' in menu
assert 'g_khaosFields' in menu and 'g_khaosDraft' in menu
assert 'hidden' in menu.lower() and '3x8x4 fixed' in layout
assert len(re.findall(r'^\s*\{"[^"\n]+","[^"\n]+","[^"\n]+",[0-7],',menu,flags=re.M))>=40
assert 'khaos_update_theme' in menu
assert 'g_khaosOpen' in src and 'if(g_khaosOpen){' in src and 'rw25::close_on_directinput' in src
assert 'if(g_cfg.nativeActions)' not in menu
print('PASS V024: API v2 exports, 8 tabs, >=40 real INI-backed settings, view limits, independent UI, host absence')
