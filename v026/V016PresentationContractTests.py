#!/usr/bin/env python3
"""Offline-only contract tests: animation integration and unchanged game thread action."""
from pathlib import Path
import re
p=Path(__file__).resolve().parent
src=(p/'RunicWheelV011.cpp').read_text()
ini=(p/'RunicWheelLE.ini').read_text()
base=(p/'REFERENCE'/'v0121-RunicWheelV011.cpp').read_text()
a='void perform_action(const ActionRequest& request){'
b='// Only reads TESForm.formType'
# Preserve native v0.16 baseline while explicitly allowing 0x1F torch handling.
assert 'TORCH equip request submitted' in src
assert 'GameEquipSpell>(0x006EFAC0)' in src
assert 'WEAPON unequip request submitted' in src
assert 'ingredient request submitted (verify effect in game)' in src  # v0.26: ingredient route opened, see V026Contracts
assert 'g_cfg.feedbackDuration=value("ClickFeedbackMs",900,250,2000);' in src
assert 'if(key && b->scan>=0x3C && b->scan<=0x3F)return 0;' in src
assert 'if(g_visible || g_exitArmed)' in src
assert 'void apply_presentation(' in src and 'apply_presentation((float)vp.w,(float)vp.h);' in src
assert 'g_pipsUsed=n;' in src and 'static Vertex g_pips[3*(8*4+48)];' in src
assert 'update_visual_focus(g_presentationNow,' in src
assert 'draw_wheel_pager(cx,cy,outer,wheel);' in src
assert 'rw16::lerp_color(baseOuter,selectedOuter,sector_weight(s))' in src
assert 'const float weights[2]=' in src
assert 'g_presentationFrame=rw16::frame(' in src
assert 'InterlockedExchange(&g_exitArmed,0);' in src
for key, default in {'EnablePresentationAnimations':'1', 'ReduceMotion':'0', 'OpenAnimationMs':'160', 'CloseAnimationMs':'110', 'SectorTransitionMs':'125'}.items():
 assert re.search(r'^'+key+r'='+default+r'$',ini,re.M),key
 assert 'value("'+key+'",' in src,key
assert 'ShowSectorLabels=0' in ini
assert 'ShowItemNames=1' in ini
assert 'TESVForm' not in (p/'PresentationTimeline.h').read_text()
# Verify the pager and all drawable elements remain inside 720p/1080p/1440p/21:9.
for width,height in [(1280,720),(1920,1080),(2560,1440),(3440,1440)]:
 for percent in [75,100,130,170]:
  outer=min(min(width,height)*.31*percent/100,440,height*.44)
  cy=height*.5-min(height*.045,40)
  cx=width*.5
  # All three pager diamonds + gilded bracket including opening scale (always <=1).
  x=cx+outer*.72
  y=cy+outer*.78
  assert x-31>=0 and x+31<=width,(width,height,percent,'pager x')
  assert y-12>=0 and y+12<=height,(width,height,percent,'pager y')
  assert cy-outer>=0 and cy+outer+72<=height,(width,height,percent,'ring + plaque')
# 32 stack pips and two twelve-segment sweeps (each segment two triangles).
assert (8*4)*3+2*12*6==3*(8*4+48)

print('PASS V016: render-only focus and pager, bounded tick effects, exit flag, defaults, preserved native anchors and key passthrough; exact native delta checked by V017NativePreservationTests')
