#!/usr/bin/env python3
"""CORRECT: single timing scale issue65 6c20cfe - never systems.run(dt*scale)."""
import pathlib,re,sys
root=pathlib.Path(__file__).parent.parent; allow={'src/systems/UnifiedAnimationSystem.h','src/battle_timing.h'}; bad=[]
for f in root.glob('src/**/*'):
 if f.suffix in ('.cpp','.h'):
  rel=str(f.relative_to(root))
  [bad.append(f'{rel}:{i}: {l.strip()}') for i,l in enumerate(f.read_text(errors='ignore').splitlines(),1) if (re.search(r'\.run\(dt\s*\*',l) or (re.search(r'dt\s*\*\s*render_backend::timing_speed_scale',l) and rel not in allow))]
print('\n'.join(bad) if bad else 'dt scale check ok'); sys.exit(1 if bad else 0)
