# expand.py P < actions: cave.py's action lines as harness lines for player P.
#   walk X Y | step_to DIR X Y | surf DIR | smash DIR | strength DIR | push DIR
#   cross DIR G N (passed through)
import sys
p = sys.argv[1]
for line in sys.stdin:
    t = line.split()
    if not t or t[0].startswith('#'): continue
    k = t[0]
    if k == 'walk': print('%s walk %s %s 30000' % (p, t[1], t[2]))
    elif k == 'step_to': print('%s step_to %s %s %s' % (p, t[1], t[2], t[3]))
    elif k == 'surf': print('%s surf %s' % (p, t[1]))
    elif k == 'cross': print('%s cross %s %s %s 3000' % (p, t[1], t[2], t[3]))
    elif k == 'smash':
        print('%s press %s 2\n%s wait 10\n%s press A 2\n%s mash_field 4000' % (p, t[1], p, p, p))
    elif k == 'strength':
        print('%s press %s 2\n%s wait 10\n%s press A 2\n%s mash_field 4000\n%s press %s 24\n%s wait 60' % (p, t[1], p, p, p, p, t[1], p))
    elif k == 'push':
        print('%s press %s 24\n%s wait 60' % (p, t[1], p))
    else: print('%s %s' % (p, line.strip()))
