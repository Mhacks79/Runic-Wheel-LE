#!/usr/bin/env python3
"""Quality gate for Runic Wheel LE. Offline evidence never becomes gameplay proof.
Usage: python quality/validate.py --profile dev|release [--source path] [--ini path] [--results path]
Run from anywhere. Standard library only. --profile release deliberately fails before game QA.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
REQUIREMENTS = Path(__file__).with_name('ACCEPTANCE.json')
EXPECTED = {'SKSEPlugin_Query','SKSEPlugin_Load'}


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inspect(source, ini, requirements, profile='dev', results=None, dll=None):
    issues=[]
    src=Path(source).read_text(encoding='utf-8',errors='replace')
    cfg=Path(ini).read_text(encoding='utf-8',errors='replace')
    stripped=re.sub(r'/\*.*?\*/|//[^\n]*','',src,flags=re.S)
    # Reject actual legacy editor INI binding and runtime load. Names in comments don't count.
    obsolete=('EditorToggleScanCode','EditorUndoScanCode','EditorRedoScanCode','EditorToolScanCode')
    for key in obsolete:
        if re.search(r'^\s*'+key+r'\s*=',cfg,re.M) or re.search(r'\bvalue\s*\(\s*"'+key+r'"',stripped):
            issues.append(('FAIL','LEGACY-EDITOR',f'{key} reste actif'))
    # Regression from the v0.6/0.7 incident: F5 is Skyrim's native quicksave.
    if 'if(key && b->scan>=0x3C && b->scan<=0x3F)return 0;' not in stripped:
        issues.append(('FAIL','FUNCTION-KEY-GUARD','F2-F5 require unconditional passthrough'))
    is_task_reader='g_tasks->AddUITask' in stripped and 'hook_menu_render' not in stripped and 'install_ui_read_hooks' not in stripped
    if is_task_reader:
        checks=('g_tasks->interfaceVersion<2', 'InterlockedCompareExchange(&g_uiTaskQueued,1,0)',
                'InterlockedExchange(&g_uiTaskQueued,0)', 'rw10::valid_capacity(capacity)',
                '__except(ui_read_fault())')
        if any(token not in stripped for token in checks):
            issues.append(('BLOCK','UI-TASK-INCOMPLETE','UI task missing mandatory guards'))
        issues.append(('WARN','UI-TASK-COVERAGE','selected FormID observed in v0.10 game log; exact item identity, variants and broad compatibility unverified'))
    elif 'g_hoverCaptureEnabled=0;' not in stripped or 'g_hoverCaptureEnabled=value(' in stripped:
        issues.append(('BLOCK','UNPROVEN-UI-RUNTIME','unsafe menu Render hook must be quarantined'))
    if 'OpenMouseButton=2' not in cfg:
        issues.append(('WARN','MOUSE-DEFAULT','clic molette absent comme valeur par défaut'))
    if is_task_reader:
        match=re.search(r'^\s*EnableSafeInventoryUITask\s*=\s*([01])',cfg,re.M)
        if not match: issues.append(('FAIL','UI-FLAG','SKSE UI-task switch missing'))
        if 'EnableInventoryHoverCapture=1' in cfg: issues.append(('BLOCK','LEGACY-UNSAFE-FLAG','legacy menu Render option enabled'))
    else:
        match=re.search(r'^\s*EnableInventoryHoverCapture\s*=\s*([01])',cfg,re.M)
        if not match:
            issues.append(('FAIL','UI-FLAG','interrupteur du lecteur UI manquant'))
        elif match.group(1)=='1':
            issues.append(('BLOCK','UNPROVEN-UI-ENABLED',"lecteur SkyUI non confirmé en jeu mais activé par défaut après CTD v0.7"))
    if re.search(r'void\s+sync_time_impl\s*\([^)]*\)\s*\{\s*if\s*\(\s*!g_enableSlowTime\s*\|\|\s*!g_menuSinkReady\s*\)',stripped):
        issues.append(('BLOCK','TIME-DEPENDENCY',"le ralentissement est court-circuité lorsque l'écouteur de menus n'est pas prêt"))
    if re.search(r'\bGetInventoryItemList\s*\(',stripped):
        issues.append(('BLOCK','UNVERIFIED-AS-INVOKE',"appel ActionScript non validé réintroduit"))
    if re.search(r'\bkType_DisplayObject\s*=\s*9\b',stripped):
        issues.append(('FAIL','GFx-TYPE','DisplayObject est 8 dans le SDK LE, pas 9'))
    if 'SlowTimePercent' not in cfg:
        issues.append(('FAIL','SLOW-CONFIG','configuration vitesse absente'))
    if 'RunicWheelLE-diagnostic.log' not in stripped:
        issues.append(('WARN','EARLY-LOG','log temporaire non identifié dans la source active'))
    fingerprint={'source_sha256':sha256(source),'ini_sha256':sha256(ini)}
    if dll and Path(dll).is_file():
        fingerprint['dll_sha256']=sha256(dll)
    elif profile=='release':
        issues.append(('BLOCK','MISSING-DLL','DLL exacte à tester absente; aucune preuve binaire reproductible'))
    p0=[r for r in requirements['requirements'] if r['priority']=='P0']
    if profile=='release':
        if not results or not Path(results).is_file():
            issues.append(('BLOCK','GAME-TESTS','aucun rapport de test du jeu lié à ce build'))
        else:
            report=json.loads(Path(results).read_text(encoding='utf-8'))
            if report.get('fingerprint')!=fingerprint:
                issues.append(('BLOCK','WRONG-BUILD','preuves de test pour des fichiers différents'))
            evidence=report.get('scenarios',{})
            for row in p0:
                item=evidence.get(row['id'],{})
                if item.get('status')!='PASS' or not item.get('evidence'):
                    issues.append(('BLOCK',row['id'],'test P0 sans preuve en jeu pour ce build'))
    return issues,fingerprint


def main(argv=None):
    parser=argparse.ArgumentParser()
    parser.add_argument('--profile',choices=['dev','release'],default='dev')
    parser.add_argument('--source',type=Path,default=ROOT/'v09/RunicWheelV09.cpp')
    parser.add_argument('--ini',type=Path,default=ROOT/'v09/RunicWheelLE.ini')
    parser.add_argument('--results',type=Path,default=None)
    parser.add_argument('--dll',type=Path,default=None)
    parser.add_argument('--json',action='store_true')
    args=parser.parse_args(argv)
    try:
        req=json.loads(REQUIREMENTS.read_text(encoding='utf-8'))
        issues,fp=inspect(args.source,args.ini,req,args.profile,args.results,args.dll)
    except (OSError,ValueError,json.JSONDecodeError) as exc:
        print('FAIL input:',exc,file=sys.stderr); return 2
    if args.json:
        print(json.dumps({'profile':args.profile,'fingerprint':fp,'issues':issues},ensure_ascii=False,indent=2))
    else:
        print(f'RunicWheel quality gate: {args.profile}, source {args.source.name}')
        print('Source SHA256:',fp['source_sha256'])
        print('INI    SHA256:',fp['ini_sha256'])
        for severity,code,message in issues:
            print(f'[{severity}] {code}: {message}')
        if not issues:print('[PASS] All automated checks passed (NOT a game test).')
    return 1 if any(severity in ('FAIL','BLOCK') for severity,_,_ in issues) else 0

if __name__=='__main__':sys.exit(main())
