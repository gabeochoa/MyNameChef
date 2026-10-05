#!/usr/bin/env python3
"""CORRECT: random_device/rand only in src/seeded_rng.h. Past: 6531cfe,9674e1a,0f42784,c1ba51b."""
import pathlib,re,sys
root=pathlib.Path(__file__).parent.parent; allow={root/'src/seeded_rng.h'}; pat=re.compile(r'random_device|\brand\s*\(|\bsrand\s*\('); bad=[]
for f in root.glob('src/**/*'):
 if f.suffix in ('.cpp','.h') and f not in allow:
  [bad.append(f'{f.relative_to(root)}:{i}: {l.strip()}') for i,l in enumerate(f.read_text(errors='ignore').splitlines(),1) if pat.search(l)]
print('\n'.join(bad) if bad else 'rng check ok'); sys.exit(1 if bad else 0)
