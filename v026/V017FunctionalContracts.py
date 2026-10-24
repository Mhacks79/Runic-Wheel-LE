#!/usr/bin/env python3
from pathlib import Path
p=Path(__file__).resolve().parent
s=(p/'RunicWheelV011.cpp').read_text()
i=(p/'RunicWheelLE.ini').read_text()
assert 'LeftClickEquipsRightHand=1' in i
assert 'log_action(g_cfg.leftClickRightHand?"RIGHT":"LEFT")' in s
assert 'log_action(g_cfg.leftClickRightHand?"LEFT":"RIGHT")' in s
assert 'if(inventory_context())edit_click(false)' in s and 'if(inventory_context())edit_click(true)' in s
assert 'if(g_cfg.keyboardMode!=3)return fallback;' in s
assert 'KeyboardLayoutMode=2' in i and 'PreviousWheelScanCode=16' in i and 'NextWheelScanCode=18' in i
assert 'if(g_activation.visible && mouse &&' in s and 'accepted single mouse wheel detent' in s
assert 'if(down || b->duration==0.0f)' not in s
assert 'rw20::accept(g_scrollEdges[wheelIndex]' in s
assert 'case 0x1F:return rw05::Torch;' in s and 'case 0x1F:return Torch;' in (p/'IconPolicy.h').read_text()
assert 'TORCH equip request submitted' in s and 'TORCH unequip request submitted' in s
assert 'ingredient request submitted (verify effect in game)' in s  # v0.26: ingredient route opened, see V026Contracts
assert 'kSaveVersion=2' in s or 'kSaveVersion = 2' in s
print('PASS V017: hands, AZERTY/physical navigation, wheel pulse, owned torch, ingredient gate, co-save backward compatibility')
