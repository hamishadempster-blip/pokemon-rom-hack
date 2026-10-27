# reach.py Map x,y [rows]: land reachability (elevation rules, ledges, objects) printed as a grid.
import sys,json,struct,re
from collections import deque
root='/home/claude/pokemon-rom-hack/'
m=sys.argv[1]; s=tuple(map(int,sys.argv[2].split(',')))
y0,y1=(map(int,sys.argv[3].split('-'))) if len(sys.argv)>3 else (0,999)
d=json.load(open(root+'data/maps/%s/map.json'%m))
L=[l for l in json.load(open(root+'data/layouts/layouts.json'))['layouts'] if l.get('id')==d['layout']][0]
def folder(n): return re.sub(r'(?<!^)(?=[A-Z])','_',n.replace('gTileset_','')).lower()
def attrs(p):
    b=open(p,'rb').read(); return list(struct.unpack('<%dH'%(len(b)//2),b))
pa=attrs(root+'data/tilesets/primary/%s/metatile_attributes.bin'%folder(L['primary_tileset']))
sa=attrs(root+'data/tilesets/secondary/%s/metatile_attributes.bin'%folder(L['secondary_tileset']))
names=[l.strip().rstrip(',') for l in open(root+'include/constants/metatile_behaviors.h') if l.strip().startswith('MB_')]
b=open(root+L['blockdata_filepath'],'rb').read(); v=struct.unpack('<%dH'%(len(b)//2),b)
W=L['width'];H=L['height']
objs={(o["x"],o["y"]):o["graphics_id"] for o in d["object_events"] if (o["x"],o["y"]) not in {tuple(map(int,a.split(","))) for a in __import__("os").environ.get("FREE","").split()}}
def tile(x,y):
    e=v[y*W+x]; mid=e&0x3ff; a=(pa+[0]*(512-len(pa)))[mid] if mid<512 else sa[mid-512]
    return (e>>12)&15,(e>>10)&3,names[a&0xff]
def water(bh): return 'BRIDGE' not in bh and any(k in bh for k in ('OCEAN','POND','SEMI_DEEP','DEEP_WATER','WATERFALL','CURRENT','_WATER'))
J={(0,1):'MB_JUMP_SOUTH',(0,-1):'MB_JUMP_NORTH',(-1,0):'MB_JUMP_WEST',(1,0):'MB_JUMP_EAST'}
st=(s,tile(*s)[0]); seen={st}; q=deque([st])
while q:
    (x,y),ce=q.popleft()
    for (dx,dy),jn in J.items():
        nx,ny=x+dx,y+dy
        if not(0<=nx<W and 0<=ny<H): continue
        e,c,bh=tile(nx,ny); ne=ce
        if 'JUMP' in bh:
            if bh!=jn: continue
            nx,ny=nx+dx,ny+dy
            if not(0<=nx<W and 0<=ny<H): continue
            e,c,bh=tile(nx,ny); ne=e
        if c or (nx,ny) in objs or water(bh) or bh in ('MB_MUDDY_SLOPE','MB_BUMPY_SLOPE') and dy!=1: continue
        if not(e in (0,15) or ne in (0,15) or e==ne): continue
        n=((nx,ny), ne if e==15 else e)
        if n in seen: continue
        seen.add(n); q.append(n)
R={p for p,_ in seen}
for y in range(max(0,y0),min(H,y1+1)):
    r=''
    for x in range(W):
        e,c,bh=tile(x,y)
        ch='*' if (x,y) in R else ('O' if (x,y) in objs else ('#' if c else ('~' if water(bh) else '.')))
        r+=ch
    print('%3d %s'%(y,r))
