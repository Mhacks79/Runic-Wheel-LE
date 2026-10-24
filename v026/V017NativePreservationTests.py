#!/usr/bin/env python3
"""Strict game-thread native delta: only classification + isolated owned-torch branch.
This does not establish runtime safety, but blocks accidental edits to previously
player-observed food, sword, spell, armor, ammo, or ingredient safety paths.
"""
from pathlib import Path
p=Path(__file__).resolve().parent
now=(p/'RunicWheelV011.cpp').read_text()
old=(p/'REFERENCE'/'v016-RunicWheelV011.cpp').read_text()
start='void perform_action(const ActionRequest& request){'
end='// Only reads TESForm.formType'
now=now[now.index(start):now.index(end)]
old=old[old.index(start):old.index(end)]
anchor='    if(kind==rw05::Torch){'
stop='    if(kind==rw05::Armor){'
assert now.count(anchor)==1
now=now[:now.index(anchor)] + now[now.index(stop,now.index(anchor)):]
now=now.replace(' && kind!=rw05::Torch','')
assert now==old, 'UNEXPECTED NATIVE ACTION CHANGE; compare against v016 before release'
print('PASS v017: all previously shipped native actions exact byte-for-byte source baseline after isolating torch branch')
