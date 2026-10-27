import sys,io,contextlib,re
def compress(start, pts):
    last=start; dirn=None; out=[]
    for p in pts:
        dd=(p[0]-last[0],p[1]-last[1])
        if dirn is not None and dd!=dirn: out.append(last)
        dirn=dd; last=p
    out.append(last)
    return ''.join("0 walk %d %d 3000\n"%(x,y) for x,y in out)
sys.argv=['gates.py','FortreeCity_Gym','15,23','15,3']
g={}
buf=io.StringIO()
with contextlib.redirect_stdout(buf): exec(open('gates.py').read(), g)
fwd=[tuple(map(int,l.split())) for l in buf.getvalue().splitlines() if not l.startswith('#')]
g['T']=(15,23)
back=[p for p,o in g['search'](g['pos'], g['oris'], g['blockers'])]
f='/home/claude/pokemon-rom-hack/tools/coop_link_harness/scenarios/journey_51_winona.txt'
s=open(f).read()
a=s.index('0 walk 15 23 2000\nb sync\n')+len('0 walk 15 23 2000\nb sync\n'); b=s.index('0 dump before-winona')
s=s[:a]+compress((15,23),fwd)+s[b:]
a=s.index('the trainers where they walked up to him).\n')+len('the trainers where they walked up to him).\n'); b=s.index('0 dump back-at-door')
s=s[:a]+compress((15,3),back)+s[b:]
open(f,'w').write(s)
print(len(fwd), len(back))
